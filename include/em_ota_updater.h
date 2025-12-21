#ifndef __EM_OTA_UPDATER_H__
#define __EM_OTA_UPDATER_H__

#include "em_defs.h"

class EmOtaUpdater {
public:
    virtual bool writeStream(size_t contentLength, Stream& client) = 0;
};

#ifdef EM_ESP
#include <Update.h>
#include "em_log.h"

class Esp32OtaUpdater: public EmOtaUpdater {
public:
    virtual bool writeStream(size_t contentLength, Stream& client) override {
        if (!::Update.begin(contentLength)) {
            logError("Esp32OtaUpdater", "Not enough space to begin OTA");
            return false;
        }
        size_t written = ::Update.writeStream(client);
        if (written == 0) {
            logError("Esp32OtaUpdater", "Update failed");
            return false;
        } 
        logInfo("Esp32OtaUpdater", "Update successful!");
        ::Update.end();
        ESP.restart();
        return true;
    }
};
#endif // EM_ESP

#endif // __EM_OTA_UPDATER_H__

