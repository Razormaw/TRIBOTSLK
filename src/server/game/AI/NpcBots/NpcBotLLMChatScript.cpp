#include "ScriptMgr.h"
#include "Player.h"
#include "Channel.h"
#include "ChannelMgr.h"
#include "Chat.h"
#include "ObjectMgr.h"
#include "World.h"
#include "Config.h"
#include "NpcBotLLMInterface.h"
#include "botmgr.h"
#include "bot_ai.h"
#include "botdatamgr.h"
#include "botlog.h"
#include <cctype>
#include <thread>
#include <mutex>
#include <queue>
#include <random>
#include "Opcodes.h"

struct PendingBotChat {
    ObjectGuid botGuid;
    std::string reply;
    uint32 lang;
    std::string channelName;
};

static std::queue<PendingBotChat> g_pendingBotChats;
static std::mutex g_pendingBotChatsMutex;

// Helper para enviar un mensaje de canal desde una Creature (Bot)
static void SendBotChannelMessage(Creature* bot, const std::string& channelName, const std::string& msg, uint32 lang)
{
    // CAPA 1: ensenar al cliente el nombre del bot (respuesta de consulta de nombre)
    WorldPacket nameData(SMSG_NAME_QUERY_RESPONSE, bot->GetName().size() + 32);
    nameData << bot->GetGUID();
    nameData << uint8(0);
    nameData << bot->GetName();
    nameData << std::string("");
    nameData << uint8(1);
    nameData << uint8(0);
    nameData << uint8(0);
    sWorld->SendGlobalMessage(&nameData);

    // CAPA 2: el mensaje de canal propiamente dicho
    WorldPacket data(SMSG_MESSAGECHAT, msg.size() + channelName.size() + 32);
    data << uint8(CHAT_MSG_CHANNEL);
    data << uint32(lang);
    data << bot->GetGUID();
    data << uint32(channelName.size() + 1);
    data << channelName.c_str();
    data << uint32(msg.size() + 1);
    data << msg.c_str();
    data << uint8(0);
    sWorld->SendGlobalMessage(&data);

    // CAPA 3: puente de emergencia visible (linea de sistema global), configurable
    if (sConfigMgr->GetBoolDefault("NpcBot.LLM.FallbackSystemMsg", true))
    {
        std::string fallbackText = "[" + channelName + "] " + bot->GetName() + ": " + msg;

        WorldPacket sysData(SMSG_MESSAGECHAT, fallbackText.size() + 32);
        sysData << uint8(CHAT_MSG_SYSTEM);
        sysData << uint32(LANG_UNIVERSAL);
        sysData << uint32(fallbackText.size() + 1);
        sysData << fallbackText.c_str();
        sysData << uint8(0);
        sWorld->SendGlobalMessage(&sysData);
    }

    BOT_LOG_INFO("npcbots", "LLM: enviando mensaje de canal de {} en '{}'.", bot->GetName(), channelName);
}

// Construye el cuerpo JSON compatible con OpenAI/Groq Chat Completions API
static std::string BuildLLMRequestBody(const std::string& prompt)
{
    std::string model = sConfigMgr->GetStringDefault("NpcBot.LLM.Model", "qwen/qwen3.8-27b");
    int maxTokens = sConfigMgr->GetIntDefault("NpcBot.LLM.MaxTokens", 120);
    float temperature = sConfigMgr->GetFloatDefault("NpcBot.LLM.Temperature", 0.7f);
    
    std::string systemPrompt = "Eres un NPC bot en un servidor privado de World of Warcraft 3.3.5a. ";
    systemPrompt += "Responde de forma breve, en personaje, y en el mismo idioma del jugador. ";
    systemPrompt += "No uses comillas, markdown ni emojis.";
    
    std::string body = "{";
    body += "\"model\": \"" + model + "\", ";
    body += "\"messages\": [";
    body += "{\"role\": \"system\", \"content\": \"" + NpcBotLLMInterface::SanitizeForJson(systemPrompt) + "\"}, ";
    body += "{\"role\": \"user\", \"content\": \"" + NpcBotLLMInterface::SanitizeForJson(prompt) + "\"}";
    body += "], ";
    body += "\"max_tokens\": " + std::to_string(maxTokens) + ", ";
    body += "\"temperature\": " + std::to_string(temperature);
    body += "}";
    
    return body;
}

// Extrae el contenido de la respuesta del formato OpenAI Chat Completions
static std::string ExtractOpenAIResponse(const std::string& json)
{
    // Buscar "choices":[{"message":{"content":"..."}}]
    size_t choicesStart = json.find("\"choices\"");
    if (choicesStart == std::string::npos)
        return "";
    
    size_t contentStart = json.find("\"content\"", choicesStart);
    if (contentStart == std::string::npos)
        return "";
    
    // Avanzar hasta el valor del contenido
    contentStart = json.find(":", contentStart);
    if (contentStart == std::string::npos)
        return "";
    
    // Buscar la primera comilla después de los dos puntos
    size_t valueStart = json.find("\"", contentStart);
    if (valueStart == std::string::npos)
        return "";
    valueStart++; // Saltar la comilla de apertura
    
    std::string out;
    for (size_t i = valueStart; i < json.size(); ++i)
    {
        char c = json[i];
        if (c == '\\' && i + 1 < json.size())
        {
            char n = json[++i];
            switch (n)
            {
                case 'n': out += '\n'; break;
                case 't': out += '\t'; break;
                case 'r': out += '\r'; break;
                case '"': out += '"'; break;
                case '\\': out += '\\'; break;
                default: out += n; break;
            }
        }
        else if (c == '"')
            break;
        else
            out += c;
    }
    return out;
}

class npcbot_llm_chat_player_script : public PlayerScript
{
public:
    npcbot_llm_chat_player_script() : PlayerScript("npcbot_llm_chat_player_script") { }

        void OnChat(Player* player, uint32 type, uint32 lang, std::string& msg, Channel* channel) override
    {
        if (type != CHAT_MSG_CHANNEL || !channel)
            return;

        std::string channelName = channel->GetName();

        // Comparar sin importar mayusculas/minusculas (world = World = WORLD)
        std::string channelNameLower = channelName;
        for (size_t i = 0; i < channelNameLower.size(); ++i)
            channelNameLower[i] = (char)tolower((unsigned char)channelNameLower[i]);

        if (channelNameLower != "world" && channelNameLower != "mundo" && channelNameLower != "general")
            return;

        BOT_LOG_INFO("npcbots", "LLM: mensaje detectado de {} en canal '{}'.", player->GetName(), channelName);

        if (!sConfigMgr->GetBoolDefault("NpcBot.LLM.Enable", false))
        {
            BOT_LOG_INFO("npcbots", "LLM: desactivado por configuracion (NpcBot.LLM.Enable = 0).");
            return;
        }

        // TODOS los bots del mundo: contratados + vagabundos (generados automaticamente)
        std::vector<Creature*> availableBots;
        NpcBotRegistry const& registry = BotDataMgr::GetExistingNPCBots();
        for (NpcBotRegistry::const_iterator itr = registry.begin(); itr != registry.end(); ++itr)
        {
            Creature* bot = const_cast<Creature*>(*itr);
            if (bot && bot->IsInWorld() && bot->IsAlive())
                availableBots.push_back(bot);
        }

        BOT_LOG_INFO("npcbots", "LLM: bots disponibles para responder: {}", (uint32)availableBots.size());

        if (availableBots.empty())
            return; // No hay bots disponibles para responder

        // Seleccionar bot aleatorio
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distrib(0, availableBots.size() - 1);
        Creature* responderBot = availableBots[distrib(gen)];
        ObjectGuid botGuid = responderBot->GetGUID();
        std::string botName = responderBot->GetName();

        // Construir el prompt para el LLM
        std::string prompt = "You are an NPC bot named '" + botName + "' in a World of Warcraft 3.3.5a private server. ";
        prompt += "A player named '" + player->GetName() + "' said in the '" + channelName + "' channel: \"" + msg + "\". ";
        prompt += "Reply briefly (max 150 characters), in character, and in the same language as the player. Do not use quotes or markdown.";

        // Lanzar tarea asíncrona - sin lambda compleja
        struct ThreadData {
            ObjectGuid guid;
            std::string name;
            std::string prompt;
            uint32 lang;
            std::string channel;
        };
        
        ThreadData* data = new ThreadData();
        data->guid = botGuid;
        data->name = botName;
        data->prompt = prompt;
        data->lang = lang;
        data->channel = channelName;

        std::thread([](ThreadData* d) {
            try
            {
				std::vector<std::string> debugLines;

                int timeout = sConfigMgr->GetIntDefault("NpcBot.LLM.TimeoutSeconds", 15);
                int maxGen = sConfigMgr->GetIntDefault("NpcBot.LLM.MaxGenerations", 3);

                std::string body = BuildLLMRequestBody(d->prompt);
				std::string response = NpcBotLLMInterface::Generate(body, timeout, maxGen, debugLines);

				if (response.empty() || response.find("error") == 0)
				{
					BOT_LOG_INFO("npcbots", "LLM: fallo de generacion para el bot {}.", d->name);
					delete d;
					return;
				}

				// Extraer la respuesta del formato OpenAI/Groq
				std::string finalReply = ExtractOpenAIResponse(response);

				if (finalReply.empty())
				{
					BOT_LOG_INFO("npcbots", "LLM: JSON sin respuesta util: {}", response.substr(0, 200));
					delete d;
					return;
				}
                
                // Trim
                finalReply.erase(0, finalReply.find_first_not_of(" \t\n\r"));
                finalReply.erase(finalReply.find_last_not_of(" \t\n\r") + 1);

                if (finalReply.empty())
                {
                    delete d;
                    return;
                }

                // Encolar
                {
                    std::lock_guard<std::mutex> lock(g_pendingBotChatsMutex);
                    PendingBotChat pending;
                    pending.botGuid = d->guid;
                    pending.reply = finalReply;
                    pending.lang = d->lang;
                    pending.channelName = d->channel;
					BOT_LOG_INFO("npcbots", "LLM: respuesta lista de {}: {}", d->name, finalReply);
                    g_pendingBotChats.push(pending);
                }
                
                delete d;
            }
            catch (const std::exception& e)
            {
                BOT_LOG_ERROR("npcbots", "LLM: excepcion en hilo: {}", e.what());
                delete d;
            }
        }, data).detach();
    }
};

class npcbot_llm_chat_world_script : public WorldScript
{
public:
    npcbot_llm_chat_world_script() : WorldScript("npcbot_llm_chat_world_script") { }

    void OnUpdate(uint32 /*diff*/) override
    {
        // Procesar cola de chats pendientes de forma segura
        std::queue<PendingBotChat> localQueue;
        {
            std::lock_guard<std::mutex> lock(g_pendingBotChatsMutex);
            if (g_pendingBotChats.empty())
                return;
            std::swap(localQueue, g_pendingBotChats);
        }

        while (!localQueue.empty())
        {
            PendingBotChat task = localQueue.front();
            localQueue.pop();

            // Buscar el bot en el registro global de NPCBots (independiente del mapa)
            Creature* bot = NULL;
            NpcBotRegistry const& registry = BotDataMgr::GetExistingNPCBots();
            for (NpcBotRegistry::const_iterator itr = registry.begin(); itr != registry.end(); ++itr)
            {
                if (*itr && (*itr)->GetGUID() == task.botGuid)
                {
                    bot = const_cast<Creature*>(*itr);
                    break;
                }
            }

            if (bot && bot->IsNPCBot() && bot->IsInWorld())
            {
                SendBotChannelMessage(bot, task.channelName, task.reply, task.lang);
            }
        }
    }
};

// Registro garantizado: se ejecuta una sola vez, desde el primer bot que nace
// Registro garantizado: se ejecuta una sola vez, desde el primer bot que nace
void EnsureNpcBotLLMChatScriptsRegistered()
{
    static bool s_registered = false;
    if (s_registered)
        return;
    s_registered = true;

    // TrinityCore exige un "contexto de script" valido para poder registrar scripts
    sScriptMgr->SetScriptContext("npcbots_llm");
    new npcbot_llm_chat_player_script();
    new npcbot_llm_chat_world_script();
    sScriptMgr->SetScriptContext("");

    BOT_LOG_INFO("npcbots", "LLM: scripts de chat registrados correctamente.");
}

void AddSC_npcbot_llm_chat()
{
    new npcbot_llm_chat_player_script();
    new npcbot_llm_chat_world_script();
}

