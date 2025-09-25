#ifndef __EM_STRING__H_
#define __EM_STRING__H_

#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h> // For vsnprintf

#include "em_defs.h"
#include "em_optional.h"


enum EmStrResult: uint8_t {
    failure = 0,  // Operation failed
    success = 1,  // Succeeded, full string as result
    partial = 2   // Succeeded, a partial string as result (i.e. buffer to small)
};

// This tiny string class uses a fixed templated size and no virtual methods to minimize RAM footprint.
// TCapacity is the number of characters, not including the null terminator.
// The internal buffer will be TCapacity + 1.
template<size_t TCapacity>
class EmString {
public:
    EmString() {
        m_buf[0] = '\0';
    }

    EmString(const char* initValue) {
        set(initValue);
    }

    EmString(const EmString& initValue) {
        set(initValue.c_str());
    }

    // Returns the current length of this string object.
    size_t length() const {
        return strlen(m_buf);
    }

    // Returns the max capacity of this string object.
    size_t capacity() const {
        return TCapacity;
    }

    // Set the string to a new value. Truncates if the source is too long.
    EmStrResult set(const EmString& value) {
        return set(value.c_str());
    }

    EmStrResult set(const char* value) {
        return set_(m_buf, capacity(), value);
    }
    
    // Creates a formatted string (i.e. same as 'sprintf').
    const char* format(const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        format(fmt, args);
        va_end(args);
        return m_buf;
    }

    const char* format(const char* fmt, va_list args) {
        vsnprintf(m_buf, sizeof(m_buf), fmt, args);
        return m_buf;
    }

    // Appends a string to current one.
    EmStrResult append(const EmString& str) {
        return append(str.c_str());
    }

    EmStrResult append(const char* str) {
        if (!str || strlen(str) == 0) {
            return EmStrResult::success;
        }

        const size_t currentLen = length();
        if (currentLen >= capacity()) {
            return EmStrResult::failure; // Already full
        }

        const size_t space_left = capacity() - currentLen;
        strncat_s(m_buf, str, space_left);
        m_buf[capacity()] = '\0';
        return ::strlen(str) > space_left ? EmStrResult::partial : EmStrResult::success;
    }

    // Gets the string buffer.
    const char* c_str() const {
        return m_buf;
    }

    // Gets the string buffer.
    // Using the string buffer is not safe!
    char* buffer() {
        return m_buf;
    }

    // String compare (i.e. same as 'strcmp').
    int strcmp(const char* value) const {
        return ::strcmp(m_buf, value);
    }

    // String compare (i.e. same as 'strcmp').
    int strncmp(const char* value, size_t maxCount) const {
        return ::strncmp(m_buf, value, maxCount);
    }

    // Checks if the string starts with a specific prefix.
    bool startsWith(const char* prefix) const {
        if (!prefix || strlen(prefix) == 0) {
            return false;
        }
        return ::strncmp(m_buf, prefix, strlen(prefix)) == 0;
    }

    // Checks if the string ends with a specific suffix.
    bool endsWith(const char* suffix) const {
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

    // Gets the token at a specific 0-based position, using a separator.
    template<size_t TOutCapacity>
    EmStrResult getToken(size_t tokenIndex, char separator, EmString<TOutCapacity>& out) const {
        return getToken(tokenIndex, separator, out.buffer(), out.capacity());
    }

    EmStrResult getToken(size_t tokenIndex, char separator, char* out, size_t outMaxStrLen) const {
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
    bool isToken(size_t tokenIndex, char separator, const char* token) const {
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

    // Extracts a substring from this string into the 'out' parameter.
    // The substring begins at the specified 'beginIndex' and extends to the end.
    // Returns true on success, false if 'beginIndex' is out of bounds.
    template<size_t TOutCapacity>
    EmStrResult substring(size_t beginIndex, EmString<TOutCapacity>& out) const {
        return substring(beginIndex, out.buffer(), out.capacity());
    }

    EmStrResult substring(size_t beginIndex, char* out, size_t outMaxStrLen) const {
        return substring(beginIndex, length(), out, outMaxStrLen);
    }

    // Extracts a substring from this string into the 'out' parameter.
    // The substring begins at 'beginIndex' and extends to the character at index 'endIndex' - 1.
    // Returns true on success, false if indices are invalid.
    template<size_t TOutCapacity>
    EmStrResult substring(size_t beginIndex, size_t endIndex, EmString<TOutCapacity>& out) const {
        return substring(beginIndex, endIndex, out.buffer(), out.capacity());
    }

    EmStrResult substring(size_t beginIndex, size_t endIndex, char* out, size_t outMaxStrLen) const {
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

    // Finds the first occurrence of a character in the string.
    // Returns the index of the first occurrence, or -1 if not found.
    // Search starts from 'fromIndex'.
    int indexOf(char ch, size_t fromIndex = 0) const {
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

    // Finds the first occurrence of a substring in the string.
    // Returns the index of the first occurrence, or -1 if not found.
    // Search starts from 'fromIndex'.
    int indexOf(const char* str, size_t fromIndex = 0) const {
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

    // 'const char*' casting operator.
    operator const char*() const {
        return m_buf;
    }

    // Returns the char at the 'i' position or zero if 'i' is out of bounds.
    // If 'i' is negative it returns the char starting from end
    // (e.g. -1 returns the last char of the string).
    char operator[](int i) const {
        size_t len = length();
        if (i < 0) {
            i = static_cast<int>(len) + i;
        }
        if (i < 0 || i >= static_cast<int>(len)) {
            return 0;
        }
        return m_buf[i];
    }

    // Assigns a new string.
    EmString& operator=(const char* value) {
        set(value);
        return *this;
    }

    EmString& operator=(const EmString& value) {
        set(value.c_str());
        return *this;
    }

    // Equal operator.
    bool operator ==(const char* value) {
        return 0 == this->strcmp(value);
    }

    // Not-equal operator.
    bool operator !=(const char* value) {
        return !(*this == value);
    }

protected:
    static EmStrResult set_(char* buf, 
                            size_t capacity, 
                            const char* value, 
                            EmOptional<size_t> len = EmUndefined()) {
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
        
        // Copy the string till capacity ensuring null termination
        memcpy(buf, value, MIN(len.value(), capacity));
        buf[MIN(len.value(), capacity)] = '\0';

        // Result computation
        if (len.value() > capacity) {
            return EmStrResult::partial;
        }
        return EmStrResult::success;
    }

private:
    char m_buf[TCapacity + 1];
};

#endif // __EM_STRING__H_