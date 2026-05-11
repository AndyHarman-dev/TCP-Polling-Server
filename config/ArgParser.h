#pragma once
#include <string>
#include <unordered_map>
#include "config/Config.h"

class ArgParser {
public:
    ArgParser(int argc, char* argv[]);

    // Overlays parsed CLI values onto cfg.
    // Throws std::invalid_argument if the required port argument is absent.
    void apply(Config& cfg) const;

private:
    std::unordered_map<std::string, std::string> args_;
};
