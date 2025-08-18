#ifndef __APP__H_
#define __APP__H_

#include "em_defs.h"
#include "em_log.h"
#include "em_app_interface.h"

class EmApp: public EmLog
{
public:
    EmApp(const char* logContext = "App", 
          EmLogLevel logLevel = EmLogLevel::global) 
     : EmLog(logContext, logLevel), m_Interfaces() {};
    
    virtual ~EmApp() {
        m_Interfaces.clear();
    }

    void addInterface(EmAppInterface& interface) {
        m_Interfaces.append(interface);
    }

    void run(uint32_t loopDelayMillis);

protected:
    EmAppInterfaces m_Interfaces;
};

#endif