#ifndef __LOG_SERIAL__H_
#define __LOG_SERIAL__H_

#include <Arduino.h>
#include <SoftwareSerial.h>
#include "em_log.h"

// The Abstract Serial log target class
class EmLogSerialBase: public EmLogTarget {
public:   
    virtual void write(EmLogLevel level, 
                       const char* context, 
                       const char* msg);

    virtual size_t print(const char[]) = 0;
    virtual size_t println(const char[]) = 0;
};

// The Hardware Serial log target
class EmLogHwSerial: public EmLogSerialBase  {
public:   
    EmLogHwSerial(HardwareSerial& serial) : m_Serial(serial) {}

    void begin(unsigned long baud) {
        m_Serial.begin(baud);
    }

    virtual size_t print(const char msg[]) {
        return m_Serial.print(msg);
    }

    virtual size_t println(const char msg[]) {
        return m_Serial.println(msg);
    }

protected:
    HardwareSerial& m_Serial;
};

// The Software Serial log target
class EmLogSwSerial: public EmLogSerialBase, public SoftwareSerial {
public:    
    // Inherit base class constructors
    using SoftwareSerial::SoftwareSerial;
};


#endif