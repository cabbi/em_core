#ifndef __EM_TIME_H
#define __EM_TIME_H   

#include "em_defs.h"

#ifdef EM_TIME

#include <time.h>
#include <WiFi.h>
#include <esp_sntp.h>

#include "em_timeout.h"
#include "em_duration.h"
#include "em_log.h"

using EmEpochTypeSec = uint32_t;
using EmEpochTypeMilli = uint64_t;


// EmTime class for handling time-related operations
class EmTime: public EmLog {
protected:
    mutable bool m_isInitialized;

public:
    EmTime(EmLogLevel logLevel = EmLogLevel::global)
     : EmLog("EmTime", logLevel),
       m_isInitialized(false) { }

    // Begins the time management by configuring the NTP server and time zone
    virtual bool begin(const EmDuration& timeout,
                       uint32_t gmtOffset_sec = 0,
                       uint32_t daylightOffset_sec = 0,
                       const char* ntpServer1 = "pool.ntp.org",
                       const char* ntpServer2 = "time.nist.gov",
                       const char* ntpServer3 = nullptr) {
        configTime(gmtOffset_sec, daylightOffset_sec, 
                   ntpServer1, ntpServer2, ntpServer3);
        return checkInitialized(timeout);
    }

    bool isInitialized() const {
        return m_isInitialized;
    }

    bool checkInitialized(const EmDuration& timeout = EmDuration(100)) const;

    // Get the current time in seconds since epoch
    bool now(EmEpochTypeSec& currentTime) const {
        if (checkInitialized()) {
            currentTime = static_cast<EmEpochTypeSec>(time(nullptr));
        }
        return m_isInitialized;
    }

    // Get the current time in milliseconds since epoch
    bool nowMs(EmEpochTypeMilli& currentTimeMs) const {
        if (checkInitialized()) {
            currentTimeMs = static_cast<EmEpochTypeMilli>(time(nullptr) * 1000);
        }
        return m_isInitialized;
    }
    
    // Get the current time as a struct tm
    bool getTime(struct tm& timeinfo) const {
        if (checkInitialized()) {
            return getLocalTime(&timeinfo);
        }
        return m_isInitialized;
    }

    // Get the up time since the device stated
    static EmLowResDuration getDeviceUpTime() {
        int64_t uptime_us = esp_timer_get_time();
        return EmLowResDuration(static_cast<uint32_t>(uptime_us/1000000));
    }    
};

#endif
#endif // EM_TIME_H