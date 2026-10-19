#pragma once

#include <string>
#include <map>
#include <functional>
#include <nlohmann/json.hpp>

class ToolRegistry {
public:
    using Tool = std::function<std::string(const nlohmann::json&)>;

    ToolRegistry();

    void registerTool(const std::string& name, Tool tool);

    bool hasTool(const std::string& name) const;

    std::string execute(
        const std::string& name,
        const nlohmann::json& args
    ) const;

    std::string describeTools() const;

private:
    std::map<std::string, Tool> tools_;
};