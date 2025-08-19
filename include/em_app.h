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
     : EmLog(logContext, logLevel), m_interfaces() {};
    
    virtual ~EmApp() {
        m_interfaces.clear();
    }

    void addInterface(EmAppInterface& interface) {
        m_interfaces.append(interface);
    }

    void run(uint32_t loopDelayMillis);

protected:
    EmAppInterfaces m_interfaces;
};

#endif