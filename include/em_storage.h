#ifndef __EM_STORAGE__H_
#define __EM_STORAGE__H_

#include "em_defs.h"

#ifdef EM_NVS

#include <nvs.h>
#include <cstring>
#include <WString.h>
#include "em_log.h"
#include "em_sync_value.h"


// NVS persistent storage class
class EmStorage: public EmLog {
private:
    nvs_handle_t m_handle;

public:
    EmStorage()
     : EmLog("EmStorage"),
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
    size_t putString(const char* key, const char* value, bool commit=true) const;
    size_t putString(const char* key, const String& value, bool commit=true) const;
    size_t putBytes(const char* key, const void* value, size_t len, bool commit=true) const;

    template<typename T>
    size_t getValue(const char* key, T& value) const {
        return getBytes(key, &value, sizeof(value));
    }
    size_t getString(const char* key, char* value, const size_t maxLen) const;
    String getString(const char* key, const String defaultValue) const;
    size_t getBytesLength(const char* key) const;
    size_t getBytes(const char* key, void * buf, size_t maxLen) const;

    size_t freeEntries() const;
};

template<typename T, EmStorage& storage>
class EmStorageValue: public EmValue<T> {
protected:
    const char* m_key;

public:
    EmStorageValue(const char* key)
     : EmValue<T>(), m_key(key) {}

    virtual ~EmStorageValue() = default;

    virtual EmGetValueResult getValue(T& value) const override {
        T curVal;
        if (storage.getValue(m_key, curVal) != sizeof(value)) {
            return EmGetValueResult::failed;
        }
        if (value == curVal) {
            return EmGetValueResult::succeedEqualValue;        
        }
        value = curVal;
        return EmGetValueResult::succeedNotEqualValue;
    }

    virtual bool setValue(const T& value) override {
        return storage.putValue(m_key, value) == sizeof(value);
    }
};

#endif
#endif // __EM_STORAGE__H_