#ifndef __APP__H_
#define __APP__H_

#include "em_app_interface.h"

class EmApp
{
public:
    EmApp() : m_Interfaces(EmAppInterfaces()) {};
    
    virtual ~EmApp() {
        m_Interfaces.Clear();
    }

    void AddInterface(EmAppInterface& interface) {
        m_Interfaces.Append(interface);
    }

    void Run(uint32_t loopDelayMs);

private:
    EmAppInterfaces m_Interfaces;
};

#endif