#include "em_stream.h"

#if defined(ESP_PLATFORM) && !defined(ARDUINO)
    ESPIDFSerial Serial(UART_NUM_0);
#endif