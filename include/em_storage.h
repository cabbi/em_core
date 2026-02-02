#ifndef __EM_STORAGE__H_
#define __EM_STORAGE__H_

#include "em_defs.h"

#ifdef EM_NVS

#include <nvs.h>

#include "em_log.h"
#include "em_value.h"
#include "em_value_sync.h"
#include "em_tag.h"
#include "em_string.h"

#define EM_STORAGE_NULL_HANDLE 0

// NVS persistent storage class
//
// This class handles "post" initializations when begin() is called, this 
// allows to setup initial values before the storage is actually initialized.
// This is usefull when using EmStorageValue classes in global objects providing an 'initValue'.
class EmStorage: public EmLog {
public:
    EmStorage(const char* logContext="EmStorage", 
              EmLogLevel logLevel = EmLogLevel::global)
     : EmLog(logContext, logLevel),
       m_handle(EM_STORAGE_NULL_HANDLE),
       m_initHead(nullptr) {}

    ~EmStorage() {
        end();
        clearInitItems_();
    }

    bool isInitialized() const {
        return m_handle != EM_STORAGE_NULL_HANDLE;
    }
    bool isNotInitialized() const {
        return !isInitialized();
    }

    bool begin(const char* name, bool clearExisting=false);
    void end();
    bool clear() const;
    bool commit() const;

    // Initialization methods (i.e. value is set only if key does not exist)
    template<typename T>
    size_t initValue(const char* key, const T& value, bool commit=true) const {
        if (isNotInitialized()) {
            addToInitBytes_(key, &value, sizeof(T));
            return sizeof(value);            
        }
        if (!hasKey(key)) {
            return putValue(key, value, commit);
        }
        return 0;
    }   
    size_t initValue(const char* key, const EmTagValue& value, bool commit=true) const {
        if (isNotInitialized()) {
            addToInitTags_(key, value);
            return sizeof(value);            
        }
        if (!hasKey(key)) {
            return putValue(key, value, commit);
        }
        return 0;
    }   
    size_t initString(const char* key, const char* value, bool commit=true) const {
        if (isNotInitialized()) {
            addToInitStrings_(key, value);
            return strlen(value);            
        }
        if (!hasKey(key)) {
            return putString(key, value, commit);
        }
        return 0;
    }   
    size_t initBytes(const char* key, const void* value, size_t len, bool commit=true) const {
        if (isNotInitialized()) {
            addToInitBytes_(key, value, len);
            return len;            
        }
        if (!hasKey(key)) {
            return putBytes(key, value, len, commit);
        }
        return 0;
    }   

    // Put a generic value into the NVS storage
    //
    // @param key The key name for the value.
    // @param value The value to be stored.
    // @param commit If true, commits the change to storage immediately.
    // @param equalityCheckBeforeWrite If true, checks if the value is different before writing it.
    template<typename T>
    size_t putValue(const char* key, 
                    const T& value, 
                    bool commit=true,
                    bool equalityCheckBeforeWrite=true) const {
        return putBytes(key, &value, sizeof(value), commit, equalityCheckBeforeWrite);
    }   

    // Put a tag value into the NVS storage
    //
    // @param key The key name for the value.
    // @param value The value to be stored.
    // @param commit If true, commits the change to storage immediately.
    // @param equalityCheckBeforeWrite If true, checks if the value is different before writing it.
    size_t putValue(const char* key, 
                    const EmTagValue& value, 
                    bool commit=true,
                    bool equalityCheckBeforeWrite=true) const;


    // Put a string value into the NVS storage
    //
    // @param key The key name for the value.
    // @param value The value to be stored.
    // @param commit If true, commits the change to storage immediately.
    // @param equalityCheckBeforeWrite If true, checks if the value is different before writing it.
    size_t putString(const char* key, 
                     const char* value, 
                     bool commit=true,
                     bool equalityCheckBeforeWrite=true) const;

    // Put generic bytes into the NVS storage
    //
    // @param key The key name for the value.
    // @param value The value to be stored.
    // @param commit If true, commits the change to storage immediately.
    // @param equalityCheckBeforeWrite If true, checks if the value is different before writing it.
    size_t putBytes(const char* key, 
                    const void* value, 
                    size_t len, 
                    bool commit=true,
                    bool equalityCheckBeforeWrite=true) const;

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
    template<size_t maxLen>
    size_t getString(const char* key, EmString<maxLen>& value) const {
        size_t len = getString(key, value.buffer(), maxLen);
        return len;
    }
    size_t getString(const char* key, char* value, const size_t maxLen) const;
    size_t getBytes(const char* key, void * buf, size_t maxLen) const;

    size_t getBytesLength(const char* key) const;
    size_t getStringLength(const char* key) const;

    template<typename T>
    bool isSameValue(const char* key, const T& value) const {
        return isSameBytes(key, &value, sizeof(value));
    }
    bool isSameValue(const char* key, EmTagValue& value) const;
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

protected:
    void addToInitBytes_(const char* key, const void* value, size_t len) const {
        addInitItem_(new InitItem_(key, value, len));
    }
    void addToInitStrings_(const char* key, const char* value) const {
        addInitItem_(new InitItem_(key, value));
    }
    void addToInitTags_(const char* key, const EmTagValue& value) const {
        addInitItem_(new InitItem_(key, value));
    }

private:
    // The storage nvs handle
    nvs_handle_t m_handle;

    // Initialization items linked list
    enum class InitItemType_: uint8_t { bytes, string, tag };
    struct InitItem_ {
        const char* key;
        InitItemType_ type;
        char* bytes = nullptr;
        size_t len = 0;
        InitItem_* next = nullptr;

        InitItem_(const char* key, const void* value, size_t len) {
            this->next = nullptr;
            this->key = key;
            this->type = InitItemType_::bytes;
            this->bytes = new char[len];
            memcpy(this->bytes, value, len);
            this->len = len;
        }
        InitItem_(const char* key, const char* value) {
            this->next = nullptr;
            this->key = key;
            this->type = InitItemType_::string;
            this->len = strlen(value)+1;
            this->bytes = new char[this->len];
            memcpy(this->bytes, value, this->len);
        }
        InitItem_(const char* key, const EmTagValue& value) {
            this->next = nullptr;
            this->key = key;
            this->type = InitItemType_::tag;
            this->len = sizeof(EmTagValueStruct);
            this->bytes = new char[this->len];
            memcpy(this->bytes, &value.asStruct(), this->len);
        }
        ~InitItem_() { if (bytes) delete[] bytes; }
    };

    void addInitItem_(InitItem_* item) const {
        if (!m_initHead) {
            m_initHead = item;
        } else {
            InitItem_* cur = m_initHead;
            while (cur->next) cur = cur->next;
            cur->next = item;
        }
    }

    void clearInitItems_() const {
        while (m_initHead) {
            InitItem_* next = m_initHead->next;
            delete m_initHead;
            m_initHead = next;
        }
    }

    mutable InitItem_* m_initHead;
};

// A storage value that can read and write within the provided 'tStorage' NVM storage.
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
        // NOTE: we do not chext == sizeof(value) since value might be EmTagValue
        //       and its size might differ from the stored one due to internal allocations.
        return tStorage.putValue(getKey(), value) > 0;
    }
    
    virtual T getValue() const {
        T value;
        // NOTE: we do not chext == sizeof(value) since value might be EmTagValue
        //       and its size might differ from the stored one due to internal allocations.
        if (tStorage.getBytes(getKey(), &value, sizeof(value)) > 0) {
            return value;
        }
        return T();
    }

    virtual operator T() const {
        return getValue();
    }

    // Initialization methods (i.e. value is set only if key does not exist)
    template<typename V>
    size_t initValue(const V& value, bool commit=true) const {
        return tStorage.initValue<V>(getKey(), value, commit);
    }   

    size_t initValue(const EmTagValue& value, bool commit=true) const {
        return tStorage.initValue(getKey(), value, commit);
    }

    size_t initString(const char* value, bool commit=true) const {
        return tStorage.initString(getKey(), value, commit);
    }

    size_t initBytes(const void* value, size_t len, bool commit=true) const {
        return tStorage.initBytes(getKey(), value, commit);
    }   
};

// A storage value that can read and written within the provided 'tStorage' NVM storage.
//
// This class is a simplified templated class deriving directly from 'EmValue<T>'
template<typename T, EmStorage& tStorage>
class EmStorageValue: public EmStorageValueBase<EmValue<T>, T, tStorage> {
protected:
    const char* m_key;

public:    
    EmStorageValue(const char* key) : m_key(key) {}
    EmStorageValue(const char* key, T initValue) : m_key(key) {
        tStorage.initValue<T>(key, initValue, true);
    }

    virtual const char* getKey() const { return m_key; }
};


// This class provides 'EmStorageValue' plus an 'onSetValue' callback.
template<EmStorage& tStorage, class SelfT, typename T,
         EmOnSetValueCallbackType<SelfT, T> OnSetValue>
class EmStorageValueEx: public EmValueEx<EmStorageValue<T, tStorage>, SelfT, T, OnSetValue> {
public:
    using EmValueEx<EmStorageValue<T, tStorage>, SelfT, T, OnSetValue>::EmValueEx;
};


// A storage value that can read and write within the provided 'tStorage' NVM storage.
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
    /// @param tags The tags object holding all tags that will be synchronized by them ids.
    EmStorageSyncValue(const char* key, 
                       EmSyncFlags flags,
                       EmTagsAdd& tags)
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


// A storage value that can read and write tags within the provided 'tStorage' NVM storage.
//
// This class supports value synch between other 'EmSnycValue<T> values with the same key/id.
// It is derived from EmTag in order to have cached value for more performant read & write operations.
template<EmStorage& tStorage>
class EmStorageTag: public EmStorageValueBase<EmTag, EmTagValue, tStorage> {
public:
    EmStorageTag(const char* key, 
                 EmSyncFlags flags)
     : EmStorageValueBase<EmTag, EmTagValue, tStorage>(key, flags) {
        // Set the tag as undefined to read it the for the first time
        EmTag::m_value.setUndefinedType();
     }

    EmStorageTag(const char* key,
                 const EmTagValue& initValue, 
                 EmSyncFlags flags)
     : EmStorageValueBase<EmTag, EmTagValue, tStorage>(key, initValue, flags) {
        tStorage.initValue(key, initValue, true);
        // Set the tag as undefined to read it the for the first time
        EmTag::m_value.setUndefinedType();
     }

    EmStorageTag(const char* key, 
                 EmSyncFlags flags,
                 EmTagsAdd& tags)
     : EmStorageTag<tStorage>(key, flags) {
        tags.add(*this);
    }

    EmStorageTag(const char* key,
                 const EmTagValue& initValue, 
                 EmSyncFlags flags,
                 EmTagsAdd& tags)
     : EmStorageTag<tStorage>(key, initValue, flags) {
        tags.add(*this);
     }

    virtual const char* getKey() const { 
        return EmTag::getId(); 
    }

    // Base Tag class overrides
    virtual EmGetValueResult getValue(EmTagValue& value) const override {
        // Do we have a value already assigned to this tag object? 
        if (EmTag::m_value.isNotUndefinedType()) {
            return EmTag::m_value.getValue(value);
        }
        // Get the value from storage
        EmGetValueResult res = EmStorageValueBase<EmTag, EmTagValue, tStorage>::getValue(value);
        if (res != EmGetValueResult::failed) {
            // Cache the current storage value into the tag object
            const_cast<EmTagValue&>(EmTag::m_value).setValue(value);
        }
        return res;
    }

    template<typename T>
    EmGetValueResult getValue(T& value) const {
        EmTagValue tagValue;
        EmGetValueResult res = this->getValue(tagValue);
        if (res != EmGetValueResult::failed) {
            return tagValue.getValue<T>(value);
        }
        return res;
    }

    virtual bool setValue(const EmTagValue& value) override {
        // Perform an equality check in case tag value is already available
        // This is done to avoid NVS reading.
        if (EmTag::m_value.isNotUndefinedType() && EmTag::m_value == value) {
            return true;
        }
        // Value is different, lets store it!
        if (EmStorageValueBase<EmTag, EmTagValue, tStorage>::setValue(value)) {
            return EmTag::setValue(value);
        }
        // Storage failed!
        return false;
    }

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