#ifndef __EM_STORAGE__H_
#define __EM_STORAGE__H_

#include "em_defs.h"

#ifdef EM_NVS

#include <nvs.h>
#include <cstring>
#include <WString.h>
#include "em_log.h"
#include "em_sync_value.h"
#include "em_tag.h"


// NVS persistent storage class
class EmStorage: public EmLog {
private:
    nvs_handle_t m_handle;

public:
    EmStorage(EmLogLevel logLevel = EmLogLevel::global)
     : EmLog("EmStorage", logLevel),
       m_handle(-1) {}

    ~EmStorage() {
        end();
    }

    bool isInitialized() const {
        return m_handle != -1;
    }
    bool isNotInitialized() const {
        return !isInitialized();
    }

    bool begin(const char* name);
    void end();
    bool clear() const;
    bool commit() const;

    template<typename T>
    size_t putValue(const char* key, const T& value, bool commit=true) const {
        return putBytes(key, &value, sizeof(value), commit);
    }   
    size_t putValue(const char* key, const EmTagValue& value, bool commit=true) const {
        // We do not store undefined type!
        if (value.getType() == EmTagValueType::vt_undefined) {
            return 0;
        }
        // String special handling!?
        if (value.getType() == EmTagValueType::vt_string) {
            String str;
            EmGetValueResult res = value.getValue(str);
            if (res == EmGetValueResult::succeedNotEqualValue) {
                return putString(key, str, commit);
            } else 
            if (res == EmGetValueResult::succeedEqualValue) {
                // Avoid writing the same value again
                return str.length();
            }
            return 0;
        }
        // Not a string, lets write the value bytes
        EmTagValueStruct valueBytes;
        value.toStruct(valueBytes);
        return putBytes(key, &valueBytes, sizeof(valueBytes), commit);
    }
    size_t putString(const char* key, const char* value, bool commit=true) const;
    size_t putString(const char* key, const String& value, bool commit=true) const;
    size_t putBytes(const char* key, const void* value, size_t len, bool commit=true) const;

    template<typename T>
    size_t getValue(const char* key, T& value) const {
        return getBytes(key, &value, sizeof(value));
    }
    size_t getValue(const char* key, EmTagValue& value) const {
        // String special handling!?
        if (value.getType() == EmTagValueType::vt_string) {
            String str;
            EmGetValueResult res = value.getValue(str);
            if (res == EmGetValueResult::succeedNotEqualValue) {
                return value.setValue(str, false);
            }
            return str.length();
        }
        // Not a string, lets read the value bytes
        EmTagValueStruct valueBytes;
        size_t size = getBytes(key, &valueBytes, sizeof(valueBytes));
        if (size > 0) {
            value.fromStruct(valueBytes);
        }
        return size;
    }
    size_t getString(const char* key, char* value, const size_t maxLen) const;
    String getString(const char* key, const char* defaultValue="") const;
    size_t getBytes(const char* key, void * buf, size_t maxLen) const;

    size_t getBytesLength(const char* key) const;
    size_t getStringLength(const char* key) const;

    bool hasValue(const char* key) const { return hasBytes(key); }
    bool hasBytes(const char* key) const { return getBytesLength(key) > 0;}
    bool hasString(const char* key) const { return getStringLength(key) > 0; }

    size_t freeEntries() const;
};

template<typename T, EmStorage& tStorage>
class EmStorageValue: public EmValue<T> {
protected:
    const char* m_key;

public:
    EmStorageValue(const char* key)
     : EmValue<T>(), 
       m_key(key) {}

    virtual ~EmStorageValue() = default;

    virtual const char* getKey() const { return m_key; }

    virtual EmGetValueResult getValue(T& value) const override {
        T curVal;
        if (tStorage.getValue(m_key, curVal) != sizeof(value)) {
            return EmGetValueResult::failed;
        }
        if (value == curVal) {
            return EmGetValueResult::succeedEqualValue;        
        }
        value = curVal;
        return EmGetValueResult::succeedNotEqualValue;
    }

    virtual bool setValue(const T& value) override {
        return tStorage.putValue(m_key, value) == sizeof(value);
    }
};

template<typename T, EmStorage& tStorage>
class EmStorageValueEx: public EmStorageValue<T, tStorage> {
protected:
    void (*m_onSetValue)(const T&);

public:
    EmStorageValueEx(const char* key, 
                     void (*onSetValue)(const T&))
     : EmStorageValue<T, tStorage>(key), 
       m_onSetValue(onSetValue) {}

    virtual ~EmStorageValueEx() = default;

    virtual bool setValue(const T& value) override {
        bool res = EmStorageValue<T, tStorage>::setValue(value);
        if (res && m_onSetValue != nullptr) {
            m_onSetValue(value);
        }
        return res;
    }
};

template<EmStorage& tStorage>
class EmStorageTag: public EmStorageValue<EmTagValue, tStorage>, 
                    public EmTagBase {
public:
    EmStorageTag(const char* key, 
                 EmSyncFlags flags)
     : EmStorageValue<EmTagValue, tStorage>(key),
       EmTagBase(flags) {}

    virtual const char* getId() const override { return this->getKey(); }     
    virtual EmTagValue getValue() const {
        EmTagValue val;
        tStorage.getValue(this->getKey(), val);
        return val;
    }

    virtual EmGetValueResult getValue(EmTagValue& value) const override {
        return EmStorageValue<EmTagValue, tStorage>::getValue(value);
    }

    virtual bool setValue(const EmTagValue& value) override {
        return EmStorageValue<EmTagValue, tStorage>::setValue(value);
    }
};

template<EmStorage& tStorage>
class EmStorageTagEx: public EmStorageValueEx<EmTagValue, tStorage>, 
                      public EmTagBase {
public:
    EmStorageTagEx(const char* key, 
                   EmSyncFlags flags,
                   void (*onSetValue)(const EmTagValue&) = nullptr)
     : EmStorageValueEx<EmTagValue, tStorage>(key, onSetValue),
       EmTagBase(flags) {}

    virtual const char* getId() const override { return this->getKey(); }     
    virtual EmTagValue getValue() const {
        EmTagValue val;
        tStorage.getValue(this->getKey(), val);
        return val;
    }

    virtual EmGetValueResult getValue(EmTagValue& value) const override {
        return EmStorageValueEx<EmTagValue, tStorage>::getValue(value);
    }

    virtual bool setValue(const EmTagValue& value) override {
        return EmStorageValueEx<EmTagValue, tStorage>::setValue(value);
    }
};

#endif
#endif // __EM_STORAGE__H_