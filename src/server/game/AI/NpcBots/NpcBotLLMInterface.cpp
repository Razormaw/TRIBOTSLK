#include "NpcBotLLMInterface.h"
#include "Config.h"  // ← Cambiado de "Config/Config.h"
#include "botlog.h"
#include "Log.h"     // ← Cambiado de "Log/Log.h"
#include <sstream>
#include <chrono>
#include <thread>
#include <regex>     // ← Agregado (faltaba)

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#endif

std::atomic<int> NpcBotLLMInterface::generationCount(0);

ParsedUrl NpcBotLLMInterface::ParseUrl(const std::string& url) {
    ParsedUrl result;
    std::string temp = url;

    if (temp.find("https://") == 0) {
        result.https = true;
        result.port = 443;
        temp = temp.substr(8);
    } else if (temp.find("http://") == 0) {
        result.https = false;
        result.port = 80;
        temp = temp.substr(7);
    }

    size_t pathPos = temp.find('/');
    if (pathPos != std::string::npos) {
        result.path = temp.substr(pathPos);
        temp = temp.substr(0, pathPos);
    } else {
        result.path = "/";
    }

    size_t portPos = temp.find(':');
    if (portPos != std::string::npos) {
        result.hostname = temp.substr(0, portPos);
        result.port = static_cast<uint16_t>(std::stoi(temp.substr(portPos + 1)));
    } else {
        result.hostname = temp;
    }
    return result;
}

std::string NpcBotLLMInterface::SanitizeForJson(const std::string& input) {
    std::string sanitized;
    sanitized.reserve(input.size());
    for (unsigned char c : input) {
        switch (c) {
            case '"': sanitized += "\\\""; break;
            case '\\': sanitized += "\\\\"; break;
            case '\b': sanitized += "\\b"; break;
            case '\f': sanitized += "\\f"; break;
            case '\n': sanitized += "\\n"; break;
            case '\r': sanitized += "\\r"; break;
            case '\t': sanitized += "\\t"; break;
            default:
                if (c < 0x20) {
                    char buffer[7];
                    snprintf(buffer, sizeof(buffer), "\\u%04x", c);
                    sanitized += buffer;
                } else {
                    sanitized += static_cast<char>(c);
                }
        }
    }
    return sanitized;
}

std::string NpcBotLLMInterface::Generate(const std::string& prompt, int timeOutSeconds, int maxGenerations, std::vector<std::string>& debugLines) {
    bool debug = !debugLines.empty();
    if (generationCount.load() >= maxGenerations) {
        if (debug) debugLines.push_back("LLM: Max generations reached.");
        return "";
    }
    generationCount++;

#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

    std::string endpoint = sConfigMgr->GetStringDefault("NpcBot.LLM.Endpoint", "http://localhost:11434/api/generate");
    std::string apiKey = sConfigMgr->GetStringDefault("NpcBot.LLM.ApiKey", "");
    ParsedUrl url = ParseUrl(endpoint);

    struct addrinfo hints = {}, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(url.hostname.c_str(), std::to_string(url.port).c_str(), &hints, &res) != 0) {
        BOT_LOG_ERROR("npcbots", "LLM: no se pudo resolver el host {}.", url.hostname);
        generationCount--;
        return "error_resolve";
    }

    int sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (
#ifdef _WIN32
        sock == INVALID_SOCKET
#else
        sock < 0
#endif
    ) {
        BOT_LOG_ERROR("npcbots", "LLM: fallo al crear el socket.");
        freeaddrinfo(res);
        generationCount--;
        return "error_socket";
    }

    if (connect(sock, res->ai_addr, res->ai_addrlen) < 0) {
        BOT_LOG_ERROR("npcbots", "LLM: fallo de conexion con el endpoint.");
#ifdef _WIN32
        closesocket(sock);
#else
        close(sock);
#endif
        freeaddrinfo(res);
        generationCount--;
        return "error_connect";
    }
    freeaddrinfo(res);

    SSL_CTX* ctx = nullptr;
    SSL* ssl = nullptr;
    if (url.https) {
        SSL_library_init();
        ctx = SSL_CTX_new(TLS_client_method());
        ssl = SSL_new(ctx);
        SSL_set_fd(ssl, sock);
	    SSL_set_tlsext_host_name(ssl, url.hostname.c_str()); // SNI: obligatorio para Cloudflare/Groq
        SSL_set_verify(ssl, SSL_VERIFY_NONE, nullptr);       // sin verificacion de certificado (MVP)
        if (SSL_connect(ssl) <= 0) {
            BOT_LOG_ERROR("npcbots", "LLM: fallo de handshake SSL.");
            SSL_free(ssl); SSL_CTX_free(ctx);
#ifdef _WIN32
            closesocket(sock);
#else
            close(sock);
#endif
            generationCount--;
            return "error_ssl";
        }
    }

    std::ostringstream request;
    request << "POST " << url.path << " HTTP/1.1\r\n";
    request << "Host: " << url.hostname << "\r\n";
    request << "Content-Type: application/json\r\n";
    request << "Accept: application/json\r\n";
    request << "Connection: close\r\n";
    if (!apiKey.empty()) request << "Authorization: Bearer " << apiKey << "\r\n";
    request << "Content-Length: " << prompt.size() << "\r\n\r\n";
    request << prompt;

    std::string reqStr = request.str();
    if (url.https && ssl) SSL_write(ssl, reqStr.c_str(), reqStr.size());
    else send(sock, reqStr.c_str(), reqStr.size(), 0);

    // Configurar socket como no bloqueante para el timeout
#ifdef _WIN32
    u_long mode = 1;
    ioctlsocket(sock, FIONBIO, &mode);
#else
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
#endif

    char buffer[4096];
    std::string response;
    auto start = std::chrono::steady_clock::now();

    while (true) {
        int bytesRead = 0;
        if (url.https && ssl) bytesRead = SSL_read(ssl, buffer, sizeof(buffer) - 1);
        else bytesRead = recv(sock, buffer, sizeof(buffer) - 1, 0);

        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            response += buffer;
        } else {
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::seconds>(now - start).count() >= timeOutSeconds) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    if (url.https && ssl) {
        SSL_shutdown(ssl); SSL_free(ssl); SSL_CTX_free(ctx);
    }
#ifdef _WIN32
    closesocket(sock); WSACleanup();
#else
    close(sock);
#endif
    generationCount--;

    size_t pos = response.find("\r\n\r\n");
    if (pos != std::string::npos) return response.substr(pos + 4);
    return response;
}

std::vector<std::string> NpcBotLLMInterface::ParseResponse(const std::string& response, const std::string& startPattern, const std::string& endPattern, const std::string& deletePattern, const std::string& splitPattern, std::vector<std::string>& debugLines) 
{
    std::vector<std::string> result;
    std::string actual = response;

    if (!startPattern.empty()) {
        size_t start = actual.find(startPattern);
        if (start != std::string::npos) actual = actual.substr(start + startPattern.length());
    }
    if (!endPattern.empty()) {
        size_t end = actual.find(endPattern);
        if (end != std::string::npos) actual = actual.substr(0, end);
    }
    if (!deletePattern.empty()) {
        actual = std::regex_replace(actual, std::regex(deletePattern), "");
    }

    if (!splitPattern.empty()) {
        size_t pos = 0;
        while ((pos = actual.find(splitPattern)) != std::string::npos) {
            std::string token = actual.substr(0, pos);
            if (!token.empty()) result.push_back(token);
            actual.erase(0, pos + splitPattern.length());
        }
        if (!actual.empty()) result.push_back(actual);
    } else {
        if (!actual.empty()) result.push_back(actual);
    }
    return result;
}

void NpcBotLLMInterface::LimitContext(std::string& context, uint32_t currentLength) {
    uint32 maxLength = uint32(sConfigMgr->GetIntDefault("NpcBot.LLM.MaxContextLength", 2000));
    if (currentLength > maxLength && maxLength > 0) {
        uint32_t cutNeeded = currentLength - maxLength;
        if (cutNeeded >= context.size()) {
            context.clear();
        } else {
            uint32_t cutPos = cutNeeded;
            for (size_t i = cutNeeded; i < context.size(); ++i) {
                if (context[i] == ' ' || context[i] == '.') {
                    cutPos = i + 1;
                    break;
                }
            }
            context = context.substr(cutPos);
        }
    }
}

