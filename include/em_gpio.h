#include <stdio.h>
#include <Arduino.h>


double readAnalog(uint8_t pin, uint16_t iterations, uint16_t iterationDelayMs) {
    uint16_t min = 65535, max = 0;
    uint32_t sum = 0;
    for (uint16_t i = 0; i < iterations; i++) {
        delay(i == 0 ? 0 :iterationDelayMs);
        uint16_t adc = analogRead(pin);
        sum += adc;
        if (adc < min) min = adc;
        if (adc > max) max = adc;
    }
    return static_cast<double>(sum - max - min)/static_cast<double>(iterations-1);
}