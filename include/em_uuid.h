#include <string>
#include <random>

#include "em_defs.h"
#include "em_string.h"

const char* generateUuid(EmStringBase& uuid, 
                         size_t length,
                         const char* alphabet = "0123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz") {
    if (isNullOrEmpty(alphabet) || length == 0 || length > uuid.capacity()) {
        return "";
    }
    std::mt19937 rng;
    std::uniform_int_distribution<size_t> dist(0, strlen(alphabet)-1);
    for (size_t i = 0; i < length; ++i) {
        uuid.setAt(i, alphabet[dist(rng)]);
    }
    return uuid.c_str();
}

