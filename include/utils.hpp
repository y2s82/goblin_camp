#pragma once

#include <string>
#include <cctype>

namespace utils {
    static bool iequals(std::string a, std::string b) {
        std::transform(a.begin(), a.end(), a.begin(), ::tolower);
        std::transform(b.begin(), b.end(), b.begin(), ::tolower);
        return a == b;
    }
}

