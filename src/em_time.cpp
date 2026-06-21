#include "em_time.h"

bool EmTime:: checkInitialized(const EmDuration& timeout) const {
    // Check if the time is already initialized
    if (m_isInitialized) {
        return true;
    }
    // Attempt to get the local time with a timeout
    struct tm tmInfo;
    m_isInitialized = getLocalTime(&tmInfo, timeout.milliseconds());
    if (m_isInitialized) {
        logInfo<50>("Time initialized [<%d-%02d-%02d %02d:%02d:%02d]!", 
                    tmInfo.tm_year + 1900, tmInfo.tm_mon + 1, tmInfo.tm_mday,
                    tmInfo.tm_hour, tmInfo.tm_min, tmInfo.tm_sec);            
    } else {
        // Try to reinitialize SNTP
        // sntp_stop();  TODO: CHECK IS THIS IS NEEDED!
        // sntp_init();
        logDebug(F("Failed to initialize time within the timeout period"));
    }
    return m_isInitialized;
}