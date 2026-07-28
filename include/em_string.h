#ifndef __EM_STRING__H_
#define __EM_STRING__H_

#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h> 
#include <time.h>
#include <ctype.h>

#include "em_defs.h"
#include "em_optional.h"


// Pre-allocated architecture sizes. 
template<size_t Capacity> class EmString;
using EmStringXXS  = EmString<8>;
using EmStringXS   = EmString<16>;
using EmStringS    = EmString<32>;
using EmStringM    = EmString<128>;
using EmStringL    = EmString<512>;    
using EmStringXL   = EmString<1024>;    
using EmStringXXL  = EmString<2048>;    


// Result codes for EmString operations.
enum EmStrResult : uint8_t {
    failure = 0,  // Operation failed
    success = 1,  // Succeeded, full string as result
    partial = 2   // Succeeded, a partial string as result (i.e. buffer too small)
};

// This tiny string class uses a fixed sizeed buffer and no virtual methods to minimize RAM footprint.
// Capacity is the number of characters, not including the null terminator.
// The internal buffer will be Capacity + 1.
class EmStringBase {
public:
    // Destructor (non-virtual to save Flash and eliminate VTable RAM)
    ~EmStringBase() = default;

    // Copy & move operation are removed to let this class be trivially copyable (i.e. is_trivially_copyable_v will pass)
    EmStringBase(const EmStringBase&) = delete;
    EmStringBase(EmStringBase&&) = delete;
    EmStringBase& operator=(const EmStringBase&) = delete;
    EmStringBase& operator=(EmStringBase&&) = delete;

    // Returns the current length of this string object.
    size_t length() const {
        return strlen(m_buf);
    }

    // Returns the max capacity of this string object.
    size_t capacity() const {
        return m_capacity;
    }

    // Returns true if string is empty
    bool isEmpty() const {
        return length() == 0;
    }

    // Returns true if string is NOT empty
    bool isNotEmpty() const {
        return !isEmpty();
    }

    // Returns true if string has reached its capacity
    bool isFull() const {
        return length() == capacity();
    }

    // Returns true if string has NOT reached its capacity
    bool isNotFull() const {
        return !isFull();
    }

    // Clears the current string content
    void clear() {
        memset(m_buf, 0, m_capacity);
    }

    // Retruns the space left to reach the string capacity
    size_t spaceLeft() {
        return capacity() - length();
    }

    // Sets a new string length.
    //
    // Returns the final length.
    size_t setLen(size_t len) {
        m_buf[MIN(len, capacity())] = 0;
        return length();
    }

    // Reduces the string length.
    //
    // Returns the final length.
    size_t reduceLen(size_t val) {
        m_buf[MAX(0, static_cast<int32_t>(length()-val))] = 0;
        return length();
    }

    // Set the string to a new value. Truncates if the source is too long.
    EmStrResult set(const EmStringBase& value) {
        return set(value.c_str());
    }

    EmStrResult set(const char* value) {
        return set_(m_buf, capacity(), value);
    }
    
    // Creates a formatted string (i.e. same as 'sprintf').
    const char* format(const char* fmt, ...);

    const char* format(const char* fmt, va_list args);

    // Appends a string to current one.
    EmStrResult append(const EmStringBase& str, bool allowPartial = false) {
        return append(str.c_str(), allowPartial);
    }

    EmStrResult append(const char* str, bool allowPartial = false);

    // Appends a formatted string to current one (i.e. same as 'sprintf').
    EmStrResult appendFormat(bool allowPartial, const char* fmt, ...);
    EmStrResult appendFormat(bool allowPartial, const char* fmt, va_list args);

    // Converts the string content to an integer.
    // It skips leading spaces or tabs (e.g. '  -123' -> true). 
    // If 'strictParse' is true the parsing will return false if the string  
    // doesn't end with a digit (e.g. '-123abc' -> false) 
    bool toInt(int32_t& value, bool strictParse = true) const;

    // Converts the string content to an unsigned integer.
    // It skips leading spaces or tabs (e.g. '  -123' -> true). 
    // If 'strictParse' is true the parsing will return false if the string  
    // doesn't end with a digit (e.g. '-123abc' -> false) 
    bool toUInt(uint32_t& value, bool strictParse = true) const;

    // Converts the string content to ISO8601 timestamp format (i.e. '%Y-%m-%dT%H:%M:%SZ')
    bool toTimestamp(uint32_t epoch);    

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
    bool startsWith(const char* prefix) const;

    // Checks if the string ends with a specific suffix.
    bool endsWith(const char* suffix) const;

    // Gets the token at a specific 0-based position, using a separator.
    EmStrResult getToken(size_t tokenIndex, char separator, EmStringBase& out) const {
        return getToken(tokenIndex, separator, out.buffer(), out.capacity());
    }
    EmStrResult getToken(size_t tokenIndex, char separator, char* out, size_t outMaxStrLen) const;

    // Checks if the token at the specified index matches the given token.
    // This version avoids intermediate allocation by comparing the substring directly.
    bool isToken(size_t tokenIndex, char separator, const char* token) const;

    // Extracts a substring from this string into the 'out' parameter.
    // The substring begins at the specified 'beginIndex' and extends to the end.
    // Returns true on success, false if 'beginIndex' is out of bounds.
    EmStrResult substring(size_t beginIndex, EmStringBase& out) const {
        return substring(beginIndex, out.buffer(), out.capacity());
    }

    EmStrResult substring(size_t beginIndex, char* out, size_t outMaxStrLen) const {
        return substring(beginIndex, length(), out, outMaxStrLen);
    }

    // Extracts a substring from this string into the 'out' parameter.
    // The substring begins at 'beginIndex' and extends to the character at index 'endIndex' - 1.
    // Returns true on success, false if indices are invalid.
    EmStrResult substring(size_t beginIndex, size_t endIndex, EmStringBase& out) const {
        return substring(beginIndex, endIndex, out.buffer(), out.capacity());
    }
    EmStrResult substring(size_t beginIndex, size_t endIndex, char* out, size_t outMaxStrLen) const;

    // Finds the first occurrence of a character in the string.
    // Returns the index of the first occurrence, or -1 if not found.
    // Search starts from 'fromIndex'.
    int indexOf(char ch, size_t fromIndex = 0) const;

    // Finds the first occurrence of a substring in the string.
    // Returns the index of the first occurrence, or -1 if not found.
    // Search starts from 'fromIndex'.
    int indexOf(const char* str, size_t fromIndex = 0) const;

    // 'const char*' casting operator.
    operator const char*() const {
        return m_buf;
    }
    
    // Returns the char at the 'i' position or zero if 'i' is out of bounds.
    // If 'i' is negative it returns the char starting from end
    // (e.g. -1 returns the last char of the string).
    char operator[](int i) const;
    
    // Equals
    bool equals(const char* value, bool caseSensitive = true) const;
    bool equals(const EmStringBase& value, bool caseSensitive = true) const {
        return equals(value.c_str(), caseSensitive);
    } 


protected:
    static EmStrResult set_(char* buf, 
                            size_t capacity, 
                            const char* value, 
                            EmOptional<size_t> len = EmUndefined());

    // Protected constructor: only derived templated classes can instantiate
    EmStringBase(char* buffer, size_t capacity)
     : m_buf(buffer), m_capacity(capacity) {} 

private:
    char* const m_buf;
    const size_t m_capacity; // Capacity doesn't include null terminator
};


// The templated string class
template<size_t Capacity>
class EmString : public EmStringBase {
public:
    EmString()
     : EmStringBase(m_storage, Capacity) {
        clear();
    }

    EmString(const char* initValue)
     : EmStringBase(m_storage, Capacity) {
        set(initValue);
    }

    EmString(const char* initValue, size_t maxLen)
     : EmStringBase(m_storage, Capacity) {
        set_(m_storage, capacity(), initValue, maxLen);
    }

    EmString(const EmStringBase& initValue)
     : EmStringBase(m_storage, Capacity) {
        set(initValue.c_str());
    }

private:
    char m_storage[Capacity + 1]; // Fixed-size memory block
};

inline bool operator==(const EmStringBase& a, const EmStringBase& b) {
    return a.equals(b, true);
}

inline bool operator!=(const EmStringBase& a, const EmStringBase& b) {
    return !a.equals(b, true);
}

inline bool operator >(const EmStringBase& a, const EmStringBase& b) {
    return strcmp(a.c_str(), b.c_str()) > 0;
}

inline bool operator >=(const EmStringBase& a, const EmStringBase& b) {
    return (a > b) || (a == b);
}

inline bool operator <(const EmStringBase& a, const EmStringBase& b) {
    return !(a > b) && (a != b);
}

inline bool operator <=(const EmStringBase& a, const EmStringBase& b) {
    return !(a > b);
}

#endif // __EM_STRING__H_
