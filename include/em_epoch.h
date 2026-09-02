#ifndef __EM_EPOCH_H__
#define __EM_EPOCH_H__

#include <stdint.h>
#include <time.h>

// A tiny epoch class used to define different representations. 
template<typename T>
struct EmEpoch {
    T value;

    EmEpoch() : value(0) {}
    explicit EmEpoch(T val) : value(val) {}

    static EmEpoch Zero() { return EmEpoch(0); }
    static EmEpoch Invalid() { return EmEpoch(0); }

    bool getTime(struct tm& timeinfo) {
        time_t time = static_cast<time_t>(value);
        return localtime_r(&time, &timeinfo) != nullptr;
    }

    // Explicit conversion operator 
    explicit operator T() const { return value; }

    bool isZero() const { return value == 0; }
    bool isInvalid() const { return value == 0; }
    bool isValid() const { return value > 0; }

    bool operator==(const EmEpoch& other) const { return value == other.value; }
    bool operator!=(const EmEpoch& other) const { return value != other.value; }
    bool operator< (const EmEpoch& other) const { return value <  other.value; }
    bool operator<=(const EmEpoch& other) const { return value <= other.value; }
    bool operator> (const EmEpoch& other) const { return value >  other.value; }
    bool operator>=(const EmEpoch& other) const { return value >= other.value; }

    EmEpoch operator+(T seconds) const {
        return EmEpoch(value + seconds);
    }

    EmEpoch operator-(T seconds) const {
        return EmEpoch(value - seconds);
    }

    EmEpoch& operator+=(T seconds) {
        value += seconds;
        return *this;
    }

    EmEpoch& operator-=(T seconds) {
        value -= seconds;
        return *this;
    }

    // Increment / Decrement
    EmEpoch& operator++() { 
        ++value;
        return *this;
    }

    EmEpoch operator++(int) { 
        EmEpoch temp = *this;
        ++value;
        return temp;
    }

    EmEpoch& operator--() { 
        --value;
        return *this;
    }

    EmEpoch operator--(int) { 
        EmEpoch temp = *this;
        --value;
        return temp;
    }
};

// The explicit epoch type using 32 bits only
// By using an uint32_t, the epoch will overflow on February 7, 2106
using EmEpoch32 = EmEpoch<uint32_t>;

// A full 64bit epoch
using EmEpoch64 = EmEpoch<int64_t>;

// An epoch storing milliseconds since 1970-1-1
using EmEpochAsMillisec = EmEpoch<int64_t>;

// Commutative helper: (seconds & epoch)
template<typename T>
inline EmEpoch<T> operator+(T seconds, const EmEpoch<T>& epoch) {
    return EmEpoch<T>(epoch.value + seconds);
}
template<typename T>
inline EmEpoch<T> operator-(T seconds, const EmEpoch<T>& epoch) {
    return EmEpoch<T>(epoch.value - seconds);
}

#endif //  __EM_EPOCH_H__
