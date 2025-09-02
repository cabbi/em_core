#ifndef __EM_DURATION_H__
#define __EM_DURATION_H__

#include "em_defs.h"
#include "em_threading.h"

// EmDuration class for handling duration-related operations
// NOTE:
// due to 32 bit implementation a maximum duration of 49 days and 17 hours is supported!
class EmDuration {
protected:
    ts_uint32 m_durationMillis;  

public:
    EmDuration(uint16_t hours,
               uint16_t minutes, 
               uint16_t seconds, 
               uint16_t milliseconds=0) :
        m_durationMillis( 
            (static_cast<uint32_t>(hours) * 3600 * 1000) + 
            (static_cast<uint32_t>(minutes) * 60 * 1000) + 
            (static_cast<uint32_t>(seconds) * 1000) +       
             static_cast<uint32_t>(milliseconds)) {}
    
    // Using 'explicit' prevents unintentional conversions from integer types.
    explicit EmDuration(uint32_t milliseconds) :
        m_durationMillis(milliseconds) {}

    EmDuration(const EmDuration& other) :
        m_durationMillis(static_cast<uint32_t>(other.m_durationMillis)) {}

    // Operators
    bool operator ==(const EmDuration& other) const {
        return m_durationMillis == other.m_durationMillis;
    }
    
    bool operator !=(const EmDuration& other) const {
        return m_durationMillis != other.m_durationMillis;
    }
    
    bool operator <(const EmDuration& other) const {
        return m_durationMillis < other.m_durationMillis;
    }
    
    bool operator >(const EmDuration& other) const {
        return m_durationMillis > other.m_durationMillis;
    }
    
    bool operator <=(const EmDuration& other) const {
        return m_durationMillis <= other.m_durationMillis;
    }
    
    bool operator >=(const EmDuration& other) const {
        return m_durationMillis >= other.m_durationMillis;
    }   

    EmDuration operator +(uint32_t milliseconds) const {
        return EmDuration(milliseconds + m_durationMillis);}

    EmDuration operator +(const EmDuration& other) const {
        return EmDuration(m_durationMillis + other.m_durationMillis);
    }
    
    EmDuration operator -(uint32_t milliseconds) const {
        return EmDuration(m_durationMillis - milliseconds);
    }

    EmDuration operator -(const EmDuration& other) const {
        return EmDuration(m_durationMillis - other.m_durationMillis);
    }

    EmDuration& operator +=(uint32_t milliseconds) {
        m_durationMillis += milliseconds;
        return *this;
    }
    
    EmDuration& operator +=(const EmDuration& other) {
        m_durationMillis += other.m_durationMillis;
        return *this;
    }
    
    EmDuration& operator -=(uint32_t milliseconds) {
        m_durationMillis -= milliseconds;
        return *this;
    }
    
    EmDuration& operator -=(const EmDuration& other) {
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
    uint32_t milliseconds() const {
        return m_durationMillis;
    }

};

#endif // EM_DURATION_H