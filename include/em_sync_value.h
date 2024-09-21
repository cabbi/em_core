#ifndef __SYNCVALUE__H_
#define __SYNCVALUE__H_

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>

#include "em_thread_lock.h"


// The basic value interface to get and set a value.
template <class T>
class EmValue {
public:
    EmValue() {};

    virtual bool GetValue(T& value) const =0;
    virtual bool SetValue(T const value)=0;
};

template <class T>
class EmValue<T*> {
public:
    EmValue(const uint8_t maxLen)
     : m_MaxLen(maxLen) {}

    virtual bool GetValue(T* value) const =0;
    virtual bool SetValue(T* const value)=0;

    uint8_t GetMaxLen() const { 
        return m_MaxLen; 
    }

protected:
    const uint8_t m_MaxLen;
};



#endif