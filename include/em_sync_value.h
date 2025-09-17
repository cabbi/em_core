#ifndef __EM_SYNCVALUE__H_
#define __EM_SYNCVALUE__H_

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
//   it SHOULD NOT change the provided 'value' content
//
template <class T>
class EmValue {
public:
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

// The flags assigned to each synchronized item
enum class EmSyncFlags: uint8_t {
    canRead  = 0x01, // The item can be read but read can fail and synching moves forward
    mustRead = 0x02, // The item must be read if not, then item synching stops
    canWrite = 0x04, // Item can be written
    // Combination flags
    canReadCanWrite = 0x05,
    mustReadCanWrite = 0x06,
    // Internal flags
    _firstRead = 0x10,    // The item has never been read
    _pendingWrite = 0x20,  // The item has a pending write (i.e. setValue failed)
    _valueChanged = 0x40   // The item value has changed
};

inline EmSyncFlags operator~ (EmSyncFlags a) { return static_cast<EmSyncFlags>(~static_cast<int>(a)); }
inline EmSyncFlags operator|(EmSyncFlags a, EmSyncFlags b) { return static_cast<EmSyncFlags>(static_cast<int>(a) | static_cast<int>(b)); }
inline EmSyncFlags operator&(EmSyncFlags a, EmSyncFlags b) { return static_cast<EmSyncFlags>(static_cast<int>(a) & static_cast<int>(b)); }
inline EmSyncFlags& operator|=(EmSyncFlags& a, EmSyncFlags b) { return (EmSyncFlags&)((int&)(a) |= static_cast<int>(b)); }
inline EmSyncFlags& operator&=(EmSyncFlags& a, EmSyncFlags b) { return (EmSyncFlags&)((int&)(a) &= static_cast<int>(b)); }

enum class CheckNewValueResult: int8_t {
    noChange = 0,
    valueChanged,
    mustReadFailed,
    pendingWrite
};

// The synchronized value class.
template <class EmValueOfT, class T>
class EmSyncValue: public EmValueOfT {
public:
    EmSyncValue(EmSyncFlags flags) 
     : m_flags(flags|EmSyncFlags::_firstRead) {}

    virtual ~EmSyncValue() = default;

    /*  TODO: check if we can avoid the "m_currentValue" storage in 'EmSyncValues'
    virtual bool setValue(const T& value) override {
        bool res = EmValueOfT::setValue(value);
        if (res) {
            setPendingWrite(false);
            setValueChanged(true);
        } else {
            setPendingWrite(true);
        }
        return res;
    }
    */        

    virtual CheckNewValueResult checkNewValue(T& currentValue) {
        // Item to be read?
        if (writeOnly()) {
            return CheckNewValueResult::noChange;
        }
        // Pending write? 
        if (isPendingWrite()) { 
            return CheckNewValueResult::pendingWrite; 
        }
        // Get the value and check the operation result
        EmGetValueResult res = this->getValue(currentValue);
        if (EmGetValueResult::failed == res) {
            // Get the value failed
            if (mustRead()) {
                // Read cannot fail!
                return CheckNewValueResult::mustReadFailed;
            }
            // Read can fail (e.g. device is turned off or offline)
            return CheckNewValueResult::noChange;
        }
        // Get value succeeded
        if (this->isFirstRead()) {
            setFirstRead(false);
            return CheckNewValueResult::valueChanged;
        }
        return EmGetValueResult::succeedNotEqualValue == res ?
               CheckNewValueResult::valueChanged :
               CheckNewValueResult::noChange;
    }

    virtual void doPendingWrite(T& currentValue) {
        if (this->setValue(currentValue)) {
            setFirstRead(false);
            setPendingWrite(false);
        }
    }

    virtual bool canRead() const {
        return 0 != static_cast<int>(m_flags & EmSyncFlags::canRead);
    }

    virtual bool mustRead() const {
        return 0 != static_cast<int>(m_flags & EmSyncFlags::mustRead);
    }

    virtual bool readOnly() const {
        return 0 == static_cast<int>(m_flags & EmSyncFlags::canWrite);
    }

    virtual bool writeOnly() const {
        return 0 == static_cast<int>(m_flags & (EmSyncFlags::canRead | EmSyncFlags::mustRead));
    }

    virtual bool isPendingWrite() const {
        return 0 != static_cast<int>(m_flags & EmSyncFlags::_pendingWrite);
    }

    virtual void setPendingWrite(bool newStatus) {
        if (newStatus) {
            m_flags |= EmSyncFlags::_pendingWrite;
        } else {
            m_flags &= ~EmSyncFlags::_pendingWrite;
        }
    }

    virtual void setValueChanged(bool newStatus) {
        if (newStatus) {
            m_flags |= EmSyncFlags::_valueChanged;
        } else {
            m_flags &= ~EmSyncFlags::_valueChanged;
        }
    }

    virtual bool isFirstRead() const {
        return 0 != static_cast<int>(m_flags & EmSyncFlags::_firstRead);
    }

    virtual void setFirstRead(bool firstRead) {
        if (firstRead) {
            m_flags |= EmSyncFlags::_firstRead;
        } else {
            m_flags &= ~EmSyncFlags::_firstRead;
        }
    }

    virtual bool setCurrentValue_(const T& currentValue) {
        // Read only item?
        if (!readOnly()) {
            if (!this->setValue(currentValue)) {
                setPendingWrite(true);
                return false;
            }
        }
        return true;
    }

protected:
    EmSyncFlags m_flags;
};


// This is tha base abstract class that keeps multiple instances of EmSyncValue synchronized.
template <class EmSyncItemOfT, class T>
class EmSyncValues: public EmUpdatable {
public:
    EmSyncValues() = default;
    virtual ~EmSyncValues() = default;

    virtual EmIterator<EmSyncItemOfT>* iterator() = 0;

    virtual void update() override {
        doSync();
    }

    virtual EmGetValueResult getValue(T& value) const {
        if (value == m_currentValue) {
            return EmGetValueResult::succeedEqualValue;
        }
        value = m_currentValue;
        return EmGetValueResult::succeedNotEqualValue;
    }

    virtual bool setValue(const T& value, bool doSyncNow) {
        m_currentValue = value;
        if (doSyncNow) {
            return doSync();
        }
        return true;
    }

    virtual bool doSync() {
        EmAutoPtr<EmIterator<EmSyncItemOfT>> it(iterator());
        EmSyncItemOfT* pItem = nullptr;
        while (it->next(pItem)) {
            switch (pItem->checkNewValue(this->m_currentValue)) {
                case CheckNewValueResult::valueChanged:
                    // First changed value found: lets write all the others! 
                    return updateToNewValue_(it.get(), pItem);
                case CheckNewValueResult::mustReadFailed:
                    // A "must read" value failed to read, cannot proceed with synch!
                    return false;
                case CheckNewValueResult::pendingWrite:
                    // An "old" pending write
                    pItem->doPendingWrite(this->m_currentValue);
                    break;
                case CheckNewValueResult::noChange:
                    break;
            }
        }
        return true;
    }

protected:
    virtual bool updateToNewValue_(EmIterator<EmSyncItemOfT>* it, 
                                   EmSyncItemOfT* pUpdatedItem) {
        bool res = true;
        EmSyncItemOfT* pItem = nullptr;
        it->reset();
        while (it->next(pItem)) {
            // Value item that gave this new value?
            if (pUpdatedItem != pItem) {
                if (!pItem->setCurrentValue_(this->m_currentValue)) {
                    res = false;
                }
            }
        }
        return res;
    }

    T m_currentValue;
};

// The simple array priority based values synchronization class.
// The values order is set as the priority, the first that
// changes sets other values.
template <class EmSyncItemOfT, class T, uint8_t size>
class EmSimpleSyncValue: public EmSyncValues<EmSyncItemOfT, T> {
public:
    EmSimpleSyncValue(EmSyncItemOfT* firstItem, ...) {
        m_items[0] = firstItem;
        va_list args;
        va_start(args, firstItem);
        for(uint8_t i=1; i < size; i++) {
            m_items[i] = va_arg(args, EmSyncItemOfT*);
        }
        va_end(args);
    }

    virtual ~EmSimpleSyncValue() = default;

    virtual EmIterator<EmSyncItemOfT>* iterator() {
        return new EmArrayIterator<EmSyncItemOfT>(m_items, size);
    }

protected:
    EmSyncItemOfT* m_items[size];
};

#endif