#include "util/duration.h"
#include <charconv>
#include <stdexcept>

std::chrono::seconds util::parse_duration(const std::string& s) {
    if (s.empty()) throw std::invalid_argument("empty duration");
    char suffix = s.back();
    long multiplier = 0;
    if      (suffix == 's') multiplier = 1;
    else if (suffix == 'm') multiplier = 60;
    else if (suffix == 'h') multiplier = 3600;
    else throw std::invalid_argument("duration suffix must be s, m, or h");

    long value = 0;
    auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size() - 1, value);
    if (ec != std::errc{} || ptr != s.data() + s.size() - 1 || value <= 0)
        throw std::invalid_argument("invalid duration value: " + s);

    if (value > std::numeric_limits<long>::max() / multiplier)
        throw std::invalid_argument("duration overflow: " + s);

    return std::chrono::seconds(value * multiplier);
}
