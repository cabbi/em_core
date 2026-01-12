#ifndef __EM_DURATION_H__
#define __EM_DURATION_H__

#include "em_defs.h"
#include "em_threading.h"

// The default duration that supports milliseconds for a maximum of 49 days and 17 hours time.
class EmDuration;

// A short duration object in case you need to handle shorter milliseconds durations up to 65 seconds.
class EmDurationShort;

// The long duration object in case you need to handle longer milliseconds durations.
class EmDurationLong;

// The low resolution duration up to seconds
class EmLowResDuration;


// EmDuration class for handling duration-related operations
template<uint32_t UnitxSec, typename T, typename I>
class EmDuration_ {
public:
    EmDuration_() : EmDuration_(0) {}
             
    // Using 'explicit' prevents unintentional conversions from integer types.
    explicit EmDuration_(T units) :
        m_durationUnits(units) {}

    EmDuration_(const EmDuration_& other) :
        m_durationUnits(static_cast<T>(other.m_durationUnits)) {}

    // Operators
    bool operator ==(const EmDuration_& other) const {
        return m_durationUnits == other.m_durationUnits;
    }
    
    bool operator !=(const EmDuration_& other) const {
        return m_durationUnits != other.m_durationUnits;
    }
    
    bool operator <(const EmDuration_& other) const {
        return m_durationUnits < other.m_durationUnits;
    }
    
    bool operator >(const EmDuration_& other) const {
        return m_durationUnits > other.m_durationUnits;
    }
    
    bool operator <=(const EmDuration_& other) const {
        return m_durationUnits <= other.m_durationUnits;
    }
    
    bool operator >=(const EmDuration_& other) const {
        return m_durationUnits >= other.m_durationUnits;
    }   

    EmDuration_ operator +(T units) const {
        return EmDuration_(units + m_durationUnits);}

    EmDuration_ operator +(const EmDuration_& other) const {
        return EmDuration_(m_durationUnits + other.m_durationUnits);
    }
    
    EmDuration_ operator -(T units) const {
        return EmDuration_(m_durationUnits - units);
    }

    EmDuration_ operator -(const EmDuration_& other) const {
        return EmDuration_(m_durationUnits - other.m_durationUnits);
    }

    EmDuration_& operator +=(T units) {
        m_durationUnits += units;
        return *this;
    }
    
    EmDuration_& operator +=(const EmDuration_& other) {
        m_durationUnits += other.m_durationUnits;
        return *this;
    }
    
    EmDuration_& operator -=(T units) {
        m_durationUnits -= units;
        return *this;
    }
    
    EmDuration_& operator -=(const EmDuration_& other) {
        m_durationUnits -= other.m_durationUnits;
        return *this;
    }

    // Get the duration in hours
    double hours() const {
        return static_cast<double>(m_durationUnits) / static_cast<double>(3600 * UnitxSec);
    }

    // Get the duration in minutes
    double minutes() const {
        return static_cast<double>(m_durationUnits) / static_cast<double>(60 * UnitxSec);
    }

    // Get the duration in seconds
    double seconds() const {
        return static_cast<double>(m_durationUnits) / static_cast<double>(UnitxSec);
    }

    T durationUnits() const {
        return m_durationUnits.load(); 
    } 

    static uint32_t unitxSec() {
        return UnitxSec;
    }

protected:
    I m_durationUnits;  
};

// Class for handling duration-related operations up to seconds
class EmLowResDuration: public EmDuration_<1, uint32_t, ts_uint32> {
public:
    EmLowResDuration(): EmLowResDuration(0) {} 

    EmLowResDuration(uint16_t days, 
                     uint16_t hours, 
                     uint16_t minutes, 
                     uint16_t seconds = 0):
        EmLowResDuration((static_cast<uint32_t>(days) * 24 * 3600) + 
                         (static_cast<uint32_t>(hours) * 3600) + 
                         (static_cast<uint32_t>(minutes) * 60) + 
                         (static_cast<uint32_t>(seconds))) {}

    // Using 'explicit' prevents unintentional conversions from integer types.
    explicit EmLowResDuration(uint32_t seconds):
        EmDuration_<1, uint32_t, ts_uint32>(seconds) {}

    EmLowResDuration(const EmDuration_& other):
        EmDuration_<1, uint32_t, ts_uint32>(static_cast<uint32_t>(other.durationUnits())) {}                                            

    void to(uint16_t& days,
            uint16_t& hours,
            uint16_t& minutes,
            uint16_t& seconds) const {
        uint32_t totalUnits = m_durationUnits;
        days = static_cast<uint16_t>(totalUnits / (24 * 3600));
        totalUnits %= (24 * 3600);
        hours = static_cast<uint16_t>(totalUnits / (3600));
        totalUnits %= (3600);
        minutes = static_cast<uint16_t>(totalUnits / (60));
        totalUnits %= (60);
        seconds = static_cast<uint16_t>(totalUnits);
    }
};

// EmHiResDuration class for handling duration-related operations up to milliseconds
template<typename T, typename I>
class EmHiResDuration_: public EmDuration_<1000, T, I> {
public:
    EmHiResDuration_(): EmHiResDuration_(0) {} 

    EmHiResDuration_(uint16_t hours, 
                     uint16_t minutes, 
                     uint16_t seconds, 
                     uint16_t milliseconds=0):
        EmHiResDuration_((static_cast<T>(hours) * 3600 * 1000) + 
                         (static_cast<T>(minutes) * 60 * 1000) + 
                         (static_cast<T>(seconds) * 1000) +       
                         static_cast<T>(milliseconds)) {}
    // Using 'explicit' prevents unintentional conversions from integer types.
    explicit EmHiResDuration_(T milliseconds):
        EmDuration_<1000, T, I>(milliseconds) {}

    EmHiResDuration_(const EmHiResDuration_& other):
        EmDuration_<1000, T, I>(static_cast<T>(other.durationUnits())) {} 

    void to(uint16_t& hours,
            uint16_t& minutes,
            uint16_t& seconds,
            uint16_t& milliseconds) const {
        T totalUnits = this->durationUnits();
        hours = static_cast<uint16_t>(totalUnits / (3600 * 1000));
        totalUnits %= (3600 * 1000);
        minutes = static_cast<uint16_t>(totalUnits / (60 * 1000));
        totalUnits %= (60 * 1000);
        seconds = static_cast<uint16_t>(totalUnits / 1000);
        milliseconds = static_cast<uint16_t>(totalUnits % 1000);
    }

    // Get the duration in milliseconds
    T milliseconds() const {
        return this->durationUnits();
    }
};

// The default duration that supports milliseconds for a maximum of 49 days and 17 hours time.
class EmDuration: public EmHiResDuration_<uint32_t, ts_uint32> {
public:
    using EmHiResDuration_<uint32_t, ts_uint32>::EmHiResDuration_;
};

// A short duration object in case you need to handle shorter milliseconds durations up to 65 seconds.
class EmDurationShort: public EmHiResDuration_<uint16_t, ts_uint16> {
public:
    using EmHiResDuration_<uint16_t, ts_uint16>::EmHiResDuration_;
};

// The long duration object in case you need to handle longer milliseconds durations.
class EmDurationLong: public EmHiResDuration_<uint64_t, ts_uint64> {
public:
    using EmHiResDuration_<uint64_t, ts_uint64>::EmHiResDuration_;
};

#endif // EM_DURATION_H