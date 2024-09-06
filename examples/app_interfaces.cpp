#include <stdio.h>
#include "em_app.h"

uint8_t appRestartCounter = 0;

class Int1: public EmAppInterface {
public:
    Int1(uint32_t runningTimeoutMs, bool logEnabled=false)
     : EmAppInterface(runningTimeoutMs, logEnabled), 
       m_Counter(0) {}
    
    virtual const char* Name() const {
        return "int1";
    }

    virtual EmIntOperationResult Setup() {
        m_Counter = 0;
        return EmIntOperationResult::canContinue;
    }

    virtual EmIntOperationResult Loop() {
        m_Counter++;
        printf("Counter 1: %d\n", m_Counter);
        if (m_Counter >= 10) {
            printf("Interface 1 is exiting!\n");
            return EmIntOperationResult::removeInterface;    
        }
        return EmIntOperationResult::canContinue;    
    }

private:
    int m_Counter;
};

class Int2: public EmAppInterface {
public:
    Int2(uint32_t runningTimeoutMs, bool logEnabled=false)
     : EmAppInterface(runningTimeoutMs, logEnabled), 
       m_Counter(0) {}
    
    virtual const char* Name() const {
        return "int2";
    }

    virtual EmIntOperationResult Setup() {
        m_Counter = 0;
        return EmIntOperationResult::canContinue;
    }

    virtual EmIntOperationResult Loop() {
        m_Counter++;
        printf("Counter 2: %d\n", m_Counter);
        if (m_Counter >= 20) {
            appRestartCounter++;
            if (appRestartCounter>=2) {
                printf("This is enough, exiting app!\n");
                return EmIntOperationResult::exitApp;    
            }
            printf("Interface 2 restarting app!\n");
            return EmIntOperationResult::restartApp;    
        }
        return EmIntOperationResult::canContinue;    
    }

private:
    int m_Counter;
};

int main() {
    EmApp app;
    Int1 int1(1000);
    Int2 int2(1000);
    app.AddInterface(int1);
    app.AddInterface(int2);

    app.Run(1000);
    
    return 0;
}
