#ifndef __EM_DURATION_H__
#define __EM_DURATION_H__

#include "em_defs.h"
#include "em_threading.h"

template<typename T, typename I> class EmDuration_;

// The default duration that supports a maximum of 49 days and 17 hours time.
using EmDuration = EmDuration_<uint32_t, ts_uint32>;

// A short duration object in case you need to handle shorter durations up to 65 seconds.
using EmDurationShort = EmDuration_<uint16_t, ts_uint16>;

// The long duration object in case you need to handle longer durations.
using EmDurationLong = EmDuration_<uint64_t, ts_uint64>;

// EmDuration class for handling duration-related operations
template<typename T, typename I>
class EmDuration_ {
protected:
    I m_durationMillis;  

public:
    EmDuration_(uint16_t hours,
                uint16_t minutes, 
                uint16_t seconds, 
                uint16_t milliseconds=0) :
        m_durationMillis( 
            (static_cast<T>(hours) * 3600 * 1000) + 
            (static_cast<T>(minutes) * 60 * 1000) + 
            (static_cast<T>(seconds) * 1000) +       
             static_cast<T>(milliseconds)) {}
    
    // Using 'explicit' prevents unintentional conversions from integer types.
    explicit EmDuration_(T milliseconds) :
        m_durationMillis(milliseconds) {}

    EmDuration_(const EmDuration_& other) :
        m_durationMillis(static_cast<T>(other.m_durationMillis)) {}

    // Operators
    bool operator ==(const EmDuration_& other) const {
        return m_durationMillis == other.m_durationMillis;
    }
    
    bool operator !=(const EmDuration_& other) const {
        return m_durationMillis != other.m_durationMillis;
    }
    
    bool operator <(const EmDuration_& other) const {
        return m_durationMillis < other.m_durationMillis;
    }
    
    bool operator >(const EmDuration_& other) const {
        return m_durationMillis > other.m_durationMillis;
    }
    
    bool operator <=(const EmDuration_& other) const {
        return m_durationMillis <= other.m_durationMillis;
    }
    
    bool operator >=(const EmDuration_& other) const {
        return m_durationMillis >= other.m_durationMillis;
    }   

    EmDuration_ operator +(T milliseconds) const {
        return EmDuration_(milliseconds + m_durationMillis);}

    EmDuration_ operator +(const EmDuration_& other) const {
        return EmDuration_(m_durationMillis + other.m_durationMillis);
    }
    
    EmDuration_ operator -(T milliseconds) const {
        return EmDuration_(m_durationMillis - milliseconds);
    }

    EmDuration_ operator -(const EmDuration_& other) const {
        return EmDuration_(m_durationMillis - other.m_durationMillis);
    }

    EmDuration_& operator +=(T milliseconds) {
        m_durationMillis += milliseconds;
        return *this;
    }
    
    EmDuration_& operator +=(const EmDuration_& other) {
        m_durationMillis += other.m_durationMillis;
        return *this;
    }
    
    EmDuration_& operator -=(T milliseconds) {
        m_durationMillis -= milliseconds;
        return *this;
    }
    
    EmDuration_& operator -=(const EmDuration_& other) {
        m_durationMillis -= other.m_durationMillis;
        return *this;
    }

    // Get the duration in hours
    double hours() const {
        return static_cast<double>(m_durationMillis) / (3600 * 1000.0);
    }

    // Get the duration in minutes
    double minutes() const {
        return static_cast<double>(m_durationMillis) / (60 * 1000.0);
    }

    // Get the duration in seconds
    double seconds() const {
        return static_cast<double>(m_durationMillis) / 1000.0;
    }

    // Get the duration in milliseconds
    T milliseconds() const {
        return m_durationMillis;
    }

    void to(uint16_t& days,
            uint16_t& hours,
            uint16_t& minutes,
            uint16_t& seconds,
            uint16_t& milliseconds) const {
        T totalMillis = m_durationMillis;
        days = static_cast<uint16_t>(totalMillis / (24 * 3600 * 1000));
        totalMillis %= (24 * 3600 * 1000);
        hours = static_cast<uint16_t>(totalMillis / (3600 * 1000));
        totalMillis %= (3600 * 1000);
        minutes = static_cast<uint16_t>(totalMillis / (60 * 1000));
        totalMillis %= (60 * 1000);
        seconds = static_cast<uint16_t>(totalMillis / 1000);
    }

    void to(uint16_t& hours,
            uint16_t& minutes,
            uint16_t& seconds,
            uint16_t& milliseconds) const {
        T totalMillis = m_durationMillis;
        hours = static_cast<uint16_t>(totalMillis / (3600 * 1000));
        totalMillis %= (3600 * 1000);
        minutes = static_cast<uint16_t>(totalMillis / (60 * 1000));
        totalMillis %= (60 * 1000);
        seconds = static_cast<uint16_t>(totalMillis / 1000);
        milliseconds = static_cast<uint16_t>(totalMillis % 1000);
    }
};

#endif // EM_DURATION_H