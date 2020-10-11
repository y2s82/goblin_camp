#pragma once

#include <string>
#include <cctype>

namespace utils {
    inline void to_upper(std::string &str) {
        std::transform(str.begin(), str.end(), str.begin(), ::toupper);
    }
    inline void to_lower(std::string &str) {
        std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    }
    inline std::string to_lower_copy(std::string str) {
        to_lower(str);
        return str;
    }
    inline bool iequals(std::string a, std::string b) {
        to_lower(a);
        to_lower(b);
        return a == b;
    }
}

