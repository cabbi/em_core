#ifndef __EM_STREAM_H_
#define __EM_STREAM_H_

#ifdef ARDUINO
    #include <Arduino.h>
    #include <HardwareSerial.h>
#elif ESP_PLATFORM
    #include "driver/uart.h"
#endif

#include <string>
#include <cstdint>
#include <stddef.h>

class EmPrint {
public:
    virtual size_t write(uint8_t c) = 0;
    
    size_t print(const char* s) {
        size_t len = strlen(s);
        for (int i=0; i<len; i++) write(s[i]);
        return len;
    }
    size_t print(int n) { return printf("%d", n); }
    
    size_t println() { return print("\n"); }
    size_t println(const char* s) { size_t n = print(s); print("\n"); return n+1; }
    size_t println(int n) { size_t len = print(n); print("\n"); return len+1; }
    size_t printf(const char *format, ...);
};

class EmStream {
public:
    virtual int available() = 0;
    virtual int peek() = 0;
    virtual int read() = 0;
    virtual size_t read(uint8_t*, size_t) = 0;
    virtual size_t readBytes(char *buffer, size_t length) {
        return read((uint8_t*)buffer, length);
    }
    virtual size_t readBytes(uint8_t *buffer, size_t length) {
        return read(buffer, length);
    }

};

// The abstract serial stream class used by devices that need a serial communication
class EmSerialStream: public EmStream 
{
public:
#ifdef EM_HW_SERIAL_AVR
	virtual void begin(unsigned long baud, uint32_t config=SERIAL_8N1) = 0;
#elif ARDUINO
	virtual void begin(unsigned long baud, 
                       uint32_t config=SERIAL_8N1, 
                       int8_t rxPin=-1, 
                       int8_t txPin=-1) = 0;
#elif ESP_PLATFORM
	virtual bool begin(unsigned long baud, int8_t rxPin=-1, int8_t txPin=-1) = 0;
	virtual bool begin(const uart_config_t& uart_config, int8_t rxPin=-1, int8_t txPin=-1) = 0;
#endif        
	virtual void end() = 0;
    virtual int available() = 0;
    virtual int read() = 0;
    virtual int peek() = 0;
	virtual size_t write(unsigned char byte) = 0;
	virtual size_t write(const char* text) = 0;
	virtual size_t write(const char *buffer, int buffLen) = 0;
	virtual void flush(bool txOnly=true) = 0;
    virtual void flushRxBuffer() = 0;
    virtual int baudRate() = 0;
};

#endif //__EM_STREAM_H_