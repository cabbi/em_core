#ifndef __TIMEOUT__H_
#define __TIMEOUT__H_

#include <stdint.h>

inline uint32_t millis() { return 0; }

class EmTimeout
{
public:
    EmTimeout(uint32_t timeoutMs)
     : m_timeoutMs(timeoutMs)
    {
        Restart();
    }

    void SetTimeout(uint32_t timeoutMs, bool restart) 
    {
        m_timeoutMs=timeoutMs; 
        if (restart)
        {
            Restart();
        }
    }
    
    void Restart() 
    {
        m_startMs = millis();
    }
    
    bool IsElapsed(bool restartIfElapsed) 
    {
        bool isElapsed = ((millis() - m_startMs) > m_timeoutMs);
        if (restartIfElapsed && isElapsed)
        {
            Restart();
        }
        return isElapsed;
    }

    uint32_t GetRemainingMs()
    {
        int32_t ret = m_timeoutMs-(millis()-m_startMs);
        if (ret > 0)
        { 
            return (uint32_t)ret;
        }
        return 0;
    }

protected:
    uint32_t m_timeoutMs;
    uint32_t m_startMs;
};

#endif