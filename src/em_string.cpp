#include "em_string.h"

const char* EmStringBase::format(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    format(fmt, args);
    va_end(args);
    return m_buf;
}

const char* EmStringBase::format(const char* fmt, va_list args) {
    vsnprintf(m_buf, m_capacity, fmt, args);
    return m_buf;
}

EmStrResult EmStringBase::append(const char* data, size_t len, bool allowPartial) {
    // Nothing to append?
    if (!data || len == 0) {
        return EmStrResult::success;
    }
    // Already full string
    if (isFull()) {
        return EmStrResult::failure;
    }
    // Partial append?
    const size_t left = spaceLeft();
    if (!allowPartial && left < len) {
        return EmStrResult::failure;
    }
    // Do the append
    strncat(m_buf, data, len);
    m_buf[capacity()] = '\0';
    return len > left ? EmStrResult::partial : EmStrResult::success;
}

EmStrResult EmStringBase::appendFormat(bool allowPartial, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    EmStrResult res = appendFormat(allowPartial, fmt, args);
    va_end(args);
    return res;
}

EmStrResult EmStringBase::appendFormat(bool allowPartial, const char* fmt, va_list args) {
    if (isFull()) {
        return EmStrResult::failure;
    }
    const size_t len = length();
    const size_t left = spaceLeft();
    const int w = vsnprintf(&m_buf[len], left+1, fmt, args);
    if (w < 0) {
        return EmStrResult::failure;
    }
    if (w > left) {
        if (allowPartial) {
            return EmStrResult::partial;
        }
        // Restore previous string length
        m_buf[len] = '\0';
        return EmStrResult::failure;
    }
    return EmStrResult::success;
}

bool EmStringBase::toInt(int32_t& value, bool strictParse) const {
    // NOTES: 
    //  - No 'atoi' (unsafe, no error reporting)
    //  - No 'strtol' (heavy on embedded, locale‑dependent)
    // Skip leading whitespace
    char* str = m_buf;

    // Skip leading whitespace (standard spaces)
    while (*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r') {
        str++;
    }

    // Optional sign
    bool negative = false;
    if (*str == '-') {
        negative = true;
        str++;
    } else if (*str == '+') {
        str++;
    }

    // Must have at least one digit
    if (!isdigit((unsigned char)*str)) return false;

    int32_t result = 0;

    while (*str && isdigit((unsigned char)*str)) {
        int digit = *str - '0';

        // Safe overflow checks using absolute boundaries
        if (negative) {
            if (result < INT32_MIN / 10 || (result == INT32_MIN / 10 && -digit < INT32_MIN % 10)) {
                return false; // Underflow
            }
            result = result * 10 - digit;
        } else {
            if (result > INT32_MAX / 10 || (result == INT32_MAX / 10 && digit > INT32_MAX % 10)) {
                return false; // Overflow
            }
            result = result * 10 + digit;
        }
        str++;
    }

    // Handle strict vs non-strict parsing
    if (strictParse) {
        // Strict: strictly no trailing characters allowed
        if (*str != '\0') return false;
    } else {
        // Non-strict: trailing whitespace is okay, but trailing non-digits are ignored
        // (The loop already stopped at the first non-digit character)
    }

    value = result;
    return true;
}

bool EmStringBase::toUInt(uint32_t& value, bool strictParse) const {
    int32_t signedVal = 0;
    bool success = toInt(signedVal, strictParse);
    if (success) {
        value = static_cast<uint32_t>(signedVal);
    }
    return success;
}

bool EmStringBase::startsWith(const char* prefix) const {
    if (!prefix || strlen(prefix) == 0) {
        return false;
    }
    return ::strncmp(m_buf, prefix, strlen(prefix)) == 0;
}

bool EmStringBase::endsWith(const char* suffix) const {
    if (!suffix) {
        return false;
    }
    const size_t suffixLen = strlen(suffix);
    const size_t thisLen = length();
    if (suffixLen > thisLen) {
        return false;
    }
    return ::strcmp(m_buf + thisLen - suffixLen, suffix) == 0;
}

int EmStringBase::indexOf(char ch, size_t fromIndex) const {
    size_t len = length();
    if (fromIndex >= len) {
        return -1;
    }
    const char* result = strchr(m_buf + fromIndex, ch);
    if (result) {
        return result - m_buf;
    }
    return -1;
}

int EmStringBase::indexOf(const char* str, size_t fromIndex) const {
    if (!str) {
        return -1;
    }
    size_t len = length();
    if (fromIndex >= len) {
        return -1;
    }
    const char* result = strstr(m_buf + fromIndex, str);
    if (result) {
        return result - m_buf;
    }
    return -1;
}

EmStrResult EmStringBase::getToken(size_t tokenIndex, char separator, char* out, size_t outMaxStrLen) const {
    if (!out){
        return EmStrResult::failure;
    }
    
    out[0] = '\0';
    if (outMaxStrLen == 0) {
        return EmStrResult::failure;
    }

    int startIndex = 0;
    for (size_t i = 0; i < tokenIndex; ++i) {
        startIndex = indexOf(separator, startIndex);
        if (startIndex == -1) {
            // Token index out of bounds
            return EmStrResult::failure;
        }
        startIndex++; // Move past the separator
    }

    size_t len;
    const int endIndex = indexOf(separator, startIndex);
    if (endIndex != -1) {
        len = endIndex - startIndex;
    } else {
        len = length() - startIndex;
    }

    EmStrResult res = EmStrResult::success;
    if (len >= outMaxStrLen) {
        // Not enough space in out
        len = outMaxStrLen;
        res = EmStrResult::partial; 
    }

    strncpy(out, m_buf + startIndex, len);
    out[len] = '\0';
    return res;
}

// Checks if the token at the specified index matches the given token.
// This version avoids intermediate allocation by comparing the substring directly.
bool EmStringBase::isToken(size_t tokenIndex, char separator, const char* token) const {
    if (!token)
        return false;
    
    int startIndex = 0;
    for (size_t i = 0; i < tokenIndex; ++i) {
        startIndex = indexOf(separator, startIndex);
        if (startIndex == -1) {
            // Token index out of bounds
            return false; 
        }
        startIndex++; // Move past the separator
    }

    const int endIndex = indexOf(separator, startIndex);
    const size_t len = strlen(token);

    if (endIndex != -1) { // Token is not the last one
        return (len == (size_t)(endIndex - startIndex)) && (::strncmp(m_buf + startIndex, token, len) == 0);
    } else { // Last token in the string
        return (len == length() - startIndex) && (::strcmp(m_buf + startIndex, token) == 0);
    }
}

EmStrResult EmStringBase::substring(size_t beginIndex, size_t endIndex, char* out, size_t outMaxStrLen) const {
    if (!out){
        return EmStrResult::failure;
    }
    
    out[0] = '\0';
    if (outMaxStrLen == 0) {
        return EmStrResult::failure;
    }

    const size_t len = length();
    if (beginIndex >= len || beginIndex >= endIndex) {
        return EmStrResult::failure;
    }
    if (endIndex > len) {
        endIndex = len;
    }
    return set_(out, outMaxStrLen, m_buf + beginIndex, endIndex - beginIndex);
}

bool EmStringBase::equals(const char* value, bool caseSensitive) const {
    // Treat nullptr as empty string
    if (value == nullptr) {
        return m_buf[0] == 0;
    }
    // Case sensitive comparison
    if (caseSensitive) {
        return ::strcmp(m_buf, value) == 0;
    }
    // Case insensitive comparison
    const unsigned char* a = reinterpret_cast<const unsigned char*>(m_buf);
    const unsigned char* b = reinterpret_cast<const unsigned char*>(value);
    while (*a && *b) {
        if (::tolower(*a) != ::tolower(*b)) {
            return false;
        }                
        ++a;
        ++b;
    }
    return *a == *b;
} 

EmStrResult EmStringBase::set_(char* buf, 
                               size_t capacity, 
                               const char* value, 
                               EmOptional<size_t> len) {
    if (!buf) {
        return EmStrResult::failure;
    }

    buf[0] = '\0';
    if (!value) {
        return EmStrResult::failure;
    }

    if (len.hasNoValue()) {
        len = strlen(value);
    }

    // Input validation
    if (!value || strlen(value) == 0) {
        buf[0] = '\0';
        return EmStrResult::success;
    }
    
    // Copy the string till capacity or requested len ensuring null termination
    memcpy(buf, value, MIN(len.value(), capacity));
    buf[MIN(len.value(), capacity)] = '\0';

    // Result computation
    if (len.value() > capacity) {
        return EmStrResult::partial;
    }
    return EmStrResult::success;
}

// Hash specialization for EmStringBase to be used in unordered containers
size_t calculateHash(const char* str) {
    const char* p = str;
    // This check happens at compile-time, choosing the right block for your target architecture
    if constexpr (sizeof(std::size_t) == 8) {
        std::size_t hash = 14695981039346656037ULL;
        while (*p) {
            hash ^= static_cast<std::size_t>(*p++);
            hash *= 1099511628211ULL;
        }
        return hash;
    } else {
        std::size_t hash = 2166136261U;
        while (*p) {
            hash ^= static_cast<std::size_t>(*p++);
            hash *= 16777619U;
        }
        return hash;
    }
}

namespace std {
    std::size_t hash<EmStringBase>::operator()(const EmStringBase& s) const noexcept {
        return calculateHash(s.c_str());
    }
}
