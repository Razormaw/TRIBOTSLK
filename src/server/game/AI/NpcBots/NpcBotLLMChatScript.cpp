/*
 * NpcBotLLMChatScript.cpp
 * Scripts de chat LLM para NPCBots de TRIBOTSLK
 * Corregido: packet SMSG_MESSAGECHAT con PackedGuid, LANG_UNIVERSAL fijo,
 *            canal 'world' fijo, sin SendGlobalMessage para canales.
 */

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
#include "Map.h"

// ---------------------------------------------------------------------------
// Estructura de cola para respuestas pendientes (thread-safe)
// ---------------------------------------------------------------------------
struct PendingBotChat {
    ObjectGuid botGuid;
    std::string reply;
    std::string channelName;
};

static std::queue<PendingBotChat> g_pendingBotChats;
static std::mutex g_pendingBotChatsMutex;

// ---------------------------------------------------------------------------
// Envía un mensaje de canal desde un NPCBot (Creature) al mundo.
// IMPORTANTE: NO usa SendGlobalMessage; usa SendToAllPlayers del mapa del bot.
// ---------------------------------------------------------------------------
static void SendBotChannelMessage(Creature* bot, const std::string& channelName,
                                  const std::string& msg)
{
    if (!bot || !bot->IsInWorld() || !bot->IsAlive())
        return;

    // CAPA 1 (opcional): name query response para que el cliente cachee el nombre.
    // El opcode SMSG_NAME_QUERY_RESPONSE exige: guid + unknown + nameLen + name +
    // realmNameLen + race + gender + class.  Si el nameLen o realmNameLen faltan,
    // el cliente descarta el packet silenciosamente y luego no muestra el nombre.
    {
        std::string const& botName = bot->GetName();
        WorldPacket nameData(SMSG_NAME_QUERY_RESPONSE, botName.size() + 40);
        nameData << bot->GetGUID();                  // ObjectGuid completo (lo exige este opcode)
        nameData << uint8(0);                        // unknown
        nameData << uint8(botName.size());           // nameLen
        nameData << botName;                         // name (sin null)
        nameData << uint8(0);                        // realmNameLen = 0 (mismo reino)
        nameData << uint8(bot->GetByteValue(UNIT_FIELD_BYTES_0, 0));  // race
        nameData << uint8(bot->GetByteValue(UNIT_FIELD_BYTES_0, 1));  // gender
        nameData << uint8(bot->GetByteValue(UNIT_FIELD_BYTES_0, 2));  // class
        sWorld->SendGlobalMessage(&nameData);
    }

    // CAPA 2: mensaje de canal propiamente dicho.
    // Estructura correcta para CHAT_MSG_CHANNEL en 3.3.5a:
    //   uint8     type
    //   uint32    language
    //   PackedGuid senderGuid
    //   uint32    channelNameLen  (incluye null)
    //   char[]    channelName     (con null)
    //   PackedGuid senderGuid2    (sí, se repite para CHANNEL)
    //   uint32    msgLen          (incluye null)
    //   char[]    msg             (con null)
    //   uint8     chatTag
    WorldPacket data(SMSG_MESSAGECHAT, msg.size() + channelName.size() + 40);
    data << uint8(CHAT_MSG_CHANNEL);
    data << uint32(LANG_UNIVERSAL);                  // forzado: los canales en 3.3.5a son universal
    data << bot->GetGUID();                          // PackedGuid (ByteBuffer lo hace solo)
    data << uint32(channelName.size() + 1);
    data << channelName.c_str();
    data << bot->GetGUID();                          // PackedGuid repetido
    data << uint32(msg.size() + 1);
    data << msg.c_str();
    data << uint8(0);                                // chat tag
    sWorld->SendGlobalMessage(&data);

    // CAPA 3: puente de emergencia visible (CHAT_MSG_SYSTEM global).
    // CHAT_MSG_SYSTEM tiene estructura DISTINTA:
    //   uint8  type
    //   uint32 msgLen (incluye null)
    //   char[] msg    (con null)
    if (sConfigMgr->GetBoolDefault("NpcBot.LLM.FallbackSystemMsg", false))
    {
        std::string fallbackText = "[" + channelName + "] " + bot->GetName() + ": " + msg;
        WorldPacket sysData(SMSG_MESSAGECHAT, fallbackText.size() + 16);
        sysData << uint8(CHAT_MSG_SYSTEM);
        sysData << uint32(fallbackText.size() + 1);
        sysData << fallbackText.c_str();
        sWorld->SendGlobalMessage(&sysData);
    }

    BOT_LOG_INFO("npcbots", "LLM: mensaje de canal enviado por '{}' en '{}'.",
                 bot->GetName(), channelName);
}

// ---------------------------------------------------------------------------
// Construye el body JSON para APIs compatibles con OpenAI (Groq, OpenAI, etc.)
// ---------------------------------------------------------------------------
static std::string BuildLLMRequestBody(const std::string& prompt)
{
    std::string model = sConfigMgr->GetStringDefault("NpcBot.LLM.Model", "qwen/qwen3.8-27b");
    int maxTokens     = sConfigMgr->GetIntDefault("NpcBot.LLM.MaxTokens", 120);
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

// ---------------------------------------------------------------------------
// Extrae el contenido de la respuesta en formato OpenAI Chat Completions
// ---------------------------------------------------------------------------
static std::string ExtractOpenAIResponse(const std::string& json)
{
    size_t choicesStart = json.find("\"choices\"");
    if (choicesStart == std::string::npos)
        return "";

    size_t contentStart = json.find("\"content\"", choicesStart);
    if (contentStart == std::string::npos)
        return "";

    contentStart = json.find(":", contentStart);
    if (contentStart == std::string::npos)
        return "";

    size_t valueStart = json.find("\"", contentStart);
    if (valueStart == std::string::npos)
        return "";
    valueStart++;

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
                case '"': out += '"';  break;
                case '\\': out += '\\'; break;
                default:  out += n;    break;
            }
        }
        else if (c == '"')
            break;
        else
            out += c;
    }
    return out;
}

// ---------------------------------------------------------------------------
// PlayerScript: detecta mensajes en el canal 'world' (canal fijo del servidor)
// ---------------------------------------------------------------------------
class npcbot_llm_chat_player_script : public PlayerScript
{
public:
    npcbot_llm_chat_player_script() : PlayerScript("npcbot_llm_chat_player_script") { }

    void OnChat(Player* player, uint32 type, uint32 /*lang*/, std::string& msg, Channel* channel) override
    {
        if (type != CHAT_MSG_CHANNEL || !channel)
			        BOT_LOG_INFO("npcbots", "LLM: OnChat disparado (type={}, channel={}).",
                     type, channel ? channel->GetName() : "NULL");
            return;

        std::string channelName = channel->GetName();

        // Comparación case-insensitive: world = World = WORLD = mundo = general
        std::string lower = channelName;
        for (size_t i = 0; i < lower.size(); ++i)
            lower[i] = (char)tolower((unsigned char)lower[i]);

        if (lower != "world" && lower != "mundo" && lower != "general")
			        BOT_LOG_INFO("npcbots", "LLM: canal aceptado '{}', continuando con LLM.", channelName);
            return;

        BOT_LOG_INFO("npcbots", "LLM: mensaje detectado de '{}' en canal '{}'.",
                     player->GetName(), channelName);

        if (!sConfigMgr->GetBoolDefault("NpcBot.LLM.Enable", false))
        {
            BOT_LOG_INFO("npcbots", "LLM: desactivado por configuracion (NpcBot.LLM.Enable = 0).");
            return;
        }

                // Recoger todos los NPCBots activos del mundo
        std::vector<Creature*> availableBots;
        NpcBotRegistry const& registry = BotDataMgr::GetExistingNPCBots();
        for (NpcBotRegistry::const_iterator itr = registry.begin(); itr != registry.end(); ++itr)
        {
            Creature* bot = const_cast<Creature*>(*itr);
            if (!bot)
                continue;
            if (!bot->IsInWorld())
                continue;
            if (!bot->IsAlive())
                continue;
            if (!bot->IsNPCBot())        // ← guard extra: solo bots reales
                continue;
            availableBots.push_back(bot);
        }

        BOT_LOG_INFO("npcbots", "LLM: bots disponibles para responder: {}",
                     (uint32)availableBots.size());

        if (availableBots.empty())
            return;

        // Seleccionar bot aleatorio
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distrib(0, (int)availableBots.size() - 1);
        Creature* responderBot = availableBots[distrib(gen)];
        ObjectGuid botGuid = responderBot->GetGUID();
        std::string botName = responderBot->GetName();

        // Prompt para el LLM
        std::string prompt = "You are an NPC bot named '" + botName + "' in a World of Warcraft 3.3.5a private server. ";
        prompt += "A player named '" + player->GetName() + "' said in the '" + channelName + "' channel: \"" + msg + "\". ";
        prompt += "Reply briefly (max 150 characters), in character, and in the same language as the player. Do not use quotes or markdown.";

        // Datos para el hilo asíncrono
        struct ThreadData {
            ObjectGuid guid;
            std::string name;
            std::string prompt;
            std::string channel;
        };

        ThreadData* tdata = new ThreadData();
        tdata->guid    = botGuid;
        tdata->name    = botName;
        tdata->prompt  = prompt;
        tdata->channel = channelName;

        std::thread([](ThreadData* d) {
            try
{
    std::vector<std::string> debugLines;
    int timeout = sConfigMgr->GetIntDefault("NpcBot.LLM.TimeoutSeconds", 15);
    int maxGen  = sConfigMgr->GetIntDefault("NpcBot.LLM.MaxGenerations", 3);

    std::string body = BuildLLMRequestBody(d->prompt);
    std::string response = NpcBotLLMInterface::Generate(body, timeout, maxGen, debugLines);

    if (response.empty() || response.find("error") == 0)
    {
        BOT_LOG_INFO("npcbots", "LLM: fallo de generacion para el bot '{}'.", d->name);
        delete d;
        return;
    }

    std::string finalReply = ExtractOpenAIResponse(response);
    if (finalReply.empty())
    {
        BOT_LOG_INFO("npcbots", "LLM: JSON sin respuesta util: {}",
                     response.substr(0, 200));
        delete d;
        return;
    }

    size_t first = finalReply.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) { delete d; return; }
    size_t last = finalReply.find_last_not_of(" \t\n\r");
    finalReply = finalReply.substr(first, last - first + 1);

    if (finalReply.empty()) { delete d; return; }

    {
        std::lock_guard<std::mutex> lock(g_pendingBotChatsMutex);
        PendingBotChat pending;
        pending.botGuid     = d->guid;
        pending.reply       = finalReply;
        pending.channelName = d->channel;
        BOT_LOG_INFO("npcbots", "LLM: respuesta lista de '{}': {}", d->name, finalReply);
        g_pendingBotChats.push(pending);
    }

    delete d;
}
	catch (const std::exception& e)
	{
		BOT_LOG_ERROR("npcbots", "LLM: excepcion en hilo: {}", e.what());
		delete d;
	}
           
        }, tdata).detach();
    }
};

// ---------------------------------------------------------------------------
// WorldScript: procesa la cola de respuestas pendientes en el hilo del mundo
// ---------------------------------------------------------------------------
class npcbot_llm_chat_world_script : public WorldScript
{
public:
    npcbot_llm_chat_world_script() : WorldScript("npcbot_llm_chat_world_script") { }

    void OnUpdate(uint32 /*diff*/) override
    {
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

            if (bot && bot->IsInWorld())
                SendBotChannelMessage(bot, task.channelName, task.reply);
        }
    }
};

// ---------------------------------------------------------------------------
// Registro de scripts (llamado desde ScriptLoader.cpp generado por CMake)
// ---------------------------------------------------------------------------
void AddSC_npcbot_llm_chat()
{
    new npcbot_llm_chat_player_script();
    new npcbot_llm_chat_world_script();

    BOT_LOG_INFO("npcbots", "LLM: scripts de chat registrados correctamente.");
}

