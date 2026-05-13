#pragma once
#include <chrono>
#include <string>

namespace util {
    // Parses a duration string with a required suffix: s (seconds), m (minutes), h (hours).
    // Throws std::invalid_argument on bad input.
    std::chrono::seconds parse_duration(const std::string& s);
}
