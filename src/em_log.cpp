#include "em_log.h"

#ifndef EM_NO_LOG

EmLogLevel EmLog::g_Level = EmLogLevel::none;
EmLogTarget* EmLog::g_Targets = NULL;
uint8_t EmLog::g_TargetsCount = 0;

const char* levelToStr(EmLogLevel level) {
    switch (level) {
        case EmLogLevel::none: 
            return "None"; 
        case EmLogLevel::error:
            return "Error"; 
        case EmLogLevel::warning:
            return "Warning"; 
        case EmLogLevel::info:
            return "Info"; 
        case EmLogLevel::debug:
            return "Debug"; 
        case EmLogLevel::global: 
            // Avoid looping ;)
            if (EmLog::g_Level == EmLogLevel::global) {
                return "global";
            }
            return levelToStr(EmLog::g_Level); 
    }
    return "<unknown>";
}

void EmLog::log(EmLogLevel level, const char* msg) const { 
    if (checkLevel(level)) {
        writeToTargets_(level, m_Context, msg);
    }
}

void EmLog::log(EmLogLevel level, const __FlashStringHelper* msg) const { 
    if (checkLevel(level)) {
        writeToTargets_(level, m_Context, msg);
    }
}

void EmLog::log(EmLogLevel level, const char* context, const char* msg) { 
    if (g_Level >= level) {
        writeToTargets_(level, context, msg);
    }
}

void EmLog::log(EmLogLevel level, const char* context, const __FlashStringHelper* msg) { 
    if (g_Level >= level) {
        writeToTargets_(level, context, msg);
    }
}

void EmLog::writeToTargets_(EmLogLevel level, const char* context, const char* msg) { 
    for(uint8_t i=0; i<g_TargetsCount; i++) {
        g_Targets[i].write(level, context, msg);
    }
}

void EmLog::writeToTargets_(EmLogLevel level, 
                            const char* context, 
                            const __FlashStringHelper* msg) { 
    for(uint8_t i=0; i<g_TargetsCount; i++) {
        g_Targets[i].write(level, context, msg);
    }
}

#endif