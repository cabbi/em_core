#ifndef __LOG_SERIAL__H_
#define __LOG_SERIAL__H_

#include <Arduino.h>
#include "em_log.h"

// The Abstract Serial log target class
class EmLogSerialBase: public HardwareSerial, EmLogTarget {
public:   
    virtual void write(EmLogLevel level, 
                       const char* context, 
                       const char* msg) {
        print("[");print(LevelToStr(level));print("] ");
        if (context != NULL) {
            print(context);print(" - ");
        }
        println(msg);
    }

    virtual void println(const char*) = 0;
    virtual void print(const char*) = 0;
};

// The Hardware Serial log target
class EmLogHwSerial: public HardwareSerial, EmLogSerialBase {
public:   
    // Inherit base class constructors
    using HardwareSerial::HardwareSerial;
};

// The Software Serial log target
class EmLogSwSerial: public SoftwareSerial, EmLogSerialBase {
public:    
    // Inherit base class constructors
    using SoftwareSerial::SoftwareSerial;
};


#endif