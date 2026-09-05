#ifndef __EM_EPOCH_H__
#define __EM_EPOCH_H__

#include <stdint.h>
#include <time.h>

// A tiny epoch class used to define different representations. 
template<typename T>
struct EmEpochBase {
    T value;

    EmEpochBase() : value(0) {}
    explicit EmEpochBase(T val) : value(val) {}

    static EmEpochBase Zero() { return EmEpochBase(0); }
    static EmEpochBase Invalid() { return EmEpochBase(0); }

    bool getTime(struct tm& timeinfo) {
        time_t time = static_cast<time_t>(value);
        return localtime_r(&time, &timeinfo) != nullptr;
    }

    // Explicit conversion operator 
    explicit operator T() const { return value; }

    bool isZero() const { return value == 0; }
    bool isInvalid() const { return value == 0; }
    bool isValid() const { return value > 0; }

    bool operator==(const EmEpochBase& other) const { return value == other.value; }
    bool operator!=(const EmEpochBase& other) const { return value != other.value; }
    bool operator< (const EmEpochBase& other) const { return value <  other.value; }
    bool operator<=(const EmEpochBase& other) const { return value <= other.value; }
    bool operator> (const EmEpochBase& other) const { return value >  other.value; }
    bool operator>=(const EmEpochBase& other) const { return value >= other.value; }

    EmEpochBase operator+(T seconds) const {
        return EmEpochBase(value + seconds);
    }

    EmEpochBase operator-(T seconds) const {
        return EmEpochBase(value - seconds);
    }

    EmEpochBase& operator+=(T seconds) {
        value += seconds;
        return *this;
    }

    EmEpochBase& operator-=(T seconds) {
        value -= seconds;
        return *this;
    }

    // Increment / Decrement
    EmEpochBase& operator++() { 
        ++value;
        return *this;
    }

    EmEpochBase operator++(int) { 
        EmEpochBase temp = *this;
        ++value;
        return temp;
    }

    EmEpochBase& operator--() { 
        --value;
        return *this;
    }

    EmEpochBase operator--(int) { 
        EmEpochBase temp = *this;
        --value;
        return temp;
    }
};

// The compiler will choose the appropriate epoch type based on the pointer size (32-bit or 64-bit).
using EmEpoch = EmEpochBase<std::conditional<sizeof(void*) == 8, int64_t, uint32_t>::type>;

// The explicit epoch type using 32 bits only
// By using an uint32_t, the epoch will overflow on February 7, 2106
using EmEpoch32 = EmEpochBase<uint32_t>;

// A full 64bit epoch
using EmEpoch64 = EmEpochBase<int64_t>;

// An epoch storing milliseconds since 1970-1-1
using EmEpochAsMillisec = EmEpochBase<int64_t>;

// Commutative helper: (seconds & epoch)
template<typename T>
inline EmEpochBase<T> operator+(T seconds, const EmEpochBase<T>& epoch) {
    return EmEpochBase<T>(epoch.value + seconds);
}
template<typename T>
inline EmEpochBase<T> operator-(T seconds, const EmEpochBase<T>& epoch) {
    return EmEpochBase<T>(epoch.value - seconds);
}

#endif //  __EM_EPOCH_H__
