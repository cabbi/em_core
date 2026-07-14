#ifndef __EM_STREAM_H_
#define __EM_STREAM_H_

#ifdef ARDUINO
    #include <Arduino.h>
#elif ESP_PLATFORM
    #include <string>
    #include <cstdint>
    #include <stddef.h>
    #include "driver/uart.h"

    class Print {
    public:
        virtual size_t write(uint8_t c) = 0;
        
        size_t print(const std::string &s) {
            for (char c : s) write(c);
            return s.length();
        }
        size_t print(int n) { return print(std::to_string(n)); }
        
        size_t println(const std::string &s) { return print(s + "\n"); }
        size_t println(int n) { return println(std::to_string(n)); }
    };

    class Stream : public Print {
    public:
        virtual int available() = 0;
        virtual int read() = 0;
        virtual int peek() = 0;
    };

    class ESPIDFSerial : public Stream {
    private:
        uart_port_t _uart_num;
        static const int RX_BUF_SIZE = 1024;

    public:
        ESPIDFSerial(uart_port_t uart_num = UART_NUM_0) : _uart_num(uart_num) {}

        void begin(unsigned long baud, int tx_pin = 1, int rx_pin = 3) {
            uart_config_t uart_config = {
                .baud_rate = (int)baud,
                .data_bits = UART_DATA_8_BITS,
                .parity    = UART_PARITY_DISABLE,
                .stop_bits = UART_STOP_BITS_1,
                .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
                .source_clk = UART_SCLK_DEFAULT,
            };
            
            uart_param_config(_uart_num, &uart_config);
            uart_set_pin(_uart_num, tx_pin, rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
            uart_driver_install(_uart_num, RX_BUF_SIZE, 0, 0, NULL, 0);
        }

        size_t write(uint8_t c) override {
            uart_write_bytes(_uart_num, (const char*)&c, 1);
            return 1;
        }

        int available() override {
            size_t length = 0;
            uart_get_buffered_data_len(_uart_num, &length);
            return (int)length;
        }

        int read() override {
            uint8_t ch;
            int rxBytes = uart_read_bytes(_uart_num, &ch, 1, 0);
            if (rxBytes > 0) {
                return ch;
            }
            return -1;
        }

        int peek() override { return -1; }
    };

    extern ESPIDFSerial Serial;

#endif

#endif //__EM_STREAM_H_