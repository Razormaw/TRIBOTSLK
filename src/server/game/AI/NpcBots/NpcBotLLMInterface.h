#ifndef _NPCBOT_LLM_INTERFACE_H
#define _NPCBOT_LLM_INTERFACE_H

#include <string>
#include <vector>
#include <atomic>
#include <cstdint>

struct ParsedUrl {
    bool https = false;
    std::string hostname;
    uint16_t port = 80;
    std::string path = "/";
};

class NpcBotLLMInterface
{
public:
    NpcBotLLMInterface() = default;
    ~NpcBotLLMInterface() = default;

    // Sanitiza la entrada para que sea válida en un payload JSON
    static std::string SanitizeForJson(const std::string& input);

    // Envía la petición al LLM. ¡ADVERTENCIA! Esta función es bloqueante. 
    // DEBE llamarse desde un std::thread o std::async, NUNCA desde el hilo principal.
    static std::string Generate(const std::string& prompt, int timeOutSeconds, int maxGenerations, std::vector<std::string>& debugLines);

    // Limpia la respuesta del LLM (elimina patrones no deseados, comillas, etc.)
    static std::vector<std::string> ParseResponse(const std::string& response, const std::string& startPattern, const std::string& endPattern, const std::string& deletePattern, const std::string& splitPattern, std::vector<std::string>& debugLines);

    // Recorta el contexto para no exceder los límites de tokens del LLM
    static void LimitContext(std::string& context, uint32_t currentLength);

    // Utilidad para descomponer una URL (ej: "http://localhost:11434/api/generate")
    static ParsedUrl ParseUrl(const std::string& url);

private:
    static std::atomic<int> generationCount;
};

#endif // _NPCBOT_LLM_INTERFACE_H

