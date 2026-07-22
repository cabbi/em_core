#include "em_serial.h"
#include <em_log.h>

#ifdef ESP_PLATFORM

bool EmHardwareSerial::begin(unsigned long baud, int8_t rxPin, int8_t txPin) {
    if (isInitialized()) {
        return true;
    }
    uart_config_t uart_config = {}; // Zero-initializes everything safely
    uart_config.baud_rate = (int)baud;
    uart_config.data_bits = UART_DATA_8_BITS;
    uart_config.parity    = UART_PARITY_DISABLE;
    uart_config.stop_bits = UART_STOP_BITS_1;
    uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    uart_config.source_clk = UART_SCLK_DEFAULT;
    return begin(uart_config, rxPin, txPin);
}

bool EmHardwareSerial::begin(const uart_config_t& uart_config, int8_t rxPin, int8_t txPin) {
    if (isInitialized()) {
        return true;
    }
    esp_err_t res = uart_param_config(m_uartNum, &uart_config);
    if (res != ESP_OK) {
        logError<100>("EmHardwareSerial", "[%d] 'uart_param_config' failed!", res);
        return false;
    }
    res = uart_set_pin(m_uartNum, txPin, rxPin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (res != ESP_OK) {
        logError<100>("EmHardwareSerial", "[%d] 'uart_set_pin' failed!", res);
        return false;
    }
    res = uart_driver_install(m_uartNum, RX_BUF_SIZE, 0, 0, NULL, 0);
    if (res != ESP_OK) {
        logError<100>("EmHardwareSerial", "[%d] 'uart_driver_install' failed!", res);
        return false;
    }
    m_isInitialized = true;
    return true;
}

size_t EmHardwareSerial::write(unsigned char c) {
    if (!isInitialized()) {
        return -1;
    }
    return uart_write_bytes(m_uartNum, (const char*)&c, 1);
}

size_t EmHardwareSerial::write(const char* text) {
    if (!isInitialized()) {
        return -1;
    }
    return uart_write_bytes(m_uartNum, text, strlen(text));
}

size_t EmHardwareSerial::write(const char *buffer, int buffLen) {
    if (!isInitialized()) {
        return -1;
    }
    return uart_write_bytes(m_uartNum, buffer, buffLen);
}

void EmHardwareSerial::flushRxBuffer() {
    if (!isInitialized()) {
        return;
    }
    
    // Clear the internal ESP-IDF ring buffer and hardware FIFO
    esp_err_t err = uart_flush_input(m_uartNum);
    if (err != ESP_OK) {
        ESP_LOGE("EspSerial", "Failed to flush RX buffer");
    }
}

int EmHardwareSerial::baudRate() {
    if (!isInitialized()) {
        return -1;
    }

    uint32_t current_baud = 0;
    
    // Natively query the ESP32 hardware driver registers
    esp_err_t err = uart_get_baudrate(m_uartNum, &current_baud);
    if (err != ESP_OK) {
        return -1;
    }

    return (int)current_baud;
}

int EmHardwareSerial::available() {
    if (!isInitialized()) {
        return -1;
    }
    size_t length = 0;
    uart_get_buffered_data_len(m_uartNum, &length);
    return (int)length;
}

int EmHardwareSerial::read() {
    if (!isInitialized()) {
        return -1;
    }
    uint8_t ch;
    int rxBytes = uart_read_bytes(m_uartNum, &ch, 1, 0);
    if (rxBytes > 0) {
        return ch;
    }
    return -1;
}

size_t EmHardwareSerial::read(uint8_t* buffer, size_t length, uint32_t timeout_ms) {
    if (!isInitialized() || buffer == nullptr || length == 0) {
        return 0;
    }

    TickType_t ticks_to_wait = timeout_ms / portTICK_PERIOD_MS;
    if (timeout_ms > 0 && ticks_to_wait == 0) {
        ticks_to_wait = 1; // Ensure at least 1 tick wait if timeout is small but non-zero
    }

    int bytes_read = uart_read_bytes(m_uartNum, buffer, length, ticks_to_wait);
    
    if (bytes_read < 0) {
        return 0;
    }

    return (size_t)bytes_read;
}

void EmHardwareSerial::flush(bool txOnly) {
    if (!isInitialized()) {
        return;
    }
    if (!txOnly) {
        while (read() >= 0);
    }
    uart_wait_tx_done(m_uartNum, portMAX_DELAY);
}

void EmHardwareSerial::end() {
    if (!isInitialized()) {
        return;
    }
    flush();
    uart_driver_delete(m_uartNum);
    m_isInitialized = false;
}

#endif //ESP_PLATFORM