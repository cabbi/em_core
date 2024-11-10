#ifndef __DEFS__H_
#define __DEFS__H_

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <math.h>

#define SIZE_OF(x) (sizeof((x))/sizeof((x[0])))

#define MIN(x, y) ((x)<(y) ? (x) : (y))
#define MAX(x, y) ((x)>(y) ? (x) : (y))

// Returns the power of 10`^ exp as an integer number
// This method will avoid using the "double pow10(...)" implementation
inline int32_t iPow10(size_t exp) {
    return static_cast<int32_t>(pow(10, exp));
}

// Returns the integer rounded number
template <class real_type>
inline int32_t iRound(real_type num) {
    return num >= 0 ? static_cast<int32_t>(num+.5) : 
                      static_cast<int32_t>(num-0.5);
}

// Returns the integer multiplied number: num1 * num2
template <class real_type>
inline int32_t iMolt(real_type num1, real_type num2) {
    return static_cast<int32_t>(num1 * num2);
}

// Returns the integer division number: num1 / num2
template <class real_type>
inline int32_t iDiv(real_type num1, real_type num2) {
    return static_cast<int32_t>(num1 / num2);
}

// to_ptr function can be used in template classes that need both reference
// and pointer types implementation
template<typename T>
inline T* to_ptr(T& obj) { return &obj; }

template<typename T>
inline T* to_ptr(T* obj) { return obj; }

// to_str converts numbers to strings
inline const char* to_str(char* buf, size_t bufLen, uint8_t n) {
    snprintf(buf, bufLen, "%u", n);
    return buf;
}
inline const char* to_str(char* buf, size_t bufLen, uint16_t n) {
    snprintf(buf, bufLen, "%u", n);
    return buf;
}
inline const char* to_str(char* buf, size_t bufLen, uint32_t n) {
    snprintf(buf, bufLen, "%lu", n);
    return buf;
}

inline const char* to_str(char* buf, size_t bufLen, int8_t n) {
    snprintf(buf, bufLen, "%d", n);
    return buf;
}
inline const char* to_str(char* buf, size_t bufLen, int16_t n) {
    snprintf(buf, bufLen, "%d", n);
    return buf;
}
inline const char* to_str(char* buf, size_t bufLen, int32_t n) {
    snprintf(buf, bufLen, "%ld", n);
    return buf;
}

inline const char* to_str(char* buf, size_t bufLen, float n) {
    snprintf(buf, bufLen, "%g", n);
    return buf;
}

inline const char* to_str(char* buf, size_t bufLen, double n) {
    snprintf(buf, bufLen, "%g", n);
    return buf;
}


#endif