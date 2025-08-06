#ifndef __TIMEOUT__H_
#define __TIMEOUT__H_

#include <stdint.h>
#include "em_duration.h"

#include <Arduino.h>
//unsigned long millis(void);


class EmTimeout
{
public:
    EmTimeout(const EmDuration timeout, bool startAsElapsed=false)
     : EmTimeout(timeout.milliseconds(),
                 startAsElapsed) {}

    EmTimeout(uint32_t timeoutMs, bool startAsElapsed=false)
     : m_timeoutMs(timeoutMs) {
        if (startAsElapsed) {
            SetElapsed();
        } else {
            Restart();
        }
    }

    void SetElapsed() {
        m_startMs = millis() - m_timeoutMs - 1;
    }

    uint32_t GetTimeoutMs() const {
        return m_timeoutMs;
    } 

    void SetTimeout(uint32_t timeoutMs, bool restart) {
        m_timeoutMs=timeoutMs; 
        if (restart) {
            Restart();
        }
    }
    
    void Restart() {
        m_startMs = millis();
    }
    
    bool IsElapsed(bool restartIfElapsed) {
        bool isElapsed = ((millis() - m_startMs) > m_timeoutMs);
        if (restartIfElapsed && isElapsed) {
            Restart();
        }
        return isElapsed;
    }

    uint32_t GetRemainingMs() const {
        int32_t ret = (int32_t)(m_timeoutMs-(millis()-m_startMs));
        if (ret > 0) { 
            return (uint32_t)ret;
        }
        return 0;
    }

protected:
    uint32_t m_timeoutMs;
    uint32_t m_startMs;
};

#endif