#include "OllamaClient.h"

#include <iostream>
#include <string>
#include <stdexcept>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Nhận dữ liệu phản hồi từ server
static size_t WriteCallback(
    void* contents,
    size_t size,
    size_t nmemb,
    void* userp
) {
    size_t totalSize = size * nmemb;

    std::string* response =
        static_cast<std::string*>(userp);

    response->append(
        static_cast<char*>(contents),
        totalSize
    );

    return totalSize;
}

// Gửi request đến Ollama với lịch sử hội thoại JSON
static std::string SendChatRequest(
    const std::string& baseUrl,
    const std::string& model,
    double temperature,
    int maxTokens,
    int timeoutSeconds,
    const json& messages
) {
    CURL* curl = curl_easy_init();

    if (!curl) {
        throw std::runtime_error(
            "Cannot initialize CURL"
        );
    }

    struct curl_slist* headers = nullptr;

    try {
        // Tạo request
        json request;

        request["model"] = model;
        request["messages"] = messages;
        request["stream"] = false;
        request["format"] = "json";

        request["options"] = {
            {"temperature", temperature},
            {"num_predict", maxTokens}
        };

        std::string requestBody = request.dump(
    -1, ' ', false, json::error_handler_t::replace
        );
        std::string response;

        // Ghép URL API
        std::string url = baseUrl;

        while (!url.empty() && url.back() == '/') {
            url.pop_back();
        }

        url += "/api/chat";

        // Cấu hình HTTP header
        headers = curl_slist_append(
            headers,
            "Content-Type: application/json"
        );
        headers = curl_slist_append(
        headers,
        "ngrok-skip-browser-warning: true"
        );

        if (!headers) {
            throw std::runtime_error(
                "Cannot create HTTP headers"
            );
        }

        curl_easy_setopt(
            curl,
            CURLOPT_URL,
            url.c_str()
        );

        curl_easy_setopt(
            curl,
            CURLOPT_POST,
            1L
        );

        curl_easy_setopt(
            curl,
            CURLOPT_POSTFIELDS,
            requestBody.c_str()
        );

        curl_easy_setopt(
            curl,
            CURLOPT_POSTFIELDSIZE,
            static_cast<long>(requestBody.size())
        );

        curl_easy_setopt(
            curl,
            CURLOPT_TIMEOUT,
            static_cast<long>(timeoutSeconds)
        );

        curl_easy_setopt(
            curl,
            CURLOPT_WRITEFUNCTION,
            WriteCallback
        );

        curl_easy_setopt(
            curl,
            CURLOPT_WRITEDATA,
            &response
        );

        curl_easy_setopt(
            curl,
            CURLOPT_HTTPHEADER,
            headers
        );

        // Gửi request
        CURLcode result = curl_easy_perform(curl);

        if (result != CURLE_OK) {
            throw std::runtime_error(
                std::string("HTTP request failed: ") +
                curl_easy_strerror(result)
            );
        }

        // Kiểm tra HTTP status
        long statusCode = 0;

        curl_easy_getinfo(
            curl,
            CURLINFO_RESPONSE_CODE,
            &statusCode
        );

        if (statusCode < 200 || statusCode >= 300) {
            throw std::runtime_error(
                "Ollama returned HTTP status: " +
                std::to_string(statusCode) +
                "\nResponse: " + response
            );
        }

        // Phân tích JSON phản hồi
        json jsonResponse;

        try {
            jsonResponse = json::parse(response);
        }
        catch (const json::exception& e) {
            throw std::runtime_error(
                std::string("Invalid JSON response: ") +
                e.what() +
                "\nResponse: " + response
            );
        }

        // Kiểm tra cấu trúc phản hồi
        if (!jsonResponse.contains("message") ||
            !jsonResponse["message"].is_object()) {
            throw std::runtime_error(
                "Invalid Ollama response: missing message object"
            );
        }

        if (!jsonResponse["message"].contains("content") ||
            !jsonResponse["message"]["content"].is_string()) {
            throw std::runtime_error(
                "Invalid Ollama response: message.content is not a string"
            );
        }

        // Trả về nội dung do model sinh ra
        std::string content =
            jsonResponse["message"]["content"].get<std::string>();

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        return content;
    }
    catch (...) {
        if (headers != nullptr) {
            curl_slist_free_all(headers);
        }

        curl_easy_cleanup(curl);

        throw;
    }
}

// Constructor
OllamaClient::OllamaClient(
    const std::string& url,
    const std::string& modelName,
    double temp,
    int maxTok,
    int timeout
) {
    baseUrl = url;
    model = modelName;
    temperature = temp;
    maxTokens = maxTok;
    timeoutSeconds = timeout;
}

// Chat chỉ có văn bản
std::string OllamaClient::chat(
    const std::string& message
) {
    return chat(message, "");
}

// Chat có văn bản và ảnh Base64
std::string OllamaClient::chat(
    const std::string& message,
    const std::string& imageBase64
) {
    json messages = json::array();

    json userMessage;

    userMessage["role"] = "user";
    userMessage["content"] = message;

    if (!imageBase64.empty()) {
        userMessage["images"] = json::array({
            imageBase64
        });
    }

    messages.push_back(userMessage);

    return SendChatRequest(
        baseUrl,
        model,
        temperature,
        maxTokens,
        timeoutSeconds,
        messages
    );
}

// Chat với toàn bộ lịch sử hội thoại từ Agent
std::string OllamaClient::chat(
    const nlohmann::json& messages
) {
    if (!messages.is_array()) {
        throw std::runtime_error(
            "Chat messages must be a JSON array"
        );
    }

    return SendChatRequest(
        baseUrl,
        model,
        temperature,
        maxTokens,
        timeoutSeconds,
        messages
    );
}

