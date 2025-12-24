#ifndef _EM_SERIAL_H__
#define _EM_SERIAL_H__

#include <Arduino.h>
#include <HardwareSerial.h>

#include "em_defs.h"

#ifdef EM_ESP
#include "driver/uart.h"
#endif

// The abstract serial stream class used by devices that need a serial communication
class EmSerialStream 
{
public:
#ifdef EM_HW_SERIAL_AVR
	virtual void begin(unsigned long baud, uint32_t config=SERIAL_8N1) = 0;
#else
	virtual void begin(unsigned long baud, 
                       uint32_t config=SERIAL_8N1, 
                       int8_t rxPin=-1, 
                       int8_t txPin=-1, 
                       bool invert=false, 
                       unsigned long timeout_ms = 20000UL, 
                       uint8_t rxfifo_full_thrhd = 112) = 0;
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
    virtual uint32_t baudRate() = 0;
};

// The hardware serial implementation
class EmHardwareSerial: public HardwareSerial, public EmSerialStream {
public:
    using HardwareSerial::HardwareSerial;

    // Explicitly implement the pure virtual methods from EmSerialStream/Stream
#ifdef EM_HW_SERIAL_AVR
    void begin(unsigned long baud, uint32_t config=SERIAL_8N1) override {
        m_baud = baud;
        HardwareSerial::begin(baud, config);
    }
#else
    void begin(unsigned long baud, 
               uint32_t config=SERIAL_8N1, 
               int8_t rxPin=-1, 
               int8_t txPin=-1, 
               bool invert=false, 
               unsigned long timeout_ms = 20000UL, 
               uint8_t rxfifo_full_thrhd = 112) override {
        HardwareSerial::begin(baud, config, rxPin, txPin, invert, timeout_ms, rxfifo_full_thrhd);
    }
#endif    

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
    size_t write(unsigned char byte) override { 
        return HardwareSerial::write(byte); 
    }
    size_t write(const char* text) override { 
        return HardwareSerial::write(text);
    }
    size_t write(const char *buffer, int buffLen) override { 
        return HardwareSerial::write(buffer, buffLen); 
    }
    void flush() override { 
        HardwareSerial::flush(); 
    }

#ifdef EM_HW_SERIAL_AVR
    void flush(bool txOnly) override { 
        HardwareSerial::flush(); 
        if (!txOnly) {
            flushRxBuffer();
        }   
    }

    void flushRxBuffer() override {
        // macro to guard critical sections when needed for large RX buffer sizes
        #if (SERIAL_RX_BUFFER_SIZE>256)
        #define RX_BUFFER_ATOMIC ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
        #else
        #define RX_BUFFER_ATOMIC
        #endif
        RX_BUFFER_ATOMIC {_rx_buffer_head = _rx_buffer_tail;}
    }    

    virtual uint32_t baudRate() override {
        return m_baud;
    }
#else
    void flush(bool txOnly) override { 
        HardwareSerial::flush(txOnly); 
    }

    void flushRxBuffer() override {
        // NOTE: tried to use 'uart_flush_input' but it goes too deep into esp32 implementation!
        int n;
        while (n = available() > 0){
            while (n--) read();
        }
    }

    virtual uint32_t baudRate() override {
        return HardwareSerial::baudRate();
    }
#endif        

#ifdef EM_HW_SERIAL_AVR
protected:
    unsigned long m_baud;
#endif
};

#endif