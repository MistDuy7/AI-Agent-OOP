#include "tool.h"

using namespace std;
using json = nlohmann::json;

ToolRegistry::ToolRegistry() {
    registerTool("add", [](const json& args) {
        double a = args.at("a").get<double>();
        double b = args.at("b").get<double>();

        return to_string(a + b);
    });

    registerTool("multiply", [](const json& args) {
        double a = args.at("a").get<double>();
        double b = args.at("b").get<double>();

        return to_string(a * b);
    });
}

void ToolRegistry::registerTool(
    const string& name,
    Tool tool
) {
    tools_[name] = tool;
}

bool ToolRegistry::hasTool(const string& name) const {
    return tools_.find(name) != tools_.end();
}

string ToolRegistry::execute(
    const string& name,
    const json& args
) const {
    auto it = tools_.find(name);

    if (it == tools_.end()) {
        return "Error: tool not found";
    }

    try {
        return it->second(args);
    }
    catch (const exception& e) {
        return string("Tool error: ") + e.what();
    }
}

string ToolRegistry::describeTools() const {
    return
        "Available tools:\n"
        "- add: arguments {\"a\": number, \"b\": number}\n"
        "- multiply: arguments {\"a\": number, \"b\": number}\n";
}