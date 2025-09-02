#include "em_app.h"

uint8_t appRestartCounter = 0;

class Int1: public EmAppTimeoutInterface {
public:
    Int1(EmDuration runningTimeout, EmLogLevel logLevel)
     : EmAppTimeoutInterface(EmDuration(0,0,1), true, runningTimeout, logLevel), 
       m_Counter(0) {}

    virtual const char* name() const {
        return "int1";
    }

    virtual EmIntOperationResult setup() {
        m_Counter = 0;
        return EmIntOperationResult::canContinue;
    }

    virtual EmIntOperationResult loop() {
        m_Counter++;
        printf("Counter 1: %d\n", m_Counter);
        if (m_Counter >= 10) {
            printf("Interface 1 is exiting!\n");
            return EmIntOperationResult::stopInterface;    
        }
        return EmIntOperationResult::canContinue;    
    }

private:
    int m_Counter;
};

class Int2: public EmAppTimeoutInterface {
public:
    Int2(EmDuration runningTimeout, EmLogLevel logLevel)
     : EmAppTimeoutInterface(EmDuration(0,0,2), true, runningTimeout, logLevel), 
       m_Counter(0) {}
    
    virtual const char* name() const {
        return "int2";
    }

    virtual EmIntOperationResult setup() {
        m_Counter = 0;
        return EmIntOperationResult::canContinue;
    }

    virtual EmIntOperationResult loop() {
        m_Counter++;
        printf("Counter 2: %d\n", m_Counter);
        if (m_Counter >= 10) {
            appRestartCounter++;
            if (appRestartCounter>=2) {
                printf("This is enough, exiting app!\n");
                return EmIntOperationResult::stopApp;    
            }
            printf("Interface 2 restarting app!\n");
            return EmIntOperationResult::restartApp;    
        }
        return EmIntOperationResult::canContinue;    
    }

private:
    int m_Counter;
};

EmApp app;

void setup() {
    Int1 int1(EmDuration(0,1,0), EmLogLevel::info);
    Int2 int2(EmDuration(0,1,0), EmLogLevel::info);
    app.addInterface(int1);
    app.addInterface(int2);
    app.setup();
}

void loop() {
    app.loop();
}
