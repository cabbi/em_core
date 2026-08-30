#ifndef __EM_DEFS__H_
#define __EM_DEFS__H_
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <math.h>

#ifdef ARDUINO
    // This code compiles ONLY if you are using the Arduino framework
    #include <Arduino.h>
#elif ESP_PLATFORM
    // This code compiles if you are using ESP-IDF
#endif

#if defined(ESP_PLATFORM) || defined(ESP32) || defined(ESP8266)
    #include <freertos/FreeRTOS.h>
    #include <freertos/task.h>
    
    #define EM_ESP
    #define EM_STD_LIB  // Use of standard library (AVR arduinos does not have it!)
    #define EM_WIFI
    #define EM_BLE
    #define EM_MULTITHREAD
    #define EM_NVS
    #define EM_TIME
#ifdef CONFIG_IDF_TARGET_ESP32S3
    #define EM_MULTICORE  
    #define EM_CORES_COUNT 2
#else
    #define EM_CORES_COUNT 1
#endif

    enum class EmCoreId: uint8_t {
        core0 = 0, 
        // In general, system tasks are pinned to core 0 and user tasks to core 1
        coreSystemTask = 0,
    #ifdef EM_MULTICORE
        core1 = 1,
        coreUserTask = 1,
    #else
        coreUserTask = 0, // User & system tasks run on single core 
    #endif
    };

    // Feed the watchdog to prevent it from resetting the system
    // This is needed when the task is paused for a long time (e.g. waiting for WiFi connection)
    // or when the task function execution takes a long time (e.g. performing a long operation)
    //
    // The 'yieldIfNoTicks' set to true ensures the task is yield so the watchdog is refreshed.
    inline void tDelay(uint32_t pauseMs, bool yieldIfNoTicks) {
        TickType_t ticks = pdMS_TO_TICKS(pauseMs);
        if (ticks > 0) {
            vTaskDelay(ticks);
        } else 
        if (yieldIfNoTicks) {
            taskYIELD();
        }
    }

#else
    #define EM_CORES_COUNT 1
    enum class EmCoreId: uint8_t {
        core0 = 0, 
        coresystemTask = 0,
        coreUserTask = 0,
    };

    inline void tDelay(uint32_t ms, bool delayAtLeastOneTick) {
        //delay(ms);        
    }
#endif

#ifndef F
    #define F(x) (x)  
#endif

#if defined(AVR)
    #define EM_EEPROM
    #define EM_HW_SERIAL_AVR
#endif


#ifdef ARDUINO
    #if defined(ARDUINO_NANO_ESP32)
        #define USB_SERIAL_CLASS USBCDC
    #elif defined(ARDUINO_ESP32C3_DEV) || defined(ARDUINO_ESP32S3_DEV)
        #define USB_SERIAL_CLASS HWCDC
    #else
        #define USB_SERIAL_CLASS HardwareSerial
    #endif  

    #ifdef EM_ESP
        inline void restart() {
            ESP.restart();
        }
    #endif
  
#elif ESP_PLATFORM
    #include "esp_system.h"
    #include <esp_timer.h>
    #include "em_usb_serial.h"

    extern "C" {
    inline uint32_t millis() {
        return (uint32_t)(esp_timer_get_time() / 1000);
    }
    }
    inline void restart() {
        esp_restart();
    }

    class EmUsbSerial;
    #define USB_SERIAL_CLASS EmUsbSerial
    // The Arduino like Serial global object
    extern USB_SERIAL_CLASS Serial;
#endif    


#if __cplusplus >= 201402L
    #define DEPRECATED          [[deprecated]]
    #define DEPRECATED_MSG(msg) [[deprecated(msg)]]
#else
    #define DEPRECATED
    #define DEPRECATED_MSG(msg)
#endif

#define SIZE_OF(x) (sizeof((x))/sizeof((x[0])))

#ifndef MIN
#define MIN(x, y) ((x)<(y) ? (x) : (y))
#endif
#ifndef MAX
#define MAX(x, y) ((x)>(y) ? (x) : (y))
#endif

// The explicit epoch type using 32 bits only
// By using an uint32_t, the epoch will overflow on February 7, 2106
struct EmEpoch32 {
    uint32_t value;

    EmEpoch32() : value(0) {}
    explicit EmEpoch32(uint32_t val) : value(val) {}

    static EmEpoch32 Zero() { return EmEpoch32(0); }
    static EmEpoch32 Invalid() { return EmEpoch32(0); }

    // Explicit conversion operator 
    explicit operator uint32_t() const { return value; }

    bool isZero() const { return value == 0; }
    bool isInvalid() const { return value == 0; }
    bool isValid() const { return value > 0; }

    bool operator==(const EmEpoch32& other) const { return value == other.value; }
    bool operator!=(const EmEpoch32& other) const { return value != other.value; }
    bool operator< (const EmEpoch32& other) const { return value <  other.value; }
    bool operator<=(const EmEpoch32& other) const { return value <= other.value; }
    bool operator> (const EmEpoch32& other) const { return value >  other.value; }
    bool operator>=(const EmEpoch32& other) const { return value >= other.value; }

    EmEpoch32 operator+(uint32_t seconds) const {
        return EmEpoch32(value + seconds);
    }

    EmEpoch32 operator-(uint32_t seconds) const {
        return EmEpoch32(value - seconds);
    }

    EmEpoch32& operator+=(uint32_t seconds) {
        value += seconds;
        return *this;
    }

    EmEpoch32& operator-=(uint32_t seconds) {
        value -= seconds;
        return *this;
    }

    // Increment / Decrement
    EmEpoch32& operator++() { 
        ++value;
        return *this;
    }

    EmEpoch32 operator++(int) { 
        EmEpoch32 temp = *this;
        ++value;
        return temp;
    }

    EmEpoch32& operator--() { 
        --value;
        return *this;
    }

    EmEpoch32 operator--(int) { 
        EmEpoch32 temp = *this;
        --value;
        return temp;
    }
};

// Commutative helper: (seconds & epoch)
inline EmEpoch32 operator+(uint32_t seconds, const EmEpoch32& epoch) {
    return EmEpoch32(epoch.value + seconds);
}
inline EmEpoch32 operator-(uint32_t seconds, const EmEpoch32& epoch) {
    return EmEpoch32(epoch.value - seconds);
}


// Returns the power of 10`^ exp as an integer number
// This method will avoid using the "double pow10(...)" implementation
inline int32_t iPow10(size_t exp) {
    return static_cast<int32_t>(pow(10, static_cast<double>(exp)));
}

// Returns the integer rounded number
template <class real_type>
inline int32_t iRound(real_type num) {
    return num >= 0 ? static_cast<int32_t>(num+.5) : 
                      static_cast<int32_t>(num-0.5);
}

// Returns the integer multiplied number: num1 * num2
template <class real_type>
inline int32_t iMolt(real_type num1, real_type num2) {
    return static_cast<int32_t>(num1 * num2);
}

// Returns the integer division number: num1 / num2
template <class real_type>
inline int32_t iDiv(real_type num1, real_type num2) {
    return static_cast<int32_t>(num1 / num2);
}

// The abstract 'updatable' object class
class EmUpdatable {
public:
    virtual void update() = 0;
};

// Simple updater object
template <EmUpdatable* updatableObjects[], uint8_t size>
class EmUpdater {
public:
    void update() {
        for (uint8_t i=0; i < size; i++) {
            updatableObjects[i]->update();
        }
    }
};

// to_ptr function can be used in template classes that need both reference
// and pointer types implementation
template<typename T>
inline T* to_ptr(T& obj) { return &obj; }

template<typename T>
inline T* to_ptr(T* obj) { return obj; }

// to_str converts numbers to strings
inline const char* to_str(char* buf, size_t bufLen, uint8_t n) {
    snprintf(buf, bufLen, "%u", n);
    return buf;
}
inline const char* to_str(char* buf, size_t bufLen, uint16_t n) {
    snprintf(buf, bufLen, "%u", n);
    return buf;
}
inline const char* to_str(char* buf, size_t bufLen, uint32_t n) {
    snprintf(buf, bufLen, "%lu", n);
    return buf;
}

inline const char* to_str(char* buf, size_t bufLen, int8_t n) {
    snprintf(buf, bufLen, "%d", n);
    return buf;
}
inline const char* to_str(char* buf, size_t bufLen, int16_t n) {
    snprintf(buf, bufLen, "%d", n);
    return buf;
}
inline const char* to_str(char* buf, size_t bufLen, int32_t n) {
    snprintf(buf, bufLen, "%ld", n);
    return buf;
}

inline const char* to_str(char* buf, size_t bufLen, float n) {
    snprintf(buf, bufLen, "%g", n);
    return buf;
}

inline const char* to_str(char* buf, size_t bufLen, double n) {
    snprintf(buf, bufLen, "%g", n);
    return buf;
}


#endif