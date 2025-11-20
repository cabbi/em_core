#ifndef __EM_LOG__H_
#define __EM_LOG__H_

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
const char* levelToStr(EmLogLevel level);

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

// Forward declarations
template<uint8_t max_len>
void writeToTargets_(EmLogLevel level, 
                     const char* context, 
                     const char* format,
                     va_list args);
void writeToTargets_(EmLogLevel level, 
                     const char* context, 
                     const char* msg); 
void writeToTargets_(EmLogLevel level, 
                     const char* context, 
                     const __FlashStringHelper* msg); 

// NOTE:
//  Define 'EM_NO_LOG' to avoid extra Flash and RAM memory consumption.  
#ifdef EM_NO_LOG

inline const char* levelToStr(EmLogLevel level) { return ""; }

class EmLog {
public:    
    static void init(EmLogTarget& target, EmLogLevel level) {}
    static void init(EmLogTarget targets[], uint8_t targetsCount, EmLogLevel level) {}

    EmLog(const char* context = NULL, 
          EmLogLevel level = EmLogLevel::global) {}

    template<uint8_t max_len>
    void logError(const char* format, ...) const {}
    void logError(const char* msg) const {}
    void logError(const __FlashStringHelper* msg) const {}

    template<uint8_t max_len>
    void logWarning(const char* format, ...) const {}
    void logWarning(const char* msg) const {}
    void logWarning(const __FlashStringHelper* msg) const {}

    template<uint8_t max_len>
    void logInfo(const char* format, ...) const {}
    void logInfo(const char* msg) const {}
    void logInfo(const __FlashStringHelper* msg) const {}

    template<uint8_t max_len>
    void logDebug(const char* format, ...) const {}
    void logDebug(const char* msg) const {}
    void logDebug(const __FlashStringHelper* msg) const {}

    template<uint8_t max_len>
    void log(EmLogLevel level, const char* format, ...) const {}
    void log(EmLogLevel level, const char* msg) const {}
    void log(EmLogLevel level, const __FlashStringHelper* msg) const {}
    
    bool checkLevel(EmLogLevel level) const { return false; }

    void setLevel(EmLogLevel level) {}
    EmLogLevel getLevel() const { return EmLogLevel::none; }

    static void setGlobalLevel(EmLogLevel level) {}
};

template<uint8_t max_len>
inline void log(EmLogLevel level, const char* context, const char* format, ...) {}
template<uint8_t max_len>
inline void log(EmLogLevel level, const char* context, const char* format, va_list args) {}
inline void log(EmLogLevel level, const char* context, const char* msg) {}
inline void log(EmLogLevel level, const char* context, const __FlashStringHelper* msg) {}

template<uint8_t max_len>
inline void logInfo(const char* context, const char* format, ...) {}
inline void logInfo(const char* context, const char* msg) {}
inline void logInfo(const char* context, const __FlashStringHelper* msg) {}

template<uint8_t max_len>
inline void logWarning(const char* context, const char* format, ...) {}
inline void logWarning(const char* context, const char* msg) {}
inline void logWarning(const char* context, const __FlashStringHelper* msg) {}

template<uint8_t max_len>
inline void logError(const char* context, const char* format, ...) {}
inline void logError(const char* context, const char* msg) {}
inline void logError(const char* context, const __FlashStringHelper* msg) {}

template<uint8_t max_len>
inline void logDebug(const char* context, const char* format, ...) {}
inline void logDebug(const char* context, const char* msg) {}
inline void logDebug(const char* context, const __FlashStringHelper* msg) {}

#else

// The log class can be inherited to allow easy logging
class EmLog {
    friend const char* levelToStr(EmLogLevel level);
public:    
    static void init(EmLogTarget& target, EmLogLevel level) {
        EmLog::g_Targets = &target;
        EmLog::g_TargetsCount = 1;
        EmLog::g_Level = level;
    }

    static void init(EmLogTarget targets[], uint8_t targetsCount, EmLogLevel level) {
        EmLog::g_Targets = targets;
        EmLog::g_TargetsCount = targetsCount;
        EmLog::g_Level = level;
    }

    EmLog(const char* context = NULL, 
          EmLogLevel level = EmLogLevel::global)
     : m_Context(context),
       m_Level(level) { }


    template<uint8_t max_len>
    void logInfo(const char* format, ...) const { 
        if (checkLevel(EmLogLevel::info)) {
            va_list args;
            va_start(args, format);     
            writeToTargets_<max_len>(EmLogLevel::info, m_Context, format, args);
            va_end(args);
        }
    }
    void logInfo(const char* msg) const { 
        log(EmLogLevel::info, msg);
    }
    void logInfo(const __FlashStringHelper* msg) const { 
        log(EmLogLevel::info, msg);
    }
    
    template<uint8_t max_len>
    void logError(const char* format, ...) const  { 
        if (checkLevel(EmLogLevel::error)) {
            va_list args;
            va_start(args, format);     
            writeToTargets_<max_len>(EmLogLevel::error, m_Context, format, args);
            va_end(args);
        }
    }

    void logError(const char* msg) const { 
        log(EmLogLevel::error, msg);
    }
    void logError(const __FlashStringHelper* msg) const { 
        log(EmLogLevel::error, msg);
    }

    template<uint8_t max_len>
    void logWarning(const char* format, ...) const { 
        if (checkLevel(EmLogLevel::warning)) {
            va_list args;
            va_start(args, format);     
            writeToTargets_<max_len>(EmLogLevel::warning, m_Context, format, args);
            va_end(args);
        }
    }
    void logWarning(const char* msg) const { 
        log(EmLogLevel::warning, msg);
    }
    void logWarning(const __FlashStringHelper* msg) const { 
        log(EmLogLevel::warning, msg);
    }

    template<uint8_t max_len>
    void logDebug(const char* format, ...) const{ 
    #ifndef EM_LOG_NO_DEBUG
        if (checkLevel(EmLogLevel::debug)) {
            va_list args;
            va_start(args, format);     
            writeToTargets_<max_len>(EmLogLevel::debug, m_Context, format, args);
            va_end(args);
        }
    #endif
    }
    void logDebug(const char* msg) const { 
    #ifndef EM_LOG_NO_DEBUG
        log(EmLogLevel::debug, msg);
    #endif
    }
    void logDebug(const __FlashStringHelper* msg) const { 
    #ifndef EM_LOG_NO_DEBUG
        log(EmLogLevel::debug, msg);
    #endif
    }

    template<uint8_t max_len>
    inline void log(EmLogLevel level, const char* format, ...) const {
        if (checkLevel(level)) {
            va_list args;
            va_start(args, format);     
            writeToTargets_<max_len>(level, m_Context, format, args);
            va_end(args);
        }
    }

    template<uint8_t max_len>
    inline void log(EmLogLevel level, const char* format, va_list args) const {
        if (checkLevel(level)) {
            writeToTargets_<max_len>(level, m_Context, format, args);
        }
    }
    void log(EmLogLevel level, const char* msg) const;
    void log(EmLogLevel level, const __FlashStringHelper* msg) const;
    
    bool checkLevel(EmLogLevel level) const { 
        return (m_Level == EmLogLevel::global ? g_Level : m_Level) >= level; 
    }

    const char* getContext() const {
        return m_Context;
    }
    
    void setLevel(EmLogLevel level) { 
        m_Level = level; 
    }

    EmLogLevel getLevel() const { 
        return m_Level; 
    }

    static void setGlobalLevel(EmLogLevel level) { 
        g_Level = level; 
    }

    static EmLogLevel getGlobalLevel() { 
        return g_Level; 
    }

    static EmLogTarget* getTargets() {
        return g_Targets;
    }

    static EmLogTarget& getTarget(uint8_t i) {
        return g_Targets[i];
    }

    static uint8_t getTargetsCount() {
        return g_TargetsCount;
    }

protected:
    // Member vars
    const char* m_Context;
    EmLogLevel m_Level;
    // Global vars
    static EmLogLevel g_Level;
    static EmLogTarget* g_Targets;
    static uint8_t g_TargetsCount;
}; // EmLog class

// Global methods
template<uint8_t max_len>
inline void writeToTargets_(EmLogLevel level, 
                            const char* context, 
                            const char* format,
                            va_list args) { 
    char msg[max_len+1];
    vsnprintf(msg, max_len+1, format, args);
    writeToTargets_(level, context, msg);
}

template<uint8_t max_len>
inline void log(EmLogLevel level, 
                const char* context, 
                const char* format, ...) {
    if (EmLog::getGlobalLevel() >= level) {
        va_list args;
        va_start(args, format);     
        writeToTargets_<max_len>(level, context, format, args);
        va_end(args);
    }
}

template<uint8_t max_len>
inline void log(EmLogLevel level, 
                const char* context, 
                const char* format,
                va_list args) {
    if (EmLog::getGlobalLevel() >= level) {
        writeToTargets_<max_len>(level, context, format, args);
    }
}

inline void log(EmLogLevel level, const char* context, const char* msg) { 
    if (EmLog::getGlobalLevel() >= level) {
        writeToTargets_(level, context, msg);
    }
}

inline void log(EmLogLevel level, const char* context, const __FlashStringHelper* msg) { 
    if (EmLog::getGlobalLevel() >= level) {
        writeToTargets_(level, context, msg);
    }
}

template<uint8_t max_len>
inline void logInfo(const char* context, const char* format, ...) {
    va_list args;
    va_start(args, format);     
    log<max_len>(EmLogLevel::info, context, format, args);
    va_end(args);
}
inline void logInfo(const char* context, const char* msg) {
    log(EmLogLevel::info, context, msg);
}
inline void logInfo(const char* context, const __FlashStringHelper* msg) {
    log(EmLogLevel::info, context, msg);
}

template<uint8_t max_len>
inline void logWarning(const char* context, const char* format, ...) {
    va_list args;
    va_start(args, format);     
    log<max_len>(EmLogLevel::warning, context, format, args);
    va_end(args);
}
inline void logWarning(const char* context, const char* msg) {
    log(EmLogLevel::warning, context, msg);
}
inline void logWarning(const char* context, const __FlashStringHelper* msg) {
    log(EmLogLevel::warning, context, msg);
}

template<uint8_t max_len>
inline void logError(const char* context, const char* format, ...) {
    va_list args;
    va_start(args, format);     
    log<max_len>(EmLogLevel::error, context, format, args);
    va_end(args);
}
inline void logError(const char* context, const char* msg) {
    log(EmLogLevel::error, context, msg);
}
inline void logError(const char* context, const __FlashStringHelper* msg) {
    log(EmLogLevel::error, context, msg);
}

template<uint8_t max_len>
inline void logDebug(const char* context, const char* format, ...) {
    #ifndef EM_LOG_NO_DEBUG
    va_list args;
    va_start(args, format);     
    log<max_len>(EmLogLevel::debug, context, format, args);
    va_end(args);
    #endif
}
inline void logDebug(const char* context, const char* msg) {
    #ifndef EM_LOG_NO_DEBUG
    log(EmLogLevel::debug, context, msg);
    #endif
}
inline void logDebug(const char* context, const __FlashStringHelper* msg) {
    #ifndef EM_LOG_NO_DEBUG
    log(EmLogLevel::debug, context, msg);
    #endif
}

#endif 
#endif // __EM_LOG__H__