#ifndef __EM_WIFI_H__
#define __EM_WIFI_H__

#include <Arduino.h>
#include <WiFi.h>
#include <atomic>
#include <vector>

#include "em_log.h"
#include "em_string.h"
#include "em_threading.h"


enum class EmWiFiLevel: uint8_t {
    notConnected = 0,
    veryWeak     = 1,
    weak         = 2,
    fair         = 3,
    good         = 4,
    excellent    = 5
};

inline EmWiFiLevel getWiFiLevel(int8_t rssi) {
    if (rssi <= -100 || rssi >= 0) return EmWiFiLevel::notConnected;
    if (rssi >= -50) return EmWiFiLevel::excellent;
    if (rssi >= -60) return EmWiFiLevel::good;
    if (rssi >= -70) return EmWiFiLevel::fair;
    if (rssi >= -80) return EmWiFiLevel::weak;
    return EmWiFiLevel::veryWeak;  
}

inline const char* getWiFiLevelName(EmWiFiLevel level) {
    switch(level) {
        case EmWiFiLevel::notConnected: return "Not connected";
        case EmWiFiLevel::veryWeak:     return "Very weak";
        case EmWiFiLevel::weak:         return "Weak";
        case EmWiFiLevel::fair:         return "Fair";
        case EmWiFiLevel::good:         return "Good";
        case EmWiFiLevel::excellent:    return "Excellent";
    }
    return "Unknown";
}

struct EmWiFiCredential {
    EmStringS ssid;
    EmStringS password;
};

// This class manages Wi-Fi connections and orchestrates scanning and  
// connecting to the best available network from a pool of credentials.
class EmWiFi {
public:
    EmWiFi()
     : m_taskHandle(nullptr),
       m_checkIntervalSec(0) {}
    
    ~EmWiFi() { stop(); }
    
    // Add a new network configuration to the AP list.
    // Max 128 APs
    bool addAP(const char* ssid, const char* passphrase);
    
    void resetApPool() {
        EmMutexLock lock(m_networkMutex);
        m_networks.clear();
        m_currentSsid.clear();
    }
    
    int8_t getApCount() const {
        // No need to block this networks read operation
        return static_cast<int8_t>(m_networks.size());
    }

    // Start the WiFi connection check loop.
    // The loop will check each 'checkIntervalSec' if WiFi is 
    //  connected and the level is above the 'checkLevel'.  
    bool start(uint16_t checkIntervalSec = 10,
               EmWiFiLevel checkLevel = EmWiFiLevel::fair);
    void stop();

    bool isRunning() const {
        return m_taskHandle != nullptr;
    }

    bool isConnected() const {
        return WiFi.isConnected();
    }

    bool isNotConnected() const {
        return !isConnected();
    }

    const char* getSsid(EmStringS& ssid) const {
        ssid.set(WiFi.SSID().c_str());
        return ssid.c_str();
    }   

    int8_t getRssi() const {
        if (!isConnected()) {
            return 0;
        }
        return WiFi.RSSI();
    }

    EmWiFiLevel getWiFiLevel() const {
        if (!isConnected()) {
            return EmWiFiLevel::notConnected;
        }
        return ::getWiFiLevel(getRssi());
    }

    const char* getWiFiLevelName() const {
        if (!isConnected()) {
            return "Not connected";
        }
        return ::getWiFiLevelName(getWiFiLevel());
    }

private:
    bool getBestNetwork_(EmWiFiCredential& bestNetwork);
    void setCurrentSsid_(const EmStringS& ssid) {
        EmMutexLock lock(m_networkMutex);
        m_currentSsid.set(ssid);
    }
    bool isCurrentSsid_(const EmStringS& ssid) const {
        EmMutexLock lock(m_networkMutex);
        return m_currentSsid == ssid;
    }
    static void wifiTaskCore_(void* pvParameters); // FreeRTOS task function

    EmStringS m_currentSsid;
    mutable EmMutex m_networkMutex;
    std::vector<EmWiFiCredential> m_networks;
    std::atomic<TaskHandle_t> m_taskHandle;
    std::atomic<uint16_t> m_checkIntervalSec;
    std::atomic<EmWiFiLevel> m_checkLevel;
};

#endif
