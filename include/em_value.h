#ifndef __EM_VALUE__H_
#define __EM_VALUE__H_

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>

#include "em_defs.h"
#include "em_iterator.h"
#include "em_auto_ptr.h"


// The get methods result.
enum class EmGetValueResult: uint8_t {    
    // Operation failed
    failed = 0, 
    // Operation succeeded, the provided value equals the object value
    succeedEqualValue = 1,
    // Operation succeeded, the provided value is not equal as the object value
    succeedNotEqualValue = 2
};


// The basic value interface to get and set a value.
// Each class that can be synched should implement this class
//
// IMPLEMENTATION NOTES:
// ---------------------
//   If 'getValue' fails (i.e. returns EmGetValueResult::failed) 
//   it SHOULD NOT change the provided 'value' content!
template <class T>
class EmValue {
public:
    EmValue() = default;
    virtual ~EmValue() = default;

    virtual EmGetValueResult getValue(T& /*value*/) const = 0;
    virtual bool setValue(const T& /*value*/) = 0;
};

template <class T>
class EmValue<T*> {
public:
    virtual ~EmValue() = default;

    virtual EmGetValueResult getValue(T* /*value*/) const = 0;
    virtual bool setValue(const T* /*value*/) = 0;
};


// The 'onSetValue' callback prototype
template <class T>
using EmOnSetValueCallbackType = bool (*)(void* self, const T& value);


// This class provides an 'onSetValue' callback.
// 
// The aim is to define this class with any implementation of an 'EmValue' subclass.
template <class EmValueOfT,
          class T,
          EmOnSetValueCallbackType<T> OnSetValue>
class EmValueEx: public EmValueOfT {
//static_assert(std::is_base_of<EmValue<T>, EmValueOfT>::value, "EmValueOfT must derive from 'EmValue'");
public:
    using EmValueOfT::EmValueOfT;

    virtual bool setValue(const T& value) override {
        if (EmValueOfT::setValue(value)) {
            return OnSetValue(this, value);
        }
        return false;
    }
};

#endif