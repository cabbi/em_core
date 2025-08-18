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
     : m_timeoutMillis(timeoutMs) {
        if (startAsElapsed) {
            setElapsed();
        } else {
            restart();
        }
    }

    void setElapsed() {
        m_startMillis = millis() - m_timeoutMillis - 1;
    }

    uint32_t getTimeoutMs() const {
        return m_timeoutMillis;
    } 

    void setTimeout(uint32_t timeoutMs, bool restart) {
        m_timeoutMillis=timeoutMs; 
        if (restart) {
            this->restart();
        }
    }
    
    void restart() {
        m_startMillis = millis();
    }
    
    bool isElapsed(bool restartIfElapsed) {
        bool isElapsed = ((millis() - m_startMillis) > m_timeoutMillis);
        if (restartIfElapsed && isElapsed) {
            restart();
        }
        return isElapsed;
    }

    uint32_t getRemainingMillis() const {
        int32_t ret = (int32_t)(m_timeoutMillis-(millis()-m_startMillis));
        if (ret > 0) { 
            return (uint32_t)ret;
        }
        return 0;
    }

protected:
    uint32_t m_timeoutMillis;
    uint32_t m_startMillis;
};

#endif