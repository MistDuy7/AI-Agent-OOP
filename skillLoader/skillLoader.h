#pragma once

#include <string>

class SkillLoader {
public:
    explicit SkillLoader(const std::string& directory);

    std::string loadAll() const;

private:
    std::string directory;
};