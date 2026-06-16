#include "em_wifi.h"


bool EmWiFi::addAP(const char* ssid, const char* passphrase) {
    if (_networks.size() < 128 && ssid && strlen(ssid) > 0) {
        _networks.push_back({EmStringS(ssid), EmStringS(passphrase)});
        return true;
    }
    return false;
}

bool EmWiFi::start(uint16_t checkIntervalSec, EmWiFiLevel checkLevel) {
    _checkIntervalSec = checkIntervalSec;
    _checkLevel = checkLevel;
    
    if (_taskHandle == nullptr) {
        // Creates a background task running on Core 0 to leave Core 1 free for your loop()
        TaskHandle_t taskHandle;
        xTaskCreatePinnedToCore(
            this->wifiTaskCore_,   // Function to execute
            "EmWiFi_task",        // Name of task
            4096,                 // Stack size in words
            this,                 // Parameter passed to the task (pointer to this instance)
            1,                    // Task priority
            &taskHandle,          // Task handle
            0                     // Core ID (0)
        );
        _taskHandle = taskHandle;
        return taskHandle != nullptr;
    }
    return false;
}

void EmWiFi::stop() {
    if (_taskHandle != nullptr) {
        vTaskDelete(_taskHandle);
        _taskHandle = nullptr;
    }
}

int16_t EmWiFi::getBestNetworkIndex_() {
    int16_t scanResult = WiFi.scanNetworks();
    if (scanResult <= 0) {
        return -1;
    }

    int16_t bestNetworkIndexInList = -1;
    int32_t highestRssi = -1000;

    for (int i = 0; i < scanResult; i++) {
        EmStringS scannedSsid(WiFi.SSID(i).c_str());
        int currentRssi = WiFi.RSSI(i);

        for (size_t j = 0; j < _networks.size(); j++) {
            if (_networks[j].ssid == scannedSsid) {
                if (currentRssi > highestRssi) {
                    highestRssi = currentRssi;
                    bestNetworkIndexInList = j;
                }
            }
        }
    }
    WiFi.scanDelete();
    return bestNetworkIndexInList;
}

void EmWiFi::wifiTaskCore_(void* pvParameters) {
    // Cast the void pointer back to our class instance
    EmWiFi* instance = static_cast<EmWiFi*>(pvParameters);

    while (true) {
        if (!instance->_networks.empty() &&
            (WiFi.status() != WL_CONNECTED ||
             instance->getWiFiLevel() <= instance->_checkLevel)) {
            int16_t bestIndex = instance->getBestNetworkIndex_();

            if (bestIndex != -1 &&
                bestIndex != instance->_currentApIndex) {
                instance->_currentApIndex = bestIndex;
                WiFi.disconnect();
                WiFi.begin(instance->_networks[bestIndex].ssid.c_str(), 
                            instance->_networks[bestIndex].password.c_str());
                            
                
                // Wait for connection with a 10-second timeout
                int attempts = 0;
                while (WiFi.status() != WL_CONNECTED && attempts < 20) {
                    vTaskDelay(pdMS_TO_TICKS(500));
                    attempts++;
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(instance->_checkIntervalSec*1000));
    }
}
