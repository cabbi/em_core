#include "em_log_serial.h"


void EmLogSerialBase::write(EmLogLevel level, 
                            const char* context, 
                            const char* msg) {
    print("[");print(LevelToStr(level));print("] ");
    if (context != NULL) {
        print(context);print(" - ");
    }
    println(msg);
}
