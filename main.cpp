#include <iostream>
#include <string>
#include <cstdlib>
#include <exception>

#include <curl/curl.h>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include "client/OllamaClient.h"
#include "tool/tool.h"
#include "skillLoader/skillLoader.h"
#include "agent/agent.h"
#include "harness/harness.h"

using namespace std;

// Đọc một dòng từ bàn phím và trả về chuỗi UTF-8 hợp lệ.
// Windows: đọc bằng ReadConsoleW (Unicode) rồi chuyển sang UTF-8.
// Linux/macOS: terminal đã là UTF-8 nên dùng getline bình thường.
static bool readLineUtf8(string& out) {
#ifdef _WIN32
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode = 0;

    // Chỉ dùng ReadConsoleW khi input là console thật (không phải pipe/file)
    if (GetConsoleMode(hIn, &mode)) {
        wstring line;
        wchar_t buf[256];
        DWORD n = 0;

        while (true) {
            if (!ReadConsoleW(hIn, buf, 256, &n, nullptr) || n == 0) {
                return false;
            }

            line.append(buf, n);

            if (line.back() == L'\n') {
                break;
            }
        }

        // Bỏ \r\n ở cuối dòng
        while (!line.empty() &&
               (line.back() == L'\n' || line.back() == L'\r')) {
            line.pop_back();
        }

        // Ctrl+Z rồi Enter -> coi như kết thúc input
        if (line == L"\x1a") {
            return false;
        }

        out.clear();

        if (line.empty()) {
            return true;
        }

        int len = WideCharToMultiByte(
            CP_UTF8, 0,
            line.c_str(), static_cast<int>(line.size()),
            nullptr, 0, nullptr, nullptr
        );

        out.assign(len, '\0');

        WideCharToMultiByte(
            CP_UTF8, 0,
            line.c_str(), static_cast<int>(line.size()),
            out.data(), len, nullptr, nullptr
        );

        return true;
    }
#endif

    return static_cast<bool>(getline(cin, out));
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    // In tiếng Việt (UTF-8) ra console đúng dấu
    SetConsoleOutputCP(CP_UTF8);
#endif

    curl_global_init(CURL_GLOBAL_DEFAULT);

    const char* url = getenv("OLLAMA_BASE_URL");

    if (url == nullptr || string(url).empty()) {
        cerr << "Error: OLLAMA_BASE_URL is not set.\n";
        curl_global_cleanup();
        return 1;
    }

    try {
        const char* modelEnv = getenv("OLLAMA_MODEL");

        string model = "qwen3.8:27b";

        if (modelEnv != nullptr && string(modelEnv).length() > 0) {
            model = modelEnv;
        }

        // 1. Khởi tạo Ollama client (temperature, max tokens, timeout giây)
        OllamaClient client(url, model, 0.2, 1024, 180);

        // 2. Khởi tạo tool registry
        ToolRegistry tools;

        // 3. Đọc các file kỹ năng trong thư mục skills
        SkillLoader skills("skills");

        // 4. Kết nối các thành phần thành Agent
        Agent agent(client, tools, skills);

        // 5. Chạy kiểm thử nếu có tham số --test
        if (argc > 1 && string(argv[1]) == "--test") {
            Harness harness(agent);
            harness.runSmokeTest();
        }
        else {
            cout << "C++ AI Agent\n";
            cout << "Model: " << model << '\n';
            cout << "Type 'exit' to quit.\n";

            string input;

            while (true) {
                cout << "\nYou> " << flush;

                if (!readLineUtf8(input)) {
                    break;
                }

                if (input == "exit" || input == "quit") {
                    break;
                }

                if (input.empty()) {
                    continue;
                }

                try {
                    string answer = agent.run(input);
                    cout << "Agent> " << answer << '\n';
                }
                catch (const exception& e) {
                    cerr << "Request error: " << e.what() << '\n';
                }
            }
        }
    }
    catch (const exception& e) {
        cerr << "Fatal error: " << e.what() << '\n';
        curl_global_cleanup();
        return 1;
    }

    curl_global_cleanup();
    return 0;
}