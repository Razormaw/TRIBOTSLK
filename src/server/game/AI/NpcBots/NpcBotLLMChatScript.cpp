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
#include <algorithm>
#include "DatabaseEnv.h"
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
// DIRECTOR DE CONVERSACIONES BOT-A-BOT (estilo Taberna)
// ---------------------------------------------------------------------------
struct BotConversation
{
    ObjectGuid botA;
    ObjectGuid botB;
    std::string seedPhrase;
    uint8 turnsLeft;
    bool nextIsBotA;      // true = el que habla ahora es botA
};

static std::queue<BotConversation> g_activeConversations;
static std::mutex g_conversationMutex;
static uint32 g_botToBotTimer = 0;
static std::vector<std::string> g_phrasesStart;
static std::vector<std::string> g_phrasesResponse;
static bool g_phrasesLoaded = false;

static void LoadPhrasesFromDB()
{
    g_phrasesStart.clear();
    g_phrasesResponse.clear();

    QueryResult result = WorldDatabase.Query("SELECT category, text FROM npcbot_llm_phrases");
    if (result)
    {
        do
        {
            Field* fields = result->Fetch();
            std::string category = fields[0].GetString();
            std::string text     = fields[1].GetString();
            if (category == "inicio")
                g_phrasesStart.push_back(text);
            else if (category == "respuesta")
                g_phrasesResponse.push_back(text);
        } while (result->NextRow());
    }

    g_phrasesLoaded = true;
    BOT_LOG_INFO("npcbots", "LLM: frases de taberna cargadas: {} de inicio, {} de respuesta.",
                 (uint32)g_phrasesStart.size(), (uint32)g_phrasesResponse.size());
}

// ---------------------------------------------------------------------------
// CADENA DE RESPUESTAS: varios bots contestan en fila al jugador
// ---------------------------------------------------------------------------
struct ChainTurn
{
    ObjectGuid botGuid;                // bot que habla ahora
    std::string seedText;              // ultimo mensaje dicho
    std::string seedAuthor;            // quien lo dijo
    std::string playerName;            // jugador que sembro la charla
    std::vector<ObjectGuid> nextBots;  // bots de los turnos siguientes
    uint8 turnsLeft;                   // turnos restantes despues de este
	std::string channelName;
};

static std::queue<ChainTurn> g_playerChains;
static std::mutex g_chainMutex;

static Creature* FindBotByGuid(ObjectGuid guid)
{
    NpcBotRegistry const& registry = BotDataMgr::GetExistingNPCBots();
    for (NpcBotRegistry::const_iterator itr = registry.begin(); itr != registry.end(); ++itr)
    {
        if (*itr && (*itr)->GetGUID() == guid)
            return const_cast<Creature*>(*itr);
    }
    return nullptr;
}

// ---------------------------------------------------------------------------
// Envía un mensaje de canal desde un NPCBot (Creature) al mundo.
// IMPORTANTE: NO usa SendGlobalMessage; usa SendToAllPlayers del mapa del bot.
// ---------------------------------------------------------------------------
static void SendBotChannelMessage(Creature* bot, const std::string& channelName,
                                  const std::string& msg)
{
    if (!bot || !bot->IsInWorld() || !bot->IsAlive())
        return;

    // Crear un GUID virtual de jugador basado en el entry del bot
    // Esto engaña al cliente para que trate al bot como si fuera un jugador
        ObjectGuid vGuid = ObjectGuid::Create<HighGuid::Player>(uint32(0x00F00000 + bot->GetEntry()));

    // CAPA 1: SMSG_NAME_QUERY_RESPONSE para que el cliente cachee el nombre del "jugador"
    // Estructura 3.3.5a: PackedGuid + unknown + name + realmName + race + gender + class
    std::string const& botName = bot->GetName();
    WorldPacket nameData(SMSG_NAME_QUERY_RESPONSE, botName.size() + 40);
    nameData.appendPackGUID(vGuid.GetRawValue());
    nameData << uint8(0);                 // unknown = 0 (éxito)
    nameData << botName;                  // nombre (ByteBuffer agrega null automáticamente)
    nameData << std::string("");          // realmName vacío
    nameData << uint8(bot->GetByteValue(UNIT_FIELD_BYTES_0, 0));  // race
    nameData << uint8(bot->GetByteValue(UNIT_FIELD_BYTES_0, 1));  // gender
    nameData << uint8(bot->GetByteValue(UNIT_FIELD_BYTES_0, 2));  // class
    sWorld->SendGlobalMessage(&nameData);

    // CAPA 2: mensaje de canal usando el GUID virtual
    WorldPacket data(SMSG_MESSAGECHAT, msg.size() + channelName.size() + 40);
    data << uint8(CHAT_MSG_CHANNEL);
    data << uint32(LANG_UNIVERSAL);
    data << vGuid;                          // GUID virtual (el cliente busca el nombre cacheado)
    data << uint32(channelName.size() + 1);
    data << channelName.c_str();
    data << vGuid;                          // repetido para CHANNEL
    data << uint32(msg.size() + 1);
    data << msg.c_str();
    data << uint8(0);
    sWorld->SendGlobalMessage(&data);

    // CAPA 3: puente de emergencia (opcional)
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

    std::string systemPrompt = "Eres un NPC de un servidor privado de World of Warcraft 3.3.5. ";
    systemPrompt += "Responde de forma breve, en personaje y en el idioma del jugador. ";
    systemPrompt += "No uses comillas, ni markdown, ni emojis.";

    std::string body;
    body += "{";
    body += "\"model\":\"" + NpcBotLLMInterface::SanitizeForJson(model) + "\",";
    body += "\"max_tokens\":" + std::to_string(maxTokens) + ",";
    body += "\"messages\":[";
    body += "{\"role\":\"system\",\"content\":\"" + NpcBotLLMInterface::SanitizeForJson(systemPrompt) + "\"},";
    body += "{\"role\":\"user\",\"content\":\"" + NpcBotLLMInterface::SanitizeForJson(prompt) + "\"}";
    body += "]}";
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
        // 1. Verificar que sea un mensaje de canal valido
        if (type != CHAT_MSG_CHANNEL || !channel)
            return;

        std::string channelName = channel->GetName();

        // 2. Comparacion case-insensitive contra la lista del conf
        std::string lower = channelName;
        for (size_t i = 0; i < lower.size(); ++i)
            lower[i] = (char)tolower((unsigned char)lower[i]);

        std::string channelsConf = sConfigMgr->GetStringDefault("NpcBot.LLM.Channels", "world,mundo,general");
        bool allowed = false;
        size_t pos = 0;
        while (pos <= channelsConf.size())
        {
            size_t comma = channelsConf.find(',', pos);
            std::string token = channelsConf.substr(pos, (comma == std::string::npos ? channelsConf.size() : comma) - pos);
            size_t b = token.find_first_not_of(" \t");
            size_t e = token.find_last_not_of(" \t");
            if (b != std::string::npos)
            {
                token = token.substr(b, e - b + 1);
                for (size_t i = 0; i < token.size(); ++i)
                    token[i] = (char)tolower((unsigned char)token[i]);
                if (token == lower) { allowed = true; break; }
            }
            if (comma == std::string::npos) break;
            pos = comma + 1;
        }
        if (!allowed)
            return;

        BOT_LOG_INFO("npcbots", "LLM: mensaje detectado de '{}' en canal '{}'.", player->GetName(), channelName);

        // 3. Verificar si el sistema está habilitado
        if (!sConfigMgr->GetBoolDefault("NpcBot.LLM.Enable", false))
        {
            BOT_LOG_INFO("npcbots", "LLM: desactivado por configuracion (NpcBot.LLM.Enable = 0).");
            return;
        }

        // 4. Recoger todos los NPCBots activos del mundo
        std::vector<Creature*> availableBots;
        NpcBotRegistry const& registry = BotDataMgr::GetExistingNPCBots();
        for (NpcBotRegistry::const_iterator itr = registry.begin(); itr != registry.end(); ++itr)
        {
            Creature* bot = const_cast<Creature*>(*itr);
            if (!bot || !bot->IsInWorld() || !bot->IsAlive())
                continue;
            availableBots.push_back(bot);
        }

        BOT_LOG_INFO("npcbots", "LLM: bots disponibles para responder: {}", (uint32)availableBots.size());

        if (availableBots.empty())
        {
            BOT_LOG_INFO("npcbots", "LLM: no hay bots disponibles para responder.");
            return;
        }

        // ===== CADENA: varios bots responden en fila al jugador =====
        std::random_device rd;
        std::mt19937 gen(rd());
        std::shuffle(availableBots.begin(), availableBots.end(), gen);

        uint8 minT = uint8(sConfigMgr->GetIntDefault("NpcBot.LLM.PlayerChain.MinTurns", 2));
        uint8 maxT = uint8(sConfigMgr->GetIntDefault("NpcBot.LLM.PlayerChain.MaxTurns", 3));
        if (maxT < minT) maxT = minT;
        uint8 turns = uint8(urand(minT, maxT));
        if (!sConfigMgr->GetBoolDefault("NpcBot.LLM.PlayerChain.Enable", true))
            turns = 1;
        if (turns > availableBots.size())
            turns = uint8(availableBots.size());

        ChainTurn turn;
        turn.botGuid     = availableBots[0]->GetGUID();
        turn.seedText    = msg;
        turn.seedAuthor  = player->GetName();
        turn.playerName  = player->GetName();
        turn.turnsLeft   = uint8(turns - 1);
        turn.channelName = channelName;
        for (uint8 i = 1; i < turns; ++i)
            turn.nextBots.push_back(availableBots[i]->GetGUID());

        {
            std::lock_guard<std::mutex> lock(g_chainMutex);
            g_playerChains.push(turn);
        }

        BOT_LOG_INFO("npcbots", "LLM: cadena de {} respuesta(s) preparada para '{}'.", turns, player->GetName());
    }
};

// ---------------------------------------------------------------------------
// WorldScript: procesa la cola de respuestas pendientes en el hilo del mundo
// ---------------------------------------------------------------------------
class npcbot_llm_chat_world_script : public WorldScript
{
public:
    npcbot_llm_chat_world_script() : WorldScript("npcbot_llm_chat_world_script") { }

    void OnUpdate(uint32 diff) override
    {
		
		        // ================= CADENA DE RESPUESTAS A JUGADORES =================
        {
            bool hasChain;
            {
                std::lock_guard<std::mutex> lock(g_chainMutex);
                hasChain = !g_playerChains.empty();
            }

            if (hasChain)
            {
                ChainTurn turn;
                {
                    std::lock_guard<std::mutex> lock(g_chainMutex);
                    turn = g_playerChains.front();
                    g_playerChains.pop();
                }

                Creature* speaker = FindBotByGuid(turn.botGuid);
                if (speaker && speaker->IsInWorld() && speaker->IsAlive())
                {
                    struct ChainData
                    {
                        ObjectGuid botGuid;
                        std::string botName;
                        std::string seedText;
                        std::string seedAuthor;
                        std::string playerName;
                        std::vector<ObjectGuid> nextBots;
                        uint8 turnsLeft;
                    };

                    ChainData* cd = new ChainData();
                    cd->botGuid   = speaker->GetGUID();
                    cd->botName   = speaker->GetName();
                    cd->seedText  = turn.seedText;
                    cd->seedAuthor= turn.seedAuthor;
                    cd->playerName= turn.playerName;
                    cd->nextBots  = turn.nextBots;
                    cd->turnsLeft = turn.turnsLeft;

                    std::thread([channelName = turn.channelName](ChainData* d)
                    {
                        try
                        {
                            std::string prompt = "Eres un bot NPC llamado '" + d->botName + "' en un servidor privado de World of Warcraft 3.3.5a. ";
                            prompt += "A player named '" + d->playerName + "' is chatting in the '" + channelName + "' channel. ";
                            prompt += "El último mensaje de la conversación fue de '" + d->seedAuthor + "': \"" + d->seedText + "\". ";
                            prompt += "Responde brevemente (máx. 100 caracteres), interpretando a tu personaje y en el mismo idioma que el jugador. No uses comillas ni formato Markdown.";

                            std::vector<std::string> debugLines;
                            int timeout = sConfigMgr->GetIntDefault("NpcBot.LLM.TimeoutSeconds", 15);
                            int maxGen  = sConfigMgr->GetIntDefault("NpcBot.LLM.MaxGenerations", 3);

                            std::string body = BuildLLMRequestBody(prompt);
                            std::string response = NpcBotLLMInterface::Generate(body, timeout, maxGen, debugLines);

                            std::string reply;
                            if (!response.empty() && response.find("\"error\"") == std::string::npos)
                                reply = ExtractOpenAIResponse(response);

                            // Failover a BD si la IA fallo
                            if (reply.empty() && !g_phrasesResponse.empty())
                                reply = g_phrasesResponse[urand(0, (int)g_phrasesResponse.size() - 1)];

                            if (!reply.empty())
                            {
                                {
                                    std::lock_guard<std::mutex> lock(g_pendingBotChatsMutex);
                                    PendingBotChat pending;
                                    pending.botGuid = d->botGuid;
                                    pending.reply = reply;
                                    pending.channelName = channelName;
                                    g_pendingBotChats.push(pending);
                                }

                                // Encolar el siguiente bot de la cadena
                                if (d->turnsLeft > 0 && !d->nextBots.empty())
                                {
                                    std::lock_guard<std::mutex> lock(g_chainMutex);
                                    ChainTurn next;
                                    next.botGuid    = d->nextBots.front();
                                    next.seedText   = reply;
                                    next.seedAuthor = d->botName;
                                    next.playerName = d->playerName;
									next.channelName = channelName;
                                    next.turnsLeft  = uint8(d->turnsLeft - 1);
                                    for (size_t i = 1; i < d->nextBots.size(); ++i)
                                        next.nextBots.push_back(d->nextBots[i]);
                                    g_playerChains.push(next);
                                }
                            }
                            delete d;
                        }
                        catch (...)
                        {
                            delete d;
                        }
                    }, cd).detach();
                }
                else if (turn.turnsLeft > 0 && !turn.nextBots.empty())
                {
                    // El bot murio antes de hablar: pasa el turno al siguiente de la fila
                    std::lock_guard<std::mutex> lock(g_chainMutex);
                    ChainTurn next;
                    next.botGuid    = turn.nextBots.front();
                    next.seedText   = turn.seedText;
                    next.seedAuthor = turn.seedAuthor;
                    next.playerName = turn.playerName;
					next.channelName = turn.channelName;
                    next.turnsLeft  = turn.turnsLeft;
                    for (size_t i = 1; i < turn.nextBots.size(); ++i)
                        next.nextBots.push_back(turn.nextBots[i]);
                    g_playerChains.push(next);
                }
            }
        }
        // ================= FIN CADENA =================
		
        // ================= DIRECTOR BOT-A-BOT =================
        if (sConfigMgr->GetBoolDefault("NpcBot.LLM.BotToBot.Enable", false))
        {	
			std::string botChannel = sConfigMgr->GetStringDefault("NpcBot.LLM.BotToBot.Channel", "world");
            if (!g_phrasesLoaded)
                LoadPhrasesFromDB();

            bool playerQueueEmpty;
            {
                std::lock_guard<std::mutex> lock(g_pendingBotChatsMutex);
                playerQueueEmpty = g_pendingBotChats.empty();
            }

            g_botToBotTimer += diff;
            uint32 cooldownMs = uint32(sConfigMgr->GetIntDefault("NpcBot.LLM.BotToBot.CooldownSec", 150)) * 1000u;

            // --- Iniciar conversacion nueva (solo si no hay jugadores esperando respuesta) ---
                        bool chainEmpty;
            {
                std::lock_guard<std::mutex> lock(g_chainMutex);
                chainEmpty = g_playerChains.empty();
            }

            if (playerQueueEmpty && chainEmpty && g_botToBotTimer >= cooldownMs)
            {
                g_botToBotTimer = 0;
                uint32 chance = uint32(sConfigMgr->GetIntDefault("NpcBot.LLM.BotToBot.Chance", 25));

                bool convActive;
                {
                    std::lock_guard<std::mutex> lock(g_conversationMutex);
                    convActive = !g_activeConversations.empty();
                }

                if (!convActive && !g_phrasesStart.empty() && urand(1, 100) <= chance)
                {
                    std::vector<Creature*> bots;
                    NpcBotRegistry const& registry = BotDataMgr::GetExistingNPCBots();
                    for (NpcBotRegistry::const_iterator itr = registry.begin(); itr != registry.end(); ++itr)
                    {
                        Creature* bot = const_cast<Creature*>(*itr);
                        if (bot && bot->IsInWorld() && bot->IsAlive())
                            bots.push_back(bot);
                    }

                    if (bots.size() >= 2)
                    {
                        std::random_device rd;
                        std::mt19937 gen(rd());
                        std::shuffle(bots.begin(), bots.end(), gen);

                        Creature* botA = bots[0];
                        Creature* botB = bots[1];
                        std::string seed = g_phrasesStart[urand(0, (int)g_phrasesStart.size() - 1)];

                        // Bot A abre la charla (gratis, sin IA)
                        SendBotChannelMessage(botA, botChannel, seed);

                        uint8 minT = uint8(sConfigMgr->GetIntDefault("NpcBot.LLM.BotToBot.MinTurns", 2));
                        uint8 maxT = uint8(sConfigMgr->GetIntDefault("NpcBot.LLM.BotToBot.MaxTurns", 3));
                        if (maxT < minT) maxT = minT;

                        BotConversation conv;
                        conv.botA = botA->GetGUID();
                        conv.botB = botB->GetGUID();
                        conv.seedPhrase = seed;
                        conv.turnsLeft = uint8(urand(minT, maxT));
                        conv.nextIsBotA = false;   // responde B primero

                        std::lock_guard<std::mutex> lock(g_conversationMutex);
                        g_activeConversations.push(conv);

                        BOT_LOG_INFO("npcbots", "LLM: charla bot-a-bot iniciada: '{}' abre y '{}' respondera.",
                                     botA->GetName(), botB->GetName());
                    }
                }
            }

            // --- Procesar siguiente turno de la conversacion activa ---
            bool convActive;
            {
                std::lock_guard<std::mutex> lock(g_conversationMutex);
                convActive = !g_activeConversations.empty();
            }

            if (playerQueueEmpty && chainEmpty && convActive)
            {
                BotConversation conv;
                conv.turnsLeft = 0;
                {
                    std::lock_guard<std::mutex> lock(g_conversationMutex);
                    if (!g_activeConversations.empty())
                    {
                        conv = g_activeConversations.front();
                        g_activeConversations.pop();
                    }
                }

                if (conv.turnsLeft > 0)
                {
                    Creature* speaker = FindBotByGuid(conv.nextIsBotA ? conv.botA : conv.botB);
                    Creature* other   = FindBotByGuid(conv.nextIsBotA ? conv.botB : conv.botA);

                    if (speaker && other && speaker->IsInWorld() && speaker->IsAlive() && other->IsInWorld() && other->IsAlive())
                    {
                        struct ConvData
                        {
                            ObjectGuid speakerGuid;
                            std::string speakerName;
                            std::string otherName;
                            std::string seed;
                            ObjectGuid botA;
                            ObjectGuid botB;
                            bool nextIsBotA;
                            uint8 turnsLeft;
                        };

                        ConvData* cd = new ConvData();
                        cd->speakerGuid = speaker->GetGUID();
                        cd->speakerName = speaker->GetName();
                        cd->otherName   = other->GetName();
                        cd->seed        = conv.seedPhrase;
                        cd->botA        = conv.botA;
                        cd->botB        = conv.botB;
                        cd->nextIsBotA  = conv.nextIsBotA;
                        cd->turnsLeft   = conv.turnsLeft;

                        std::thread([botChannel](ConvData* d)
                        {
                            try
                            {
                                std::string prompt = "You are an NPC bot named '" + d->speakerName + "' in a World of Warcraft 3.3.5a private server. ";
                                prompt += "Another NPC named '" + d->otherName + "' just said in the '" + botChannel + "' channel: \"" + d->seed + "\". ";
                                prompt += "Reply briefly (max 100 characters), in character, in the same language as the other NPC. Do not use quotes or markdown.";

                                std::vector<std::string> debugLines;
                                int timeout = sConfigMgr->GetIntDefault("NpcBot.LLM.TimeoutSeconds", 15);
                                int maxGen  = sConfigMgr->GetIntDefault("NpcBot.LLM.MaxGenerations", 3);

                                std::string body = BuildLLMRequestBody(prompt);
                                std::string response = NpcBotLLMInterface::Generate(body, timeout, maxGen, debugLines);

                                std::string reply;
                                if (!response.empty() && response.find("\"error\"") == std::string::npos)
                                    reply = ExtractOpenAIResponse(response);

                                // Failover a frases de BD si la IA fallo
                                if (reply.empty() && !g_phrasesResponse.empty())
                                    reply = g_phrasesResponse[urand(0, (int)g_phrasesResponse.size() - 1)];

                                if (!reply.empty())
                                {
                                    {
                                        std::lock_guard<std::mutex> lock(g_pendingBotChatsMutex);
                                        PendingBotChat pending;
                                        pending.botGuid = d->speakerGuid;
                                        pending.reply = reply;
                                        pending.channelName = botChannel;
                                        g_pendingBotChats.push(pending);
                                    }

                                    // Siguiente turno: habla el OTRO bot
                                    if (d->turnsLeft > 1)
                                    {
                                        std::lock_guard<std::mutex> lock(g_conversationMutex);
                                        BotConversation next;
                                        next.botA = d->botA;
                                        next.botB = d->botB;
                                        next.seedPhrase = reply;
                                        next.turnsLeft = uint8(d->turnsLeft - 1);
                                        next.nextIsBotA = !d->nextIsBotA;
                                        g_activeConversations.push(next);
                                    }
                                }
                                delete d;
                            }
                            catch (...)
                            {
                                delete d;
                            }
                        }, cd).detach();
                    }
                }
            }
        }
        // ================= FIN DIRECTOR =================
        // Procesar cola de respuestas pendientes (código existente)
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
TC_GAME_API void AddSC_npcbot_llm_chat()
{
    new npcbot_llm_chat_player_script();
    new npcbot_llm_chat_world_script();

    BOT_LOG_INFO("npcbots", "LLM: scripts de chat registrados correctamente.");
}



