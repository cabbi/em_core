#ifndef __EM_STORAGE__H_
#define __EM_STORAGE__H_

#include "em_defs.h"

#ifdef EM_NVS

#include <cstring>
#include <WString.h>
#include <nvs.h>

#include "em_log.h"
#include "em_value.h"
#include "em_value_sync.h"
#include "em_tag.h"

#define EM_STORAGE_NULL_HANDLE 0

// NVS persistent storage class
class EmStorage: public EmLog {
private:
    nvs_handle_t m_handle;

public:
    EmStorage(const char* logContext="EmStorage", 
              EmLogLevel logLevel = EmLogLevel::global)
     : EmLog(logContext, logLevel),
       m_handle(EM_STORAGE_NULL_HANDLE) {}

    ~EmStorage() {
        end();
    }

    bool isInitialized() const {
        return m_handle != EM_STORAGE_NULL_HANDLE;
    }
    bool isNotInitialized() const {
        return !isInitialized();
    }

    bool begin(const char* name);
    void end();
    bool clear() const;
    bool commit() const;

    // Initialization methods (i.e. value is set only if key does not exist)
    template<typename T>
    size_t initValue(const char* key, const T& value, bool commit=true) const {
        if (!hasKey(key)) {
            return putValue(key, value, commit);
        }
        return 0;
    }   
    size_t initValue(const char* key, const EmTagValue& value, bool commit=true) const {
        if (!hasKey(key)) {
            return putValue(key, value, commit);
        }
        return 0;
    }   
    size_t initString(const char* key, const char* value, bool commit=true) const {
        if (!hasKey(key)) {
            return putString(key, value, commit);
        }
        return 0;
    }   
    size_t initString(const char* key, const String& value, bool commit=true) const {
        if (!hasKey(key)) {
            return putString(key, value, commit);
        }
        return 0;
    }   
    size_t initBytes(const char* key, const void* value, size_t len, bool commit=true) const {
        if (!hasKey(key)) {
            return putBytes(key, value, len, commit);
        }
        return 0;
    }   

    template<typename T>
    size_t putValue(const char* key, const T& value, bool commit=true) const {
        return putBytes(key, &value, sizeof(value), commit);
    }   
    size_t putValue(const char* key, const EmTagValue& value, bool commit=true) const;
    size_t putString(const char* key, const char* value, bool commit=true) const;
    size_t putString(const char* key, const String& value, bool commit=true) const {
        return putString(key, value.c_str(), commit);
    }
    size_t putBytes(const char* key, const void* value, size_t len, bool commit=true) const;

    template<typename T>
    size_t getValue(const char* key, T& value) const {
        return getBytes(key, &value, sizeof(value));
    }
    template<typename T>
    T getValue(const char* key) const {
        T value;
        if (getBytes(key, &value, sizeof(value)) == sizeof(value)) {
            return value;
        }
        return T();
    }
    size_t getValue(const char* key, EmTagValue& value) const;
    size_t getString(const char* key, char* value, const size_t maxLen) const;
    String getString(const char* key, const char* defaultValue="") const;
    size_t getBytes(const char* key, void * buf, size_t maxLen) const;

    size_t getBytesLength(const char* key) const;
    size_t getStringLength(const char* key) const;

    template<typename T>
    bool isSameValue(const char* key, const T& value) const {
        return isSameBytes(key, &value, sizeof(value));
    }
    bool isSameValue(const char* key, EmTagValue& value) const;
    bool isSameString(const char* key, const String& value) const {
        return isSameString(key, value.c_str());
    }
    bool isSameString(const char* key, const char* value) const;
    bool isSameBytes(const char* key, const void * buf, size_t len) const;

    #if ESP_IDF_VERSION_MAJOR >= 5
    bool hasKey(const char* key) const { return nvm_find_key(m_handle, key, nullptr) == ESP_OK; }
    #else
    bool hasKey(const char* key) const { return hasValue(key) || hasString(key); }
    #endif
    bool hasValue(const char* key) const { return hasBytes(key); }
    bool hasBytes(const char* key) const { return getBytesLength(key) > 0;}
    bool hasString(const char* key) const { return getStringLength(key) > 0; }

    size_t freeEntries() const;
};

// A storage value that can be read and write within the provided 'tStorage' NVM storage.
//
// This base class has the templated 'ValueOfT' which should be derived from 'EmValue<T>'.
template<typename ValueOfT, typename T, EmStorage& tStorage>
class EmStorageValueBase: public ValueOfT {
public:
    using ValueOfT::ValueOfT;

    virtual const char* getKey() const = 0;

    virtual EmGetValueResult getValue(T& value) const override {
        T curVal;
        if (tStorage.getValue(getKey(), curVal) == 0) {
            return EmGetValueResult::failed;
        }
        if (value == curVal) {
            return EmGetValueResult::succeedEqualValue;        
        }
        value = curVal;
        return EmGetValueResult::succeedNotEqualValue;
    }

    virtual bool setValue(const T& value) override {
        if (tStorage.putValue(getKey(), value) == sizeof(value)) {
            return ValueOfT::setValue(value);
        }
        return false;
    }
    
    virtual T getValue() const {
        T value;
        if (tStorage.getBytes(getKey(), &value, sizeof(value)) == sizeof(value)) {
            return value;
        }
        return T();
    }

    virtual operator T() const {
        return getValue();
    }

    // Initialization methods (i.e. value is set only if key does not exist)
    template<typename T>
    size_t initValue(const T& value, bool commit=true) const {
        return tStorage.initValue<T>(getKey(), value, commit);
    }   

    size_t initValue(const EmTagValue& value, bool commit=true) const {
        return tStorage.initValue(getKey(), value, commit);
    }

    size_t initString(const char* value, bool commit=true) const {
        return tStorage.initString(getKey(), value, commit);
    }

    size_t initString(const String& value, bool commit=true) const {
        return tStorage.initString(getKey(), value, commit);
    }

    size_t initBytes(const void* value, size_t len, bool commit=true) const {
        return tStorage.initBytes(getKey(), value, commit);
    }   

};


// A storage value that can be read and written within the provided 'tStorage' NVM storage.
//
// This class is a simplified templated class deriving directly from 'EmValue<T>'
template<typename T, EmStorage& tStorage>
class EmStorageValue: public EmStorageValueBase<EmValue<T>, T, tStorage> {
protected:
    const char* m_key;

public:    
    EmStorageValue(const char* key) : m_key(key) {}

    virtual const char* getKey() const { return m_key; }
};


// This class provides 'EmStorageValue' plus an 'onSetValue' callback.
template<EmStorage& tStorage, class SelfT, typename T,
         EmOnSetValueCallbackType<SelfT, T> OnSetValue>
class EmStorageValueEx: public EmValueEx<EmStorageValue<T, tStorage>, SelfT, T, OnSetValue> {
public:
    using EmValueEx<EmStorageValue<T, tStorage>, SelfT, T, OnSetValue>::EmValueEx;
};


// A storage value that can be read and write within the provided 'tStorage' NVM storage.
//
// This class supports value synch between other 'EmSnycValue<T> values with the same key/id.
template<typename ValueOfT, typename T, EmStorage& tStorage>
class EmStorageSyncValue: public EmStorageValueBase<EmSyncValue<ValueOfT, T>, T, tStorage> {
public:    
    /// @brief The class constructor
    /// @param key The storage/sync value key/id
    /// @param flags The sync flags
    EmStorageSyncValue(const char* key, 
                       EmSyncFlags flags)
     : EmStorageValueBase<EmSyncValue<ValueOfT, T>, T, tStorage>(key) {
        EmSyncValue<ValueOfT, T>::m_flags = flags;
    }

    /// @brief The class constructor
    /// @param key The sync value key
    /// @param flags The sync flags
    /// @param tags The tags object olding all tags that will be synchronized by them ids.
    EmStorageSyncValue(const char* key, 
                       EmSyncFlags flags,
                       EmTags& tags)
     : EmStorageSyncValue<ValueOfT, T, tStorage>(key, flags) {
        tags.add(*this);
     }

     virtual const char* getId() const override { 
        return this->getKey(); 
    }     
};


// This class provides 'EmStorageSyncValue' plus an 'onSetValue' callback.
template<typename ValueOfT, typename T, EmStorage& tStorage,
         class SelfT,
         EmOnSetValueCallbackType<SelfT, EmTagValue> OnSetValue>
class EmStorageSyncValueEx: public EmValueEx<EmStorageSyncValue<ValueOfT, T, tStorage>, SelfT, EmTagValue, OnSetValue> {
public:
    using EmValueEx<EmStorageSyncValue<ValueOfT, T, tStorage>, SelfT, EmTagValue, OnSetValue>::EmValueEx;
};


// A storage value that can be read and write tags within the provided 'tStorage' NVM storage.
//
// This class supports value synch between other 'EmSnycValue<T> values with the same key/id.
template<EmStorage& tStorage>
class EmStorageTag: public EmStorageValueBase<EmTag, EmTagValue, tStorage> {
public:
    EmStorageTag(const char* key, 
                 EmSyncFlags flags)
     : EmStorageValueBase<EmTag, EmTagValue, tStorage>(key, flags) {}

    EmStorageTag(const char* key,
                 const EmTagValue& initValue, 
                 EmSyncFlags flags)
     : EmStorageValueBase<EmTag, EmTagValue, tStorage>(key, initValue, flags) {
        if (tStorage.isNotInitialized()) {
            tStorage.initValue(key, initValue, true);
        }
     }

    EmStorageTag(const char* key, 
                 EmSyncFlags flags,
                 EmTags& tags)
     : EmStorageTag<tStorage>(key, flags) {
        tags.add(*this);
    }

    EmStorageTag(const char* key,
                 const EmTagValue& initValue, 
                 EmSyncFlags flags,
                 EmTags& tags)
     : EmStorageTag<tStorage>(key, initValue, flags) {
        tags.add(*this);
     }

    virtual const char* getKey() const { 
        return EmTag::getId(); 
    }

    // Base class overloads
    using EmTag::getValue;
    using EmTag::setValue;
};


// This class provides 'EmStorageTag' plus an 'onSetValue' callback.
template<EmStorage& tStorage,
         class SelfT,
         EmOnSetValueCallbackType<SelfT, EmTagValue> OnSetValue>
class EmStorageTagEx: public EmValueEx<EmStorageTag<tStorage>, SelfT, EmTagValue, OnSetValue> {
public:
public:
    using EmValueEx<EmStorageTag<tStorage>, SelfT, EmTagValue, OnSetValue>::EmValueEx;
};

#endif
#endif // __EM_STORAGE__H_