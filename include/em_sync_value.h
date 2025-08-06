#ifndef __SYNCVALUE__H_
#define __SYNCVALUE__H_

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "em_defs.h"

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
//   If 'GetValue' fails (i.e. returns EmGetValueResult::failed) 
//   it SHOULD NOT change the provided 'value' content
//
template <class T>
class EmValue {
public:
    virtual EmGetValueResult GetValue(T& /*value*/) const = 0;
    virtual bool SetValue(const T /*value*/) = 0;
};

template <class T>
class EmValue<T*> {
public:
    virtual EmGetValueResult GetValue(T* /*value*/) const = 0;
    virtual bool SetValue(const T* /*value*/) = 0;
};

// The flags assigned to each synchronized item
enum class EmSyncFlags: uint8_t {
    canRead  = 0x01, // The item can be read but read can fail and synching moves forward
    mustRead = 0x02, // The item must be read if not synching stops
    write    = 0x04, // Item can be written
    // Combination flags
    canReadAndWrite = 0x05,
    mustReadAndWrite = 0x06,
    // Internal flags
    _firstRead = 0x10,
    _pendingWrite = 0x20,
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

// The item which is used in any synched value class
template <class EmValueOfT, class T>
class EmSyncItem: public EmValueOfT {
public:
    EmSyncItem(EmSyncFlags flags) 
     : m_Flags(flags|EmSyncFlags::_firstRead) {}

    virtual CheckNewValueResult CheckNewValue(T& currentValue) {
        // Item to be read?
        if (WriteOnly()) {
            return CheckNewValueResult::noChange;
        }
        // Pending write? 
        if (IsPendingWrite()) { 
            return CheckNewValueResult::pendingWrite; 
        }
        // Get the value and check the operation result
        EmGetValueResult res = this->GetValue(currentValue);
        if (EmGetValueResult::failed == res) {
            // Get the value failed
            if (MustRead()) {
                // Read cannot fail!
                return CheckNewValueResult::mustReadFailed;
            }
            // Read can fail (e.g. device is turned off or offline)
            return CheckNewValueResult::noChange;
        }
        // Get value succeeded
        if (this->IsFirstRead()) {
            SetFirstRead(false);
            return CheckNewValueResult::valueChanged;
        }
        return EmGetValueResult::succeedNotEqualValue == res ?
               CheckNewValueResult::valueChanged :
               CheckNewValueResult::noChange;
    }

    virtual void DoPendingWrite(T& currentValue) {
        if (this->SetValue(currentValue)) {
            SetFirstRead(false);
            SetPendingWrite(false);
        }
    }

    bool CanRead() const {
        return 0 != static_cast<int>(m_Flags & EmSyncFlags::canRead);
    }

    bool MustRead() const {
        return 0 != static_cast<int>(m_Flags & EmSyncFlags::mustRead);
    }

    bool ReadOnly() const {
        return 0 == static_cast<int>(m_Flags & EmSyncFlags::write);
    }

    bool WriteOnly() const {
        return 0 == static_cast<int>(m_Flags & (EmSyncFlags::canRead | EmSyncFlags::mustRead));
    }

    bool IsPendingWrite() const {
        return 0 != static_cast<int>(m_Flags & EmSyncFlags::_pendingWrite);
    }

    void SetPendingWrite(bool pendingWrite) {
        if (pendingWrite) {
            m_Flags |= EmSyncFlags::_pendingWrite;
        } else {
            m_Flags &= ~EmSyncFlags::_pendingWrite;
        }
    }

    bool IsFirstRead() const {
        return 0 != static_cast<int>(m_Flags & EmSyncFlags::_firstRead);
    }

    void SetFirstRead(bool firstRead) {
        if (firstRead) {
            m_Flags |= EmSyncFlags::_firstRead;
        } else {
            m_Flags &= ~EmSyncFlags::_firstRead;
        }
    }

    virtual bool _setCurrentValue(const T& currentValue) {
        // Read only item?
        if (!ReadOnly()) {
            if (!this->SetValue(currentValue)) {
                SetPendingWrite(true);
                return false;
            }
        }
        return true;
    }

protected:
    EmSyncFlags m_Flags;
};


// This is tha base abstract class that keeps different 
// instances of EmSyncItem synchronized.
template <class T>
class EmSyncValue: public EmUpdatable {
public:
    virtual bool Sync() = 0;

    virtual void Update() override {
        Sync();
    }

    virtual void GetValue(T& value) {
        value = m_CurrentValue;
    }

protected:
    T m_CurrentValue;
};

// The simple priority based values synchronization class.
// The values order is set as the priority, the first that
// changes sets other values.
template <class EmSyncItemOfT, class T, uint8_t size>
class EmSimpleSyncValue: public EmSyncValue<T> {
public:
    EmSimpleSyncValue(EmSyncItemOfT* items[]) {
        for(uint8_t i=0; i < size; i++) {
            m_Items[i] = items[i];
        }
    }

    bool Sync() override {
        for(uint8_t i=0; i < size; i++) {
            switch (m_Items[i]->CheckNewValue(this->m_CurrentValue)) {
                case CheckNewValueResult::valueChanged:
                    // First changed value found: lets write all the others! 
                    return _updateToNewValue(i);
                case CheckNewValueResult::mustReadFailed:
                    // A "must read" value failed to read, cannot proceed with synch!
                    return false;
                case CheckNewValueResult::pendingWrite:
                    // An "old" pending write
                    m_Items[i]->DoPendingWrite(this->m_CurrentValue);
                case CheckNewValueResult::noChange:
                   break;
            }
        }
        return true;
    }

protected:
    bool _updateToNewValue(uint8_t valIndex) {
        bool res = true;
        for(uint8_t i=0; i < size; i++) {
            // Value item that gave this new value?
            if (i != valIndex) {
                if (!m_Items[i]->_setCurrentValue(this->m_CurrentValue)) {
                    res = false;
                }
            }
        }
        return res;
    }

    EmSyncItemOfT* m_Items[size];
};

#endif