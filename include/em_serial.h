#ifndef _EM_SERIAL_H__
#define _EM_SERIAL_H__

#include <Arduino.h>


// The abstract serial stream class used by devices that need a serial communication
class EmSerialStream 
{
public:
	virtual void begin(unsigned long baud) = 0;
	virtual void begin(unsigned long baud, 
                       uint32_t config, 
                       int8_t rxPin=-1, 
                       int8_t txPin=-1, 
                       bool invert=false, 
                       unsigned long timeout_ms = 20000UL, 
                       uint8_t rxfifo_full_thrhd = 112) = 0;
	virtual void end() = 0;
    virtual int available() = 0;
    virtual int read() = 0;
    virtual int peek() = 0;
	virtual int write(unsigned char byte) = 0;
	virtual int write(const char* text) = 0;
	virtual int write(const char *buffer, int buffLen) = 0;
	virtual void flush() = 0;
};

// The hardware serial implementation
class EmHardwareSerial: public HardwareSerial, public EmSerialStream {
public:
    using HardwareSerial::HardwareSerial;

    // Explicitly implement the pure virtual methods from EmSerialStream/Stream
    void begin(unsigned long baud) override { 
        HardwareSerial::begin(baud); 
    }
    void begin(unsigned long baud, uint32_t config, int8_t rxPin, int8_t txPin, bool invert, unsigned long timeout_ms, uint8_t rxfifo_full_thrhd) override {
        HardwareSerial::begin(baud, config, rxPin, txPin, invert, timeout_ms, rxfifo_full_thrhd);
    }
    void end() override { 
        HardwareSerial::end(); 
    }
    int available() override { 
        return HardwareSerial::available(); 
    }
    int read() override { 
        return HardwareSerial::read(); 
    }
    int peek() override { 
        return HardwareSerial::peek(); 
    }
    int write(unsigned char byte) override { 
        return HardwareSerial::write(byte); 
    }
    int write(const char* text) override { 
        return HardwareSerial::write(text);
    }
    int write(const char *buffer, int buffLen) override { 
        return HardwareSerial::write(buffer, buffLen); 
    }
    void flush() override { 
        HardwareSerial::flush(); 
    }
};

#endif