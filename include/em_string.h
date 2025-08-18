#ifndef __STRING__H_
#define __STRING__H_

#include <stdint.h>
#include <string.h>
#include <stdarg.h>

// This tiny string class uses a fixed templated size and
// no virtual methods to minimize RAM footprint.
template<size_t maxStrLen>
class EmString {
public:
    EmString() {
        memset(m_buf, 0, sizeof(m_buf));
    }

    EmString(const char* initValue)
     : EmString() {
        set(initValue);
    }

    // Returns the current length of this string object.
    size_t len() {
        return strlen(m_buf);
    }

    // Returns the max length of this string object.
    size_t maxStrLen() {
        return maxStrLen;
    }

    // Set the string to a new value.
    // Returns true if the new value is not longer than max length.
    bool set(const char* value) {
        strncpy(m_buf, value, maxStrLen);
        return strlen(value) <= maxStrLen;
    }

    // Creates a formatted string (i.e. same as 'sprintf').
    const char* format(const char* format, ...) {
        va_list args;
        va_start(args, format);     
        vsnprintf(m_buf, maxStrLen+1, format, args);
        va_end(args);
        return m_buf;
    }

    // Appends a string to current one.
    const char* append(const char* str) {
        int free_size = static_cast<int>(maxStrLen - this->len());
        if (free_size > 0) {
            strncat(m_buf, str, static_cast<size_t>(free_size));
        }
        return m_buf;
    }

    // Gets the string buffer. 
    // Using the string buffer is not safe!
    char* buffer() {
        return m_buf;
    }

    // String compare (i.e. same as 'strcmp').
    // 
    int strcmp(const char* value) {
        return ::strcmp(m_buf, value);
    }

    // 'const char*' casting operator.
    operator const char*() {
        return m_buf;
    }

    // Returns the char at the 'i' position or zero if 'i' is greater or equal
    // than string length. 
    // If 'i' is negative it returns the char starting from end 
    // (e.g. -1 returns the last char of the string).
    char operator [](int i) {
        if (i < 0) {
            i = static_cast<int>(this->len()) + i;
        }
        if (i < 0 || i >= static_cast<int>(this->len())) {
            return 0;
        }
        return m_buf[i];
    }

    // Assigns a new string.
    const char* operator =(const char* value) {
        strncpy(m_buf, value, maxStrLen);
        return m_buf;
    }

    // Equal operator.
    bool operator ==(const char* value) {
        return 0 == this->strcmp(value);
    }

    // Not-equal operator.
    bool operator !=(const char* value) {
        return 0 != this->strcmp(value);
    }

private:
    char m_buf[maxStrLen+1];
};

#endif