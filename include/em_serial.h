#ifndef _EM_SERIAL_H__
#define _EM_SERIAL_H__

#include "em_defs.h"
#include "em_stream.h"

#ifdef ARDUINO
// This code compiles ONLY if you are using the Arduino framework
#include <Arduino.h>

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
               int8_t txPin=-1) override {
        HardwareSerial::begin(baud, config, rxPin, txPin);
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

    virtual int baudRate() override {
        return (int)HardwareSerial::baudRate();
    }
#endif        

#ifdef EM_HW_SERIAL_AVR
protected:
    unsigned long m_baud;
#endif
};

#elif ESP_PLATFORM
#include "driver/uart.h"

class EmHardwareSerial : public EmSerialStream, public EmPrint {
private:
    uart_port_t m_uartNum;
    bool m_isInitialized;
    static const int RX_BUF_SIZE = 1024;

public:
    EmHardwareSerial(uart_port_t uart_num = UART_NUM_0)
     : m_uartNum(uart_num), m_isInitialized(false) {}

     EmHardwareSerial(int uart_num)
     : EmHardwareSerial(static_cast<uart_port_t>(uart_num)) {}

	virtual bool begin(unsigned long baud, int8_t rxPin=-1, int8_t txPin=-1) override;
	virtual bool begin(const uart_config_t& uart_config, int8_t rxPin=-1, int8_t txPin=-1) override;
    
    virtual bool isInitialized() const {
        return m_isInitialized;
    }
	virtual void end() override;
    virtual int available() override;
    virtual int peek() { 
        return -1; 
    }
    virtual int read() override;
    virtual size_t read(uint8_t* buffer, size_t length) override {
        return read(buffer, length, 100);
    }
    virtual size_t read(uint8_t* buffer, size_t length, uint32_t timeout_ms);
 	virtual size_t write(unsigned char byte) override;
	virtual size_t write(const char* text) override;
	virtual size_t write(const char *buffer, int buffLen) override;
	virtual void flush(bool txOnly=true) override;
    virtual void flushRxBuffer() override;
    virtual int baudRate() override;
};

extern EmHardwareSerial Serial;

#endif
#endif //_EM_SERIAL_H__