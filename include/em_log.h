#ifndef __LOG__H_
#define __LOG__H_

#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>

// The logging enabled levels
enum class EmLogLevel: int8_t {
    global = -1, // Takes the EmLog::g_Level
    none = 0,
    error,
    warning,
    info,
    debug
};

// Forward declarations
class __FlashStringHelper;
const char* LevelToStr(EmLogLevel level);

// The base log target class each logging target should implement. 
class EmLogTarget {
public:    
    virtual void write(EmLogLevel /*level*/, 
                       const char* /*context*/, 
                       const char* /*msg*/) {}

    virtual void write(EmLogLevel /*level*/, 
                       const char* /*context*/, 
                       const __FlashStringHelper* /*msg*/) {}
};


// NOTE:
//  Define 'EM_NO_LOG' to avoid extra Flash and RAM memory consumption.  
#ifdef EM_NO_LOG

inline const char* LevelToStr(EmLogLevel level) { return ""; }

class EmLog {
public:    
    static void Init(EmLogTarget targets[], uint8_t targetsCount, EmLogLevel level) {}

    EmLog(const char* context = NULL, 
          EmLogLevel level = EmLogLevel::global) {}

    template<uint8_t max_len>
    void LogError(const char* format, ...) const {}
    void LogError(const char* msg) const {}
    void LogError(const __FlashStringHelper* msg) const {}

    template<uint8_t max_len>
    void LogWarning(const char* format, ...) const {}
    void LogWarning(const char* msg) const {}
    void LogWarning(const __FlashStringHelper* msg) const {}

    template<uint8_t max_len>
    void LogInfo(const char* format, ...) const {}
    void LogInfo(const char* msg) const {}
    void LogInfo(const __FlashStringHelper* msg) const {}

    template<uint8_t max_len>
    void LogDebug(const char* format, ...) const {} 
    void LogDebug(const char* msg) const {}
    void LogDebug(const __FlashStringHelper* msg) const {}

    template<uint8_t max_len>
    void Log(EmLogLevel level, const char* format, ...) const {} 
    void Log(EmLogLevel level, const char* msg) const {} 
    void Log(EmLogLevel level, const __FlashStringHelper* msg) const {} 
    
    template<uint8_t max_len>
    static void Log(EmLogLevel level, const char* context, const char* msg, ...) {} 
    static void Log(EmLogLevel level, const char* context, const char* msg) {} 
    static void Log(EmLogLevel level, const char* context, const __FlashStringHelper* msg) {} 
    
    bool CheckLevel(EmLogLevel level) const { return false; }

    void SetLevel(EmLogLevel level) {}

    static void SetGlobalLevel(EmLogLevel level) {}
};

#else

// The log class can be inherited to allow easy logging
class EmLog {
    friend const char* LevelToStr(EmLogLevel level);
public:    
    static void Init(EmLogTarget& target, EmLogLevel level) {
        EmLog::g_Targets = &target;
        EmLog::g_TargetsCount = 1;
        EmLog::g_Level = level;
    }

    static void Init(EmLogTarget targets[], uint8_t targetsCount, EmLogLevel level) {
        EmLog::g_Targets = targets;
        EmLog::g_TargetsCount = targetsCount;
        EmLog::g_Level = level;
    }

    EmLog(const char* context = NULL, 
          EmLogLevel level = EmLogLevel::global)
     : m_Context(context),
       m_Level(level) { }

    template<uint8_t max_len>
    void logError(const char* format, ...) const;
    void logError(const char* msg) const { 
        log(EmLogLevel::error, msg);
    }
    void logError(const __FlashStringHelper* msg) const { 
        log(EmLogLevel::error, msg);
    }

    template<uint8_t max_len>
    void logWarning(const char* format, ...) const;
    void logWarning(const char* msg) const { 
        log(EmLogLevel::warning, msg);
    }
    void logWarning(const __FlashStringHelper* msg) const { 
        log(EmLogLevel::warning, msg);
    }

    template<uint8_t max_len>
    void logInfo(const char* format, ...) const;
    void logInfo(const char* msg) const { 
        log(EmLogLevel::info, msg);
    }
    void logInfo(const __FlashStringHelper* msg) const { 
        log(EmLogLevel::info, msg);
    }

    template<uint8_t max_len>
    void logDebug(const char* format, ...) const;
    void logDebug(const char* msg) const { 
        log(EmLogLevel::debug, msg);
    }
    void logDebug(const __FlashStringHelper* msg) const { 
        log(EmLogLevel::debug, msg);
    }

    template<uint8_t max_len>
    void log(EmLogLevel level, const char* format, ...) const;
    void log(EmLogLevel level, const char* msg) const;
    void log(EmLogLevel level, const __FlashStringHelper* msg) const;
    
    template<uint8_t max_len>
    static void log(EmLogLevel level, const char* context, const char* format, ...);
    static void log(EmLogLevel level, const char* context, const char* msg);
    static void log(EmLogLevel level, const char* context, const __FlashStringHelper* msg);
    
    bool checkLevel(EmLogLevel level) const { 
        return (m_Level == EmLogLevel::global ? g_Level : m_Level) >= level; 
    }

    void setLevel(EmLogLevel level) { 
        // Since we are running on a 1 Core CPU this operation might be atomic 
        // (i.e. no thread sync needed since we use 1 byte and we are running on a single CPU)
        m_Level = level; 
    }

    static void setGlobalLevel(EmLogLevel level) { 
        // Since we are running on a 1 Core CPU this operation might be atomic 
        // (i.e. no thread sync needed since we use 1 byte and we are running on a single CPU)
        g_Level = level; 
    }

protected:
    template<uint8_t max_len>
    static void _writeToTargets(EmLogLevel level, 
                                const char* context, 
                                const char* format,
                                va_list args); 
    static void _writeToTargets(EmLogLevel level, 
                                const char* context, 
                                const char* msg); 
    static void _writeToTargets(EmLogLevel level, 
                                const char* context, 
                                const __FlashStringHelper* msg); 

    // Member vars
    const char* m_Context;
    EmLogLevel m_Level;
    // Global vars
    static EmLogLevel g_Level;
    static EmLogTarget* g_Targets;
    static uint8_t g_TargetsCount;
};

template<uint8_t max_len>
inline void EmLog::logError(const char* format, ...) const { 
    if (checkLevel(EmLogLevel::error)) {
        va_list args;
        va_start(args, format);     
        _writeToTargets<max_len>(EmLogLevel::error, m_Context, format, args);
        va_end(args);
    }
}

template<uint8_t max_len>
inline void EmLog::logWarning(const char* format, ...) const { 
    if (checkLevel(EmLogLevel::warning)) {
        va_list args;
        va_start(args, format);     
        _writeToTargets<max_len>(EmLogLevel::warning, m_Context, format, args);
        va_end(args);
    }
}

template<uint8_t max_len>
inline void EmLog::logInfo(const char* format, ...) const { 
    if (checkLevel(EmLogLevel::info)) {
        va_list args;
        va_start(args, format);     
        _writeToTargets<max_len>(EmLogLevel::info, m_Context, format, args);
        va_end(args);
    }
}

template<uint8_t max_len>
inline void EmLog::logDebug(const char* format, ...) const { 
    if (checkLevel(EmLogLevel::debug)) {
        va_list args;
        va_start(args, format);     
        _writeToTargets<max_len>(EmLogLevel::debug, m_Context, format, args);
        va_end(args);
    }
}

template<uint8_t max_len>
inline void EmLog::log(EmLogLevel level, const char* format, ...) const {
    if (checkLevel(level)) {
        va_list args;
        va_start(args, format);     
        _writeToTargets<max_len>(level, m_Context, format, args);
        va_end(args);
    }
}

template<uint8_t max_len>
inline void EmLog::log(EmLogLevel level, 
                       const char* context, 
                       const char* format, ...) {
    if (g_Level >= level) {
        va_list args;
        va_start(args, format);     
        _writeToTargets<max_len>(level, context, format, args);
        va_end(args);
    }
}

template<uint8_t max_len>
void EmLog::_writeToTargets(EmLogLevel level, 
                            const char* context, 
                            const char* format,
                            va_list args) { 
    char msg[max_len+1];
    vsnprintf(msg, max_len+1, format, args);
    _writeToTargets(level, context, msg);
}
#endif
#endif