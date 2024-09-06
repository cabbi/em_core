#ifndef __DEBUG_PRINT__H_
#define __DEBUG_PRINT__H_

#include <stdint.h>
#include "em_sync_lock.h"

#ifndef DebugLog

class EmLog {
public:    
    EmLog(bool) {}
    
    void LogInfo(const char*, bool) const {}
    void LogInfo(int16_t, bool)     const {}
    void LogInfo(int32_t, bool)     const {}
    void LogInfo(float, bool)       const {}

    void LogWarning(const char*, bool) const {}
    void LogWarning(int16_t, bool)     const {}
    void LogWarning(int32_t, bool)     const {}
    void LogWarning(float, bool)       const {}

    void LogError(const char*, bool) const {}
    void LogError(int16_t, bool)     const {}
    void LogError(int32_t, bool)     const {}
    void LogError(float, bool)       const {}

    bool IsLogEnabled() const    { return false; }
    void SetLogEnabled(bool) {}
};

#elif
class EmLog {
public:    
    EmLog(bool enabled=false)
     : m_Enabled(enabled),
       m_Lock(xSemaphoreCreateBinary())
     { xSemaphoreGive(m_Lock); }

    void LogInfo(const char* val, bool newLine=true)   const { if (IsLogEnabled()) { Serial.print(val); if (newLine) Serial.print("\n"); } }
    void LogInfo(const String& val, bool newLine=true) const { if (IsLogEnabled()) { Serial.print(val); if (newLine) Serial.print("\n");  } }
    void LogInfo(int16_t val, bool newLine=true)       const { if (IsLogEnabled()) { Serial.print(val); if (newLine) Serial.print("\n");  } }
    void LogInfo(int32_t val, bool newLine=true)       const { if (IsLogEnabled()) { Serial.print(val); if (newLine) Serial.print("\n");  } }
    void LogInfo(float val, bool newLine=true)         const { if (IsLogEnabled()) { Serial.print(val); if (newLine) Serial.print("\n");  } }

    bool IsLogEnabled() const    { SyncLock lock(m_Lock); return m_Enabled; }
    void SetLogEnabled(bool enabled) { SyncLock lock(m_Lock); m_Enabled = enabled; }

protected:
    bool m_Enabled;
    mutable SemaphoreHandle_t m_Lock;
};

#endif
#endif