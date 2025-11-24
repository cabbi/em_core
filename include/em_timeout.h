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
using EmTimeoutLong = EmTimeout_<uint64_t, ts_uint64>;

// The millis() function that returns the number of milliseconds since system start.
extern "C" {
    extern unsigned long millis();
}

// EmTimeout implementation
template<typename T, typename I>
class EmTimeout_
{
public:
    EmTimeout_(const EmDuration_<T, I>& timeout, bool startAsElapsed = false) noexcept
     : EmTimeout_(timeout.milliseconds(),
                 startAsElapsed) {}

    // Using 'explicit' prevents unintentional conversions from integer types.
    // For example, it would prevent `EmTimeout t = 1000;` which might be ambiguous.
    // The user would have to be explicit: `EmTimeout t(1000);`
    explicit EmTimeout_(T timeoutMs, bool startAsElapsed = false) noexcept
     : m_timeoutMillis(timeoutMs) {
        if (startAsElapsed) {
            setElapsed();
        } else {
            restart();
        }
    }

    EmTimeout_(const EmTimeout_& other) :
        m_timeoutMillis(static_cast<T>(other.m_timeoutMillis)),
        m_startMillis(static_cast<T>(other.m_startMillis)) {}

    // Forces the timeout to be considered elapsed.
    void setElapsed() {
        m_startMillis = millis_() - m_timeoutMillis - 1;
    }

    // Gets the timeout duration in milliseconds.
    T getTimeoutMs() const {
        return m_timeoutMillis;
    } 

    // Gets the timeout duration as an EmDuration object.
    EmDuration_<T, I> getDuration() const {
        return EmDuration_<T, I>(m_timeoutMillis);
    }

    // Sets a new timeout duration.
    void setTimeout(const EmDuration_<T, I>& timeout, bool restartNow = false) {
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
    
    // Checks if the timeout has elapsed.
    bool isElapsed() const {
        // This calculation is safe against millis_() rollover.
        // Using >= ensures that a timeout of N milliseconds is considered elapsed
        // once exactly N milliseconds have passed, and aligns with getRemainingMillis().
        return static_cast<T>(millis_() - m_startMillis) >= m_timeoutMillis;
    }

    // Checks if the timeout has elapsed and optionally restarts the timer if it has.
    bool isElapsed(bool restartIfElapsed) {
        const bool elapsed = isElapsed();
        if (restartIfElapsed && elapsed) {
            restart();
        }
        return elapsed;
    }

    // Gets the remaining time in milliseconds. Returns 0 if the timeout has already elapsed.
    T getRemainingMillis() const {
        const T elapsed = static_cast<T>(millis_() - m_startMillis);
        if (elapsed >= m_timeoutMillis) {
            return 0;
        }
        return m_timeoutMillis - elapsed;
    }

protected:
    T millis_() const {
        return static_cast<T>(millis());
    }

    I m_timeoutMillis;
    I m_startMillis;
};

#endif // __EM_TIMEOUT_H__