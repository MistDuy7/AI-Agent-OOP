#include "skillLoader.h"

#include <filesystem>
#include <fstream>
#include <sstream>

using namespace std;
namespace fs = filesystem;

SkillLoader::SkillLoader(const string& directory)
    : directory(directory) {
}

string SkillLoader::loadAll() const {
    stringstream result;

    if (!fs::exists(directory)) {
        return "No skills found.";
    }

    for (const auto& entry : fs::directory_iterator(directory)) {
        if (entry.path().extension() != ".md") {
            continue;
        }

        ifstream file(entry.path());

        if (file.is_open()) {
            result << "\n--- "
                   << entry.path().filename().string()
                   << " ---\n";

            result << file.rdbuf() << "\n";
        }
    }

    if (result.str().empty()) {
        return "No skills found.";
    }

    return result.str();
}