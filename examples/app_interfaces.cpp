#include <Arduino.h>

#include "em_defs.h"
#include "em_log_print.h"

#include "em_app.h"
#include "em_app_interface.h"
#include "em_app_task_interface.h"

// A dummy application interface class for demonstration.
class MyInterface : public EmAppInterface {
public:
    MyInterface(const char* name,
                uint16_t loopDelayMs,
                EmList<EmAppInterface>* appInterfaces=nullptr) : 
        EmAppInterface(appInterfaces, EmDuration(0, 1, 0), name, EmLogLevel::info),
        m_loopDelayMs(loopDelayMs) {}

    virtual const char* name() const { return getContext(); }

    virtual EmIntOperationResult setup() override {
        logInfo("setup...");
        delay(1000); // Simulate work in setup
        return EmIntOperationResult::canContinue;
    }

    virtual EmIntOperationResult loop() override {
        logInfo("task loop...");
        delay(m_loopDelayMs); // Simulate work in loop
        return EmIntOperationResult::canContinue;
    }

protected:
    uint16_t m_loopDelayMs;
};

// The application object thar will own top level interfaces.
EmApp app;

// A top level application interface. This interface will run in the application loop.
// We pass the app interfaces list since this interface will be owned by the app.
MyInterface myAppInterface("Interface 1", 2000, &app.interfaces());

// A single task interface class
MyInterface singleTaskInterface("Single Task Interface", 1000);

// The task interface that will own the single task interface
// We pass the app interfaces list since this interface will be owned by the app.
EmAppTaskInterface taskInterface(singleTaskInterface, false, &app.interfaces());

// A multi-task interfaces class. 
// This interface will run all its owned interfaces in its own task.
// We pass the app interfaces list since this interface will be owned by the app.
EmAppTaskInterfaces taskInterfaces("MultiTaskInterfaces", true, &app.interfaces());

// The two interfaces that will be owned by the multi-task interfaces
MyInterface myTask1Interface("Multi Task Interface 1", 500, &taskInterfaces);
MyInterface myTask2Interface("Multi Task Interface 2", 250, &taskInterfaces);

// Log target using the USB Serial port
EmLogPrintTarget<USB_SERIAL_CLASS> logPrint(Serial);

void setup() {
    delay(3000); // Wait for user's serial monitor to open
    Serial.begin(115200);
    Serial.println("Initializing...");
    EmLog::init(logPrint, EmLogLevel::info);
    app.setup();
}

void loop() {
    app.loop();
}
