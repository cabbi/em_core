#include "em_log_serial.h"


void EmLogSerialBase::write(EmLogLevel level, 
                            const char* context, 
                            const char* msg) {
    _printLevel(level);
    if (context != NULL) {
        print(context);print(" - ");
    }
    println(msg);
}

void EmLogSerialBase::write(EmLogLevel level, 
                            const char* context, 
                            const __FlashStringHelper* msg) {
    _printLevel(level);
    if (context != NULL) {
        print(context);print(" - ");
    }
    println(msg);
}

void EmLogSerialBase::_printLevel(EmLogLevel level) {
    print("[");print(LevelToStr(level));print("] ");
}