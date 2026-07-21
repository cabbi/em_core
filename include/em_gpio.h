#ifndef __EM_GPIO_H_
#define __EM_GPIO_H_

#ifdef ARDUINO
#include <Arduino.h>
#elif ESP_PLATFORM
#include <stdint.h>

// Define Arduino-style constants
#define INPUT        0
#define OUTPUT       1
#define INPUT_PULLUP 2
#define LOW          0
#define HIGH         1

void pinMode(int pin, int mode);
void digitalWrite(int pin, int level);
int digitalRead(int pin);
int analogRead(int pin);

#endif

template<class T=float>
T readAnalog(uint8_t pin, uint16_t iterations, uint16_t iterationDelayMs);

#endif //__EM_GPIO_H_