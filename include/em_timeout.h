#ifndef __EM_TIMEOUT_H__
#define __EM_TIMEOUT_H__

#include <stdint.h>

#include "em_duration.h"

template<typename T, typename I> class EmTimeout_;

// The default timeout that supports a maximum of 49 days and 17 hours time.
using EmTimeout = EmTimeout_<uint32_t, ts_uint32>;

// A short timeout object in case you need to handle shorter timeouts up to 65 seconds.
using EmTimeoutShort = EmTimeout_<uint16_t, ts_uint16>;

// The long timeout object in case you need to handle longer timeouts.
// TODO
//using EmTimeoutLong = EmTimeout_<uint64_t, ts_uint64>;

// The millis() function that returns the number of milliseconds since system start.
#ifdef __cplusplus
extern "C" {
    unsigned long millis();
}
#else
    unsigned long millis();
#endif    

// EmTimeout implementation
template<typename T, typename I>
class EmTimeout_
{
public:
    EmTimeout_(const EmHiResDuration_<T, I>& timeout, bool startAsExpired = false) noexcept
     : EmTimeout_(timeout.milliseconds(),
                 startAsExpired) {}

    // Using 'explicit' prevents unintentional conversions from integer types.
    // For example, it would prevent `EmTimeout t = 1000;` which might be ambiguous.
    // The user would have to be explicit: `EmTimeout t(1000);`
    explicit EmTimeout_(T timeoutMs, bool startAsExpired = false)
     : m_timeoutMillis(timeoutMs) {
        if (startAsExpired) {
            setExpired();
        } else {
            restart();
        }
    }

    EmTimeout_(const EmTimeout_& other) 
     : m_timeoutMillis(static_cast<T>(other.m_timeoutMillis)),
       m_startMillis(static_cast<T>(other.m_startMillis)) {}

    EmTimeout_& operator=(const EmTimeout_& other) {
        if (this != &other) {
        #ifdef EM_MULTITHREAD
            m_timeoutMillis.store(other.m_timeoutMillis.load());
            m_startMillis.store(other.m_startMillis.load());
        #else
            m_timeoutMillis = other.m_timeoutMillis;
            m_startMillis = other.m_startMillis;
        #endif
        }
        return *this;
    }

    // Forces the timeout to be considered expired.
    void setExpired() {
        m_startMillis = millis_() - m_timeoutMillis - 1;
    }

    // Gets the timeout duration in milliseconds.
    T getTimeoutMs() const {
        return m_timeoutMillis;
    } 

    // Gets the timeout duration as an EmDuration object.
    EmHiResDuration_<T, I> getDuration() const {
        return EmHiResDuration_<T, I>(m_timeoutMillis);
    }

    // Sets a new timeout duration.
    void setTimeout(const EmHiResDuration_<T, I>& timeout, bool restartNow = false) {
        setTimeout(timeout.milliseconds(), restartNow);
    }

    // Sets a new timeout duration in milliseconds.
    void setTimeout(T timeoutMs, bool restartNow = false) {
        m_timeoutMillis=timeoutMs; 
        if (restartNow) {
            this->restart();
        }
    }
    
    // Restarts the timer from the current moment.
    void restart() {
        m_startMillis = millis_();
    }
    
    // Checks if the timeout has expired.
    bool isExpired() const {
        // This calculation is safe against millis_() rollover.
        // Using >= ensures that a timeout of N milliseconds is considered expired
        // once exactly N milliseconds have passed, and aligns with getRemainingMillis().
        return static_cast<T>(millis_() - m_startMillis) >= m_timeoutMillis;
    }
    
    // Checks if the timeout has expired and optionally restarts the timer if it has.
    bool isExpired(bool restartIfExpired) {
        const bool expired = isExpired();
        if (restartIfExpired && expired) {
            restart();
        }
        return expired;
    }

    bool isNotExpired() const {
        return !isExpired();
    }

    bool isNotExpired(bool restartIfExpired) const {
        return !isExpired(restartIfExpired);
    }
    
    DEPRECATED_MSG("Use 'isExpired' instead!")
    bool isElapsed() const {
        return isExpired();
    }

    DEPRECATED_MSG("Use 'isExpired' instead!")
    bool isElapsed(bool restartIfExpired) {
        return isExpired(restartIfExpired);
    }


    // Gets the remaining time in milliseconds. Returns 0 if the timeout has already expired.
    T getRemainingMillis() const {
        const T expired = static_cast<T>(millis_() - m_startMillis);
        if (expired >= m_timeoutMillis) {
            return 0;
        }
        return m_timeoutMillis - expired;
    }

    EmHiResDuration_<T, I> getRemainingTime() const {
        return EmHiResDuration_<T, I>(getRemainingMillis());
    }

protected:
    T millis_() const {
        return static_cast<T>(millis());
    }

    I m_timeoutMillis;
    I m_startMillis;
};

#endif // __EM_TIMEOUT_H__