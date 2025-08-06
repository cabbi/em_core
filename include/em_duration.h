#ifndef EM_DURATION_H
#define EM_DURATION_H

#include "em_defs.h"

// EmDuration class for handling duration-related operations
class EmDuration {
protected:
    uint64_t m_durationMillis;

public:
    EmDuration(uint16_t hours,
               uint16_t minutes, 
               uint16_t seconds, 
               uint16_t milliseconds=0) {
        m_durationMillis = 
            (static_cast<uint64_t>(hours) * 3600 * 1000) + 
            (static_cast<uint64_t>(minutes) * 60 * 1000) + 
            (static_cast<uint64_t>(seconds) * 100) +       
            (static_cast<uint64_t>(milliseconds));
    }
    
    EmDuration(uint64_t milliseconds) 
     : m_durationMillis(milliseconds) {}

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

    EmDuration operator +(uint64_t milliseconds) {
        return EmDuration(m_durationMillis + milliseconds);
    }

    EmDuration operator +(const EmDuration& other) const {
        return EmDuration(m_durationMillis + other.m_durationMillis);
    }
    
    EmDuration operator -(uint64_t milliseconds) const {
        return EmDuration(m_durationMillis - milliseconds);
    }

    EmDuration operator -(const EmDuration& other) const {
        return EmDuration(m_durationMillis - other.m_durationMillis);
    }

    EmDuration& operator +=(uint64_t milliseconds) {
        m_durationMillis += milliseconds;
        return *this;
    }
    
    EmDuration& operator +=(const EmDuration& other) {
        m_durationMillis += other.m_durationMillis;
        return *this;
    }
    
    EmDuration& operator -=(uint64_t milliseconds) {
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
    uint64_t milliseconds() const {
        return m_durationMillis;
    }
};

#endif // EM_DURATION_H