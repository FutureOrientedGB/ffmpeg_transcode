// self
#include "string_utils.hpp"



bool startswith(const std::string &str, const std::string &prefix) {
    if (str.length() < prefix.length()) {
        return false;
    }
    return 0 == str.compare(0, prefix.length(), prefix);
}


bool endswith(const std::string &str, const std::string &suffix) {
    if (str.length() < suffix.length()) {
        return false;
    }
    return 0 == str.compare(str.length() - suffix.length(), suffix.length(), suffix);
}

