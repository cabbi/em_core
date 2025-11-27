#ifndef __EM_STORAGE__H_
#define __EM_STORAGE__H_

#include "em_defs.h"

#ifdef EM_NVS

#include <cstring>
#include <WString.h>
#include "nvs.h"

#include "em_log.h"
#include "em_value.h"
#include "em_value_sync.h"
#include "em_tag.h"


// NVS persistent storage class
class EmStorage: public EmLog {
private:
    nvs_handle_t m_handle;

public:
    EmStorage(const char* logContext="EmStorage", 
              EmLogLevel logLevel = EmLogLevel::global)
     : EmLog(logContext, logLevel),
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
    size_t putString(const char* key, const String& value, bool commit=true) const;
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

    #if ESP_IDF_VERSION_MAJOR >= 5
    bool hasKey(const char* key) const { return nvm_find_key(m_handle, key, nullptr) == ESP_OK; }
    #else
    bool hasKey(const char* key) const { return hasValue(key); }
    #endif
    bool hasValue(const char* key) const { return hasBytes(key); }
    bool hasBytes(const char* key) const { return getBytesLength(key) > 0;}
    bool hasString(const char* key) const { return getStringLength(key) > 0; }

    size_t freeEntries() const;
};

// A storage value that can be read and write within the provided 'tStorage' NVM storage.
//
// This base class has the templated 'ValueOfT' which should be derived from 'EmValue<T>'.
template<typename ValueOfT, typename T, const EmStorage& tStorage>
class EmStorageValueBase: public ValueOfT {
protected:
    const char* m_key;

public:
    EmStorageValueBase(const char* key) : m_key(key) {}

    virtual ~EmStorageValueBase() = default;

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
    
    virtual T getValue() const {
        T value;
        if (tStorage.getBytes(m_key, &value, sizeof(value)) == sizeof(value)) {
            return value;
        }
        return T();
    }

    virtual operator T() const {
        return getValue();
    }
};


// A storage value that can be read and write within the provided 'tStorage' NVM storage.
//
// This class is a simplified templated class deriving directly from 'EmValue<T>'
template<typename T, const EmStorage& tStorage>
class EmStorageValue: public EmStorageValueBase<EmValue<T>, T, tStorage> {
};


// This class provides 'EmStorageValue' plus an 'onSetValue' callback.
template<typename T, const EmStorage& tStorage>
class EmStorageValueEx: public EmValueEx<EmStorageValue<T, tStorage>, T> {
public:
    /// @brief The class constructor
    /// @param key The storage value key
    /// @param flags The sync flags
    EmStorageValueEx(const char* key, 
                     EmOnSetValueCallbackType<T> onSetValue)
     : EmValueEx<EmStorageValue<T, tStorage>, T>(onSetValue),
       EmStorageValue<T, tStorage>(key) {}

    virtual ~EmStorageValueEx() = default;
};


// A storage value that can be read and write within the provided 'tStorage' NVM storage.
//
// This class supports value synch between other 'EmSnycValue<T> values with the same key/id.
template<typename ValueOfT, typename T, const EmStorage& tStorage>
class EmStorageSyncValue: public EmStorageValueBase<EmSyncValue<ValueOfT, T>, T, tStorage> {
    
    /// @brief The class constructor
    /// @param key The storage/sync value key/id
    /// @param flags The sync flags
    EmStorageSyncValue(const char* key, 
                       EmSyncFlags flags)
     : EmStorageValueBase<EmSyncValue<ValueOfT, T>, T, tStorage>(key),
       EmSyncValue<ValueOfT, T>(flags) {}

    /// @brief The class constructor
    /// @param key The sync value key
    /// @param flags The sync flags
    /// @param tags The tags object olding all tags that will be synchronized by them ids.
    EmStorageSyncValue(const char* key, 
                       EmSyncFlags flags,
                       EmTags& tags)
     : EmStorageValueBase<EmSyncValue<ValueOfT, T>, T, tStorage>(key),
       EmSyncValue<ValueOfT, T>(flags)  {
        tags.add(*this);
     }

     virtual const char* getId() const override { 
        return this->getKey(); 
    }     
};


// This class provides 'EmStorageSyncValue' plus an 'onSetValue' callback.
template<typename ValueOfT, typename T, const EmStorage& tStorage>
class EmStorageSyncValueEx: public EmValueEx<EmStorageSyncValue<ValueOfT, T, tStorage>, T> {
public:
    /// @brief The class constructor
    /// @param key The storage/sync value key/id
    /// @param flags The sync flags
    EmStorageSyncValueEx(const char* key, 
                         EmSyncFlags flags,
                         EmOnSetValueCallbackType<T> onSetValue)
     : EmValueEx<EmStorageSyncValue<ValueOfT, T, tStorage>, T>(onSetValue),
       EmStorageSyncValue<ValueOfT, T, tStorage>(key, flags) {}

    /// @brief The class constructor
    /// @param key The sync value key
    /// @param flags The sync flags
    /// @param tags The tags object olding all tags that will be synchronized by them ids.
    EmStorageSyncValueEx(const char* key, 
                         EmSyncFlags flags,
                         EmOnSetValueCallbackType<T> onSetValue,
                         EmTags& tags)
     : EmValueEx<EmStorageSyncValue<ValueOfT, T, tStorage>, T>(onSetValue),
       EmStorageSyncValue<ValueOfT, T, tStorage>(key, flags) {
        tags.add(*this);
     }
};


// A storage value that can be read and write tags within the provided 'tStorage' NVM storage.
//
// This class supports value synch between other 'EmSnycValue<T> values with the same key/id.
template<const EmStorage& tStorage>
class EmStorageTag: public EmStorageValue<EmTag, tStorage> {
public:
    EmStorageTag(const char* key, 
                 EmSyncFlags flags)
     : EmStorageValueBase<EmTag, EmTagValue, tStorage>(key),
       EmTag(key, flags) {}

    EmStorageTag(const char* key, 
                 EmSyncFlags flags,
                 EmTags& tags)
     : EmStorageTag(key, flags) {
        tags.add(*this);
     }
};


// This class provides 'EmStorageTag' plus an 'onSetValue' callback.
template<const EmStorage& tStorage>
class EmStorageTagEx: public EmValueEx<EmStorageTag<tStorage>, EmTagValue> {
public:
    /// @brief The class constructor
    /// @param key The storage/sync value key/id
    /// @param flags The sync flags
    EmStorageTagEx(const char* key, 
                   EmSyncFlags flags,
                   EmOnSetValueCallbackType<EmTagValue> onSetValue)
     : EmValueEx<EmStorageTag<tStorage>, EmTagValue>(onSetValue),
       EmStorageTag<tStorage>(key, flags) {}

    /// @brief The class constructor
    /// @param key The sync value key
    /// @param flags The sync flags
    /// @param tags The tags object olding all tags that will be synchronized by them ids.
    EmStorageTagEx(const char* key, 
                   EmSyncFlags flags,
                   EmOnSetValueCallbackType<EmTagValue> onSetValue,
                   EmTags& tags)
     : EmValueEx<EmStorageTag<tStorage>, EmTagValue>(onSetValue),
       EmStorageTag<tStorage>(key, flags) {
        tags.add(*this);
     }
};

#endif
#endif // __EM_STORAGE__H_