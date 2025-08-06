#ifndef EM_TIME_H
#define EM_TIME_H   

#include "time.h"
#include <WiFi.h>
#include <esp_sntp.h>

#include "em_defs.h"
#include "em_timeout.h"
#include "em_log.h"

// EmTime class for handling time-related operations
class EmTime: public EmLog {
protected:
    bool m_isInitialized;

public:
    EmTime()
     : EmLog("EmTime"),
       m_isInitialized(false) { }

    // Begins the time management by configuring the NTP server and time zone
    virtual bool begin(const EmDuration& timeout,
                       uint32_t gmtOffset_sec = 0,
                       uint32_t daylightOffset_sec = 0,
                       const char* ntpServer = "pool.ntp.org") {
        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
        return checkInitialized(timeout);
    }

    bool isInitialized() const {
        return m_isInitialized;
    }

    bool checkInitialized(const EmDuration& timeout = EmDuration(100)) {
        // Check if the time is already initialized
        if (m_isInitialized) {
            return true;
        }
        // Attempt to get the local time with a timeout
        struct tm tmInfo;
        m_isInitialized = getLocalTime(&tmInfo, timeout.milliseconds());
        if (m_isInitialized) {
            LogInfo<50>("Time initialized [<%d-%02d-%02d %02d:%02d:%02d]!", 
                        tmInfo.tm_year + 1900, tmInfo.tm_mon + 1, tmInfo.tm_mday,
                        tmInfo.tm_hour, tmInfo.tm_min, tmInfo.tm_sec);            
        } else {
            LogError("Failed to initialize time within the timeout period.");
        }
        return m_isInitialized;
    }

    // Get the current time in seconds since epoch
    bool now(time_t& currentTime) {
        if (checkInitialized()) {
            currentTime = time(nullptr);
        }
        return m_isInitialized;
    }

    // Get the current time in milliseconds since epoch
    bool nowMs(uint32_t& currentTimeMs) {
        if (checkInitialized()) {
            currentTimeMs = static_cast<uint32_t>(time(nullptr) * 1000);
        }
        return m_isInitialized;
    }
    
    // Get the current time as a struct tm
    bool getTime(struct tm& timeinfo) {
        if (checkInitialized()) {
            return getLocalTime(&timeinfo);
        }
        return m_isInitialized;
    }   
};

#endif