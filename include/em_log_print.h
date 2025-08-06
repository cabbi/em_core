#ifndef __LOG_SERIAL__H_
#define __LOG_SERIAL__H_

#include "em_log.h"

// The basic print log target
template <class T>
class EmLogPrintTarget: public EmLogTarget {
public:   
    EmLogPrintTarget(T& printer) : m_Printer(printer) {}

    virtual void write(EmLogLevel level, 
                       const char* context, 
                       const char* msg){
        _printLevel(level);
        if (context != NULL) {
            m_Printer.print(context);m_Printer.print(" - ");
        }
        m_Printer.println(msg);
    }

    virtual void write(EmLogLevel level, 
                       const char* context, 
                       const __FlashStringHelper* msg){
        _printLevel(level);
        if (context != NULL) {
            m_Printer.print(context);m_Printer.print(" - ");
        }
        m_Printer.println(msg);
    }                       

protected:
    virtual void _printLevel(EmLogLevel level){
        m_Printer.print("[");m_Printer.print(LevelToStr(level));m_Printer.print("] ");
    }

    T& m_Printer;
};

#endif