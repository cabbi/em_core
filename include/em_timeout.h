#ifndef __EM_TIMEOUT_H__
#define __EM_TIMEOUT_H__

#include <stdint.h>
#include <Arduino.h>

#include "em_duration.h"


class EmTimeout
{
public:
    EmTimeout(const EmDuration& timeout, bool startAsElapsed = false) noexcept
     : EmTimeout(timeout.milliseconds(),
                 startAsElapsed) {}

    // Using 'explicit' prevents unintentional conversions from integer types.
    // For example, it would prevent `EmTimeout t = 1000;` which might be ambiguous.
    // The user would have to be explicit: `EmTimeout t(1000);`
    explicit EmTimeout(uint32_t timeoutMs, bool startAsElapsed = false) noexcept
     : m_timeoutMillis(timeoutMs) {
        if (startAsElapsed) {
            setElapsed();
        } else {
            restart();
        }
    }

    EmTimeout(const EmTimeout& other) :
        m_timeoutMillis(static_cast<uint32_t>(other.m_timeoutMillis)),
        m_startMillis(static_cast<uint32_t>(other.m_startMillis)) {}

    // Forces the timeout to be considered elapsed.
    void setElapsed() {
        m_startMillis = millis() - m_timeoutMillis - 1;
    }

    // Gets the timeout duration in milliseconds.
    uint32_t getTimeoutMs() const {
        return m_timeoutMillis;
    } 

    // Gets the timeout duration as an EmDuration object.
    EmDuration getDuration() const {
        return EmDuration(m_timeoutMillis);
    }

    // Sets a new timeout duration.
    void setTimeout(const EmDuration& timeout, bool restartNow = false) {
        setTimeout(timeout.milliseconds(), restartNow);
    }

    // Sets a new timeout duration in milliseconds.
    void setTimeout(uint32_t timeoutMs, bool restartNow = false) {
        m_timeoutMillis=timeoutMs; 
        if (restartNow) {
            this->restart();
        }
    }
    
    // Restarts the timer from the current moment.
    void restart() {
        m_startMillis = millis();
    }
    
    // Checks if the timeout has elapsed.
    bool isElapsed() const {
        // This calculation is safe against millis() rollover.
        // Using >= ensures that a timeout of N milliseconds is considered elapsed
        // once exactly N milliseconds have passed, and aligns with getRemainingMillis().
        return (millis() - m_startMillis) >= m_timeoutMillis;
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
    uint32_t getRemainingMillis() const {
        const uint32_t elapsed = millis() - m_startMillis;
        if (elapsed >= m_timeoutMillis) {
            return 0;
        }
        return m_timeoutMillis - elapsed;
    }

protected:
    ts_uint32 m_timeoutMillis;
    ts_uint32 m_startMillis;
};

#endif // __EM_TIMEOUT_H__