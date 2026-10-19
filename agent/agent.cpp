#include "agent.h"
#include <exception>

using json = nlohmann::json;

// Bỏ <think>...</think>, code fence ``` và lấy khối JSON {...} đầu tiên
static std::string extractJson(std::string text) {
    // 1. Xoá phần suy nghĩ của Qwen3 (nếu có)
    size_t thinkEnd = text.find("</think>");
    if (thinkEnd != std::string::npos) {
        text = text.substr(thinkEnd + 8);
    }

    // 2. Lấy từ dấu '{' đầu tiên đến dấu '}' cuối cùng
    size_t start = text.find('{');
    size_t end = text.rfind('}');

    if (start == std::string::npos || end == std::string::npos || end < start) {
        return text;
    }

    return text.substr(start, end - start + 1);
}

// Chuyển giá trị JSON bất kỳ thành chuỗi để in ra
static std::string toText(const json& value) {
    if (value.is_string()) {
        return value.get<std::string>();
    }
    return value.dump();
}

Agent::Agent(OllamaClient& client,
             ToolRegistry& tools,
             const SkillLoader& skills)
    : client(client), tools(tools), skills(skills) {

    std::string systemPrompt =
        "You are a helpful C++ AI Agent.\n"
        "Use tools when appropriate.\n" +
        tools.describeTools() +   // đã có sẵn tiêu đề "Available tools:"
        "\nSkills:\n" + skills.loadAll() +
        "\nRespond with ONE JSON object only. No markdown, no extra text.\n"
        "For a final answer, use: "
        "{\"type\":\"final\",\"content\":\"answer\"}\n"
        "For a tool call, use: "
        "{\"type\":\"tool_call\",\"name\":\"add\","
        "\"arguments\":{\"a\":2,\"b\":3}}\n";

    messages = json::array();
    messages.push_back({
        {"role", "system"},
        {"content", systemPrompt}
    });
}

std::string Agent::run(const std::string& userInput) {
    messages.push_back({
        {"role", "user"},
        {"content", userInput}
    });

    // Giới hạn số vòng lặp để tránh gọi tool vô hạn.
    for (int i = 0; i < 5; i++) {
        // SỬA LỖI CHÍNH: OllamaClient::chat đã trả về message.content (string)
        std::string content = client.chat(messages);

        // Luôn lưu câu trả lời của model vào lịch sử
        messages.push_back({
            {"role", "assistant"},
            {"content", content}
        });

        json result;

        try {
            result = json::parse(extractJson(content));
        }
        catch (const json::exception&) {
            // Model trả văn bản thường -> coi như câu trả lời cuối
            return content;
        }

        if (!result.is_object() || !result.contains("type")) {
            return content;
        }

        std::string type = toText(result["type"]);

        if (type == "final") {
            if (result.contains("content")) {
                return toText(result["content"]);
            }
            return content;
        }

        if (type == "tool_call") {
            std::string name = result.contains("name")
                ? toText(result["name"])
                : "";

            json args = result.contains("arguments") &&
                        result["arguments"].is_object()
                ? result["arguments"]
                : json::object();

            std::string toolResult;

            if (!tools.hasTool(name)) {
                toolResult = "Error: tool '" + name + "' does not exist.";
            }
            else {
                toolResult = tools.execute(name, args);
            }

            messages.push_back({
                {"role", "user"},
                {"content",
                    "Tool result (" + name + "): " + toolResult +
                    "\nNow answer the original question using the "
                    "final JSON format."}
            });

            continue;
        }

        return content;
    }

    return "Agent stopped: maximum tool-call rounds reached.";
}