#ifndef __EM_STRING__H_
#define __EM_STRING__H_

#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h> // For vsnprintf

// This tiny string class uses a fixed templated size and
// no virtual methods to minimize RAM footprint.
template<size_t TCapacity>
class EmString {
public:
    // TCapacity is the number of characters, not including the null terminator.
    // The internal buffer will be TCapacity + 1.
    static constexpr size_t Capacity = TCapacity;

    EmString() {
        m_buf[0] = '\0';
    }

    EmString(const char* initValue) {
        set(initValue);
    }

    // Returns the current length of this string object.
    size_t length() const {
        return strlen(m_buf);
    }

    // Returns the max capacity of this string object.
    size_t capacity() const {
        return Capacity;
    }

    // Set the string to a new value. Truncates if the source is too long.
    void set(const char* value) {
        if (!value) {
            m_buf[0] = '\0';
            return;
        }
        // snprintf is safe and guarantees null termination.
        snprintf(m_buf, sizeof(m_buf), "%s", value);
    }

    // Creates a formatted string (i.e. same as 'sprintf').
    const char* format(const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        vsnprintf(m_buf, sizeof(m_buf), fmt, args);
        va_end(args);
        return m_buf;
    }

    // Appends a string to current one.
    void append(const char* str) {
        if (!str) {
            return;
        }
        size_t currentLen = length();
        if (currentLen >= Capacity) {
            return; // Already full
        }
        size_t space_left = Capacity - currentLen;
        strncat_s(m_buf, str, space_left);
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

    // Equal operator.
    bool operator ==(const char* value) {
        return 0 == this->strcmp(value);
    }

    // Not-equal operator.
    bool operator !=(const char* value) {
        return !(*this == value);
    }

private:
    char m_buf[TCapacity + 1];
};

#endif // __EM_STRING__H_