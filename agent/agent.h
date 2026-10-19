#pragma once

#include "../client/OllamaClient.h"
#include "../tool/tool.h"
#include "../skillLoader/skillLoader.h"
#include <nlohmann/json.hpp>
#include <string>

class Agent {
public:
    Agent(OllamaClient& client,
          ToolRegistry& tools,
          const SkillLoader& skills);

    std::string run(const std::string& userInput);

private:
    OllamaClient& client;
    ToolRegistry& tools;
    const SkillLoader& skills;
    nlohmann::json messages;
};