#ifndef __SYNCVALUE__H_
#define __SYNCVALUE__H_

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>

#include "em_thread_lock.h"

// The basic value interface to get and set a value.
// Each class tha can be synched should implement this class
template <class T>
class EmValue {
public:
    virtual bool GetValue(T& /*value*/) const = 0;
    virtual bool SetValue(const T /*value*/) = 0;
    virtual bool Equals(const T /*value*/) = 0;
};

// The flags assigned to each synchronized item
enum class EmSyncFlags: uint8_t {
    canRead  = 0x01, // The item can be read but read can fail and synching moves forward
    mustRead = 0x02, // The item must be read if not synching stops
    write    = 0x04, // Item can be written
    // Internal flags
    _pendingWrite = 0x10,
    // Flag masks
    _userFlagsMask = 0x0F,
    _internalFlagsMask = 0xF0,
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
template <class T, class EmValueType> // Where EmValueType derives from EmValue<T>
class EmSyncItem: public EmValueType {
public:
    EmSyncItem(EmSyncFlags flags) 
     : m_Flags(flags),
       // TODO: handle pointer value type
       m_CurrentValue(T()) {}

    virtual CheckNewValueResult CheckNewValue(T& newVal) const {
        // Item can be read?
        if (WriteOnly()) {
            return CheckNewValueResult::noChange;
        }
        // Get item value
        if (GetValue(newVal)) {
            if (!Equals(newVal) {
                // TODO: handle pointer value type
                m_CurrentValue = newVal;
                return CheckNewValueResult::valueChanged;
            }
        } else {
            // Failed getting value
            if (MustRead()) {
                return CheckNewValueResult::mustReadFailed;
            }
        }
        // Device might be off or offline, lets see if a pending write 
        return (m_Flags & EmSyncFlags::_pendingWrite) ? 
               CheckNewValueResult::pendingWrite : 
               CheckNewValueResult::noChange;
    }

    virtual void DoPendingWrite() {
        // TODO
    }

    virtual bool GetValue(T& value) const {
        if (!EmValueType::GetValue(value)) {
            return false;
        }
        m_CurrentValue = value;
        return true;
    }

    bool ReadOnly() {
        // TODO: is this correct???
        return EmSyncFlags::_userFlagsMask & (~m_Flags & (EmSyncFlags::canRead | EmSyncFlags::mustRead));
    }

    bool CanRead() {
        return m_Flags & EmSyncFlags::canRead;
    }

    bool MustRead() {
        return m_Flags & EmSyncFlags::mustRead;
    }

    bool WriteOnly() {
        // TODO: is this correct???
        return EmSyncFlags::_userFlagsMask & (~m_Flags & EmSyncFlags::write);
    }

protected:
    EmSyncFlags m_Flags;
    T m_CurrentValue;
};

// This is tha base abstract class that keeps different 
// instances of EmSyncItem synchronized.
template <class T, class EmValueType>
class EmSyncValue {
    bool Sync() = 0;
};

// The simple priority based values synchronization class.
// The values order is set as the priority, the first that
// changes sets other values.
template <class T, class EmValueType>
class EmSimpleSyncValue: public EmSyncValue<T, EmValueType> {
    EmSimpleSyncValue(EmSyncItem<T, 
                      EmValueType> items[], 
                      uint8_t count)
     : m_Items(items),
       m_Count(count) {
    }

    bool Sync() {
        T newVal;
        for(uint8_t i=0; i < m_Count; i++) {
            switch (m_Items[i].CheckNewValue(newValue)) {
                case CheckNewValueResult::valueChanged:
                    return _updateToNewValue(newVal, i);
                case CheckNewValueResult::mustReadFailed:
                    return false;
                case CheckNewValueResult::pendingWrite:
                    m_Items[i].doPendingWrite();
            }
        }
        return true;
    }

protected:
    bool _updateToNewValue(T newVal, uint8_t valIndex) {
        bool res = true;
        for(uint8_t i=0; i < m_Count; i++) {
            // Value item that gave this new value?
            if (i == valIndex) {
                continue;
            }
            // Writable item?
            if (!m_Items[i].ReadOnly()) {
                if (!m_Items[i].SetValue(newVal)) {
                    // One failed but we move forward writing othe items
                    m_Items[i].SetPendingWrite(true);
                    res = false;
                }
            }
        }
        return res;
    }

    EmSyncItem<T, EmValueType>* m_Items;
    uint8_t m_Count;
};

#endif