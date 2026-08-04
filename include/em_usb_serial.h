#ifndef _EM_USB_SERIAL_H__
#define _EM_USB_SERIAL_H__

#include "em_defs.h"
#include "em_stream.h"

#ifdef ARDUINO
// This code compiles ONLY if you are using the Arduino framework
#include <Arduino.h>

#elif ESP_PLATFORM

class EmUsbSerial : public EmSerialStream {
private:
    bool m_isInitialized;

public:
    EmUsbSerial() : m_isInitialized(false) {}

    virtual bool begin() {
        if (m_isInitialized) return true;

        // NOTHING NEEDED HERE!

        m_isInitialized = true;
        return true;
    }

   
    virtual bool isInitialized() const { return m_isInitialized; }
    virtual void end() override { m_isInitialized = false; }
    
    virtual size_t write(unsigned char byte) override {
        if (!m_isInitialized) return -1;
        size_t w = fwrite(&byte, 1, 1, stdout);
        fflush(stdout);
        return w;
    }
    
    virtual size_t write(const void* buffer, int buffLen) override {
        if (!m_isInitialized || buffLen <= 0) return 0;
        size_t w = fwrite(buffer, 1, buffLen, stdout);
        fflush(stdout);
        return w;
    }

    virtual int available() override { return 0; }
    virtual int peek() override { return -1; }
    virtual int read() override { return -1; }
    virtual size_t read(uint8_t*, size_t) { return -1; }  
    virtual void flush(bool txOnly=true) override { fflush(stdout); }
    virtual void flushRxBuffer() override {}
    virtual int baudRate() override { return 115200; }

protected:        
    virtual bool begin(unsigned long baud, int8_t rxPin=-1, int8_t txPin=-1) override {
        return false;
    }
    virtual bool begin(const uart_config_t& uart_config, int8_t rxPin=-1, int8_t txPin=-1) override {
        return false;
    }

};
#endif
#endif //_EM_USB_SERIAL_H__