#ifndef OLLAMA_CLIENT_H
#define OLLAMA_CLIENT_H

#include <string>
#include <nlohmann/json.hpp>

class OllamaClient {
private:
    std::string baseUrl;
    std::string model;
    double temperature;
    int maxTokens;
    int timeoutSeconds;

public:
    OllamaClient(
        const std::string& url,
        const std::string& modelName,
        double temp = 0.7,
        int maxTok = 500,
        int timeout = 30
    );

    std::string chat(const std::string& message);

    std::string chat(
        const std::string& message,
        const std::string& imageBase64
    );

    // Chat với toàn bộ lịch sử hội thoại
    std::string chat(const nlohmann::json& messages);
};

#endif
