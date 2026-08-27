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
#include "em_sbo_buffer.h"

#define EM_STORAGE_NULL_HANDLE 0

using EmNvsKeyString = EmString<NVS_KEY_NAME_MAX_SIZE-1>;

// The storage namespaces & keys iteration callback
// Note: a key can be an hashed one due to user longer key
typedef void (*EmStorageInfoCallback)(void* userArgs, 
                                      const char* nameSpace, 
                                      const char* keyName);

// Possible storage items types (this is an extention of ESP-IDF ones)
enum class EmStorageItemType: uint8_t {
    Undefined = 0,
    UInt8     = NVS_TYPE_U8,
    Int8      = NVS_TYPE_I8,
    UInt16    = NVS_TYPE_U16,
    Int16     = NVS_TYPE_I16,
    UInt32    = NVS_TYPE_U32,
    Int32     = NVS_TYPE_I32,
    UInt64    = NVS_TYPE_U64,
    Int64     = NVS_TYPE_I64,
    String    = NVS_TYPE_STR,
    Bytes     = NVS_TYPE_BLOB,
    // Extended types
    Bool      = 0x81,
    Float     = 0x82,
    Double    = 0x84,
    TagValue  = 0x88,
};

// Error codes for NVS operations
constexpr const char* _nvs_errors[] = {"UNDEFINED ERROR", 
                                       "NOT_INITIALIZED", 
                                       "NOT_FOUND", 
                                       "TYPE_MISMATCH", 
                                       "READ_ONLY", 
                                       "NOT_ENOUGH_SPACE", 
                                       "INVALID_NAME", 
                                       "INVALID_HANDLE", 
                                       "REMOVE_FAILED", 
                                       "KEY_TOO_LONG", 
                                       "PAGE_FULL", 
                                       "INVALID_STATE", 
                                       "INVALID_LENGTH"};
#define nvs_error(e) (((e)>ESP_ERR_NVS_BASE)?_nvs_errors[(e)&~(ESP_ERR_NVS_BASE)]:_nvs_errors[0])

// NVS persistent storage class
//
// Due to the NVS key limit of NVS_KEY_NAME_MAX_SIZE-1, if the requested key is bigger than
// NVS_KEY_NAME_MAX_SIZE-1 a hash key is generated to safely get and set longer key values.
//
// This class handles "post" initializations when begin() is called, this 
// allows to setup initial values before the storage is actually initialized.
// This is usefull when using EmStorageValue classes in global objects providing an 'initValue'.
class EmStorage: public EmLog {
    static constexpr char c_ResetVersionKey[] = "!#reset_ver#!"; // Let it be unique!
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

    // No copy allowed (move are removed as well)
    EmStorage(const EmStorage&) = delete;
    EmStorage& operator=(const EmStorage&) = delete;

    bool isInitialized() const {
        return m_handle != EM_STORAGE_NULL_HANDLE;
    }
    bool isNotInitialized() const {
        return !isInitialized();
    }

    // Start the new storage object by assigning a name to the used NVS namespace.
    // By assigning a 'resetVersion' bigger than zero the storage will store a version number 
    // and if the old stored version is different the old storage namespace will be reset/cleared.
    // This is usefull if some changes take place and you need to clear only once previous stored keys.
    bool begin(const char* name,
               uint16_t resetVersion=0);
    void end();
    bool clear() const;
    bool commit() const;


    // Gets the current reset version for this namespace (i.e. the name specified in the 'begin' method)
    // Returns zero if no reset version has been set or storage object is not initialized.
    uint16_t getCurrentResetVersion() const {
        if (isInitialized()) {
            uint16_t version = 0;
            if (nvs_get_u16(m_handle, c_ResetVersionKey, &version)==ESP_OK) {
                return version;
            }
        }
        return 0;
    }

    // Sets the current reset version for this namespace (i.e. the name specified in the 'begin' method)
    // Returns the new version or zero in case of error or storage object not being initialized.
    uint16_t setCurrentResetVersion(uint16_t version) const {
        if (isInitialized() &&
            nvs_set_u16(m_handle, c_ResetVersionKey, version)==ESP_OK) {
            return version;
        }
        return 0;
    }

    // Initialization methods (i.e. value is set only if key does not exist)
    template<typename T>
    bool initValue(const char* key, const T& value, bool commit=true) const {
        if (isNotInitialized()) {
            addToInitValue_(key, value);
            return sizeof(value);            
        }
        if (!hasKey(key)) {
            return setValue(key, value, commit);
        }
        return 0;
    }

    bool initValue(const char* key, const EmTagValue& value, bool commit=true) const {
        if (isNotInitialized()) {
            addToInitTags_(key, value);
            return sizeof(value);            
        }
        if (!hasKey(key)) {
            return setValue(key, value, commit);
        }
        return 0;
    }   
    bool initString(const char* key, const char* value, bool commit=true) const {
        if (isNotInitialized()) {
            addToInitStrings_(key, value);
            return strlen(value);            
        }
        if (!hasKey(key)) {
            return setString(key, value, commit);
        }
        return 0;
    }   
    bool initBytes(const char* key, const void* value, size_t len, bool commit=true) const {
        if (isNotInitialized()) {
            addToInitBytes_(key, value, len);
            return len;            
        }
        if (!hasKey(key)) {
            return setBytes(key, value, len, commit);
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
    bool setValue(const char* key, 
                  const T& value, 
                  bool commit=true,
                  bool equalityCheckBeforeWrite=true) const{
        if (!isInitialized() || key == nullptr || strlen(key) == 0) {
            return false;
        }    
        // Avoid writing the same value again
        if (equalityCheckBeforeWrite && isSameValue(key, value)) {
            return true; 
        }
        // Key check (creating hash if key is too long!)
        EmNvsKeyString keyBuf;
        key = getNvsKey(key, keyBuf);
        // Write the new value
        using cleanType = std::decay_t<T>;
        esp_err_t err = ESP_FAIL;
        if constexpr (std::is_same_v<cleanType, bool>) {
            err = nvs_set_i8(m_handle, key, (int8_t)value);
        }            
        else if constexpr (std::is_same_v<cleanType, char*> || std::is_same_v<cleanType, const char*>) {
            err = nvs_set_str(m_handle, key, value);
        } else if constexpr (std::is_integral_v<T>) {
            if constexpr (sizeof(T) == 1) {
                if constexpr (std::is_signed_v<T>) {
                    err = nvs_set_i8(m_handle, key, static_cast<int8_t>(value));
                } else {
                    err = nvs_set_u8(m_handle, key, static_cast<uint8_t>(value));
                }
            }
            else if constexpr (sizeof(T) == 2) {
                if constexpr (std::is_signed_v<T>) {
                    err = nvs_set_i16(m_handle, key, static_cast<int16_t>(value));
                } else {
                    err = nvs_set_u16(m_handle, key, static_cast<uint16_t>(value));
                }
            }
            else if constexpr (sizeof(T) == 4) {
                if constexpr (std::is_signed_v<T>) {
                    err = nvs_set_i32(m_handle, key, static_cast<int32_t>(value));
                } else {
                    err = nvs_set_u32(m_handle, key, static_cast<uint32_t>(value));
                }
            }
            else if constexpr (sizeof(T) == 8) {
                if constexpr (std::is_signed_v<T>) {
                    err = nvs_set_i64(m_handle, key, static_cast<int64_t>(value));
                } else {
                    err = nvs_set_u64(m_handle, key, static_cast<uint64_t>(value));
                }
            }
        }
        else if constexpr (std::is_same_v<cleanType, float>) {
            uint32_t raw_bits;
            memcpy(&raw_bits, &value, sizeof(raw_bits)); 
            err = nvs_set_u32(m_handle, key, raw_bits);            
        }
        else if constexpr (std::is_same_v<cleanType, double>) {
            uint64_t raw_bits;
            memcpy(&raw_bits, &value, sizeof(raw_bits));
            err = nvs_set_u64(m_handle, key, raw_bits);
        } else {
            static_assert(always_false<cleanType>, "Unsupported value type!");
            return 0;
        }    
        // Check result
        if (err != ESP_OK) {
            logError<100>("setValue failed: %s - %s", key, nvs_error(err));
            return 0;
        }
        if (commit && !this->commit()) {
            return false;
        }
        return true;
    }

    // Put a tag value into the NVS storage
    //
    // @param key The key name for the value.
    // @param value The value to be stored.
    // @param commit If true, commits the change to storage immediately.
    // @param equalityCheckBeforeWrite If true, checks if the value is different before writing it.
    bool setValue(const char* key, 
                  const EmTagValue& value, 
                  bool commit=true,
                  bool equalityCheckBeforeWrite=true) const;


    // Put a string value into the NVS storage
    //
    // @param key The key name for the value.
    // @param value The value to be stored.
    // @param commit If true, commits the change to storage immediately.
    // @param equalityCheckBeforeWrite If true, checks if the value is different before writing it.
    bool setString(const char* key, 
                   const char* value, 
                   bool commit=true,
                   bool equalityCheckBeforeWrite=true) const {
        return setValue(key, value, commit, equalityCheckBeforeWrite);
    }

    // Put generic bytes into the NVS storage as a blob.
    //
    // @param key The key name for the value.
    // @param value The value to be stored.
    // @param commit If true, commits the change to storage immediately.
    // @param equalityCheckBeforeWrite If true, checks if the value is different before writing it.
    bool setBytes(const char* key, 
                  const void* value, 
                  size_t len, 
                  bool commit=true,
                  bool equalityCheckBeforeWrite=true) const;

    template<typename T>
    bool getValue(const char* key, T& value) const {
        if (!isInitialized() || !key) {
            return false;
        }
        // Key check (creating hash if key is too long!)
        EmNvsKeyString keyBuf;
        key = getNvsKey(key, keyBuf);
        // Write the new value
        using cleanType = std::decay_t<T>;
        esp_err_t err = ESP_FAIL;
        if constexpr (std::is_same_v<cleanType, bool>) {
            int8_t v;
            err = nvs_get_i8(m_handle, key, &v);
            value = static_cast<T>(v);
        } 
        else if constexpr (std::is_integral_v<T>) {
            if constexpr (sizeof(T) == 1) {
                if constexpr (std::is_signed_v<T>) {
                    int8_t v;
                    err = nvs_get_i8(m_handle, key, &v);
                    if (err == ESP_OK) {
                        value = static_cast<T>(v);
                    }
                } else {
                    uint8_t v;
                    err = nvs_get_u8(m_handle, key, &v);
                    if (err == ESP_OK) {
                        value = static_cast<T>(v);
                    }
                }
            }
            else if constexpr (sizeof(T) == 2) {
                if constexpr (std::is_signed_v<T>) {
                    int16_t v;
                    err = nvs_get_i16(m_handle, key, &v);
                    if (err == ESP_OK) {
                        value = static_cast<T>(v);
                    }
                } else {
                    uint16_t v;
                    err = nvs_get_u16(m_handle, key, &v);
                    if (err == ESP_OK) {
                        value = static_cast<T>(v);
                    }
                }
            }
            else if constexpr (sizeof(T) == 4) {
                if constexpr (std::is_signed_v<T>) {
                    int32_t v;
                    err = nvs_get_i32(m_handle, key, &v);
                    if (err == ESP_OK) {
                        value = static_cast<T>(v);
                    }
                } else {
                    uint32_t v;
                    err = nvs_get_u32(m_handle, key, &v);
                    if (err == ESP_OK) {
                        value = static_cast<T>(v);
                    }
                }
            }
            else if constexpr (sizeof(T) == 8) {
                if constexpr (std::is_signed_v<T>) {
                    int64_t v;
                    err = nvs_get_i64(m_handle, key, &v);
                    if (err == ESP_OK) {
                        value = static_cast<T>(v);
                    }
                } else {
                    uint64_t v;
                    err = nvs_get_u64(m_handle, key, &v);
                    if (err == ESP_OK) {
                        value = static_cast<T>(v);
                    }
                }
            }
        }
        else if constexpr (std::is_same_v<cleanType, float>) {
            uint32_t raw_bits;
            err = nvs_get_u32(m_handle, key, &raw_bits);
            if (err == ESP_OK) {
                memcpy(&value, &raw_bits, sizeof(raw_bits));
            }            
        }
        else if constexpr (std::is_same_v<cleanType, double>) {
            uint64_t raw_bits;
            err = nvs_get_u64(m_handle, key, &raw_bits);
            if (err == ESP_OK) {
                memcpy(&value, &raw_bits, sizeof(raw_bits));
            }
        } else {
            static_assert(always_false<cleanType>, "Unsupported value type!");
            return false;
        }    
        // Check result
        if (err != ESP_OK) {
            logError<100>("getValue failed: %s - %s", key, nvs_error(err));
            return false;
        }
        return true;
    }

    template<typename T>
    T getValue(const char* key) const {
        T value;
        if (getBytes(key, &value, sizeof(value)) == sizeof(value)) {
            return value;
        }
        return T();
    }
    bool getValue(const char* key, EmTagValue& value) const;
    bool getString(const char* key, EmStringBase& value) const {
        size_t len = getString(key, value.buffer(), value.capacity());
        return len;
    }
    bool getString(const char* key, char* value, const size_t maxLen) const;
    bool getBytes(const char* key, void * buf, size_t maxLen) const;

    size_t getBytesLength(const char* key) const;
    size_t getStringLength(const char* key) const;

    template<typename T>
    bool isSameValue(const char* key, const T& value) const {
        using cleanType = std::decay_t<T>;
        if constexpr (std::is_same_v<cleanType, char*> || std::is_same_v<cleanType, const char*>) {
            return isSameString(key, value);
        } else {
            T nvsValue;
            if (!getValue(key, nvsValue)) {
                return false;
            }
            return value == nvsValue;
        }
    }
    bool isSameValue(const char* key, EmTagValue& value) const;
    bool isSameString(const char* key, const char* value) const;
    bool isSameBytes(const char* key, const void * buf, size_t len) const;

    bool hasKey(const char* key) const { 
        EmNvsKeyString keyBuffer;
        return nvs_find_key(m_handle, getNvsKey(key, keyBuffer), nullptr) == ESP_OK;
    }

    size_t getFreeEntriesCount();

    template<typename T>
    static EmStorageItemType getItemType() {
        using cleanType = std::decay_t<T>;
        if constexpr (std::is_same_v<cleanType, bool>) {
            return EmStorageItemType::Bool;
        }
        else if constexpr (std::is_integral_v<T>) {
            if constexpr (sizeof(T) == 1) {
                if constexpr (std::is_signed_v<T>) {
                    return EmStorageItemType::Int8;
                } else {
                    return EmStorageItemType::UInt8;
                }
            }
            else if constexpr (sizeof(T) == 2) {
                if constexpr (std::is_signed_v<T>) {
                    return EmStorageItemType::Int16;
                } else {
                    return EmStorageItemType::UInt16;
                }
            }
            else if constexpr (sizeof(T) == 4) {
                if constexpr (std::is_signed_v<T>) {
                    return EmStorageItemType::Int32;
                } else {
                    return EmStorageItemType::UInt32;
                }
            }
            else if constexpr (sizeof(T) == 8) {
                if constexpr (std::is_signed_v<T>) {
                    return EmStorageItemType::Int64;
                } else {
                    return EmStorageItemType::UInt64;
                }
            }
        }
        else if constexpr (std::is_same_v<cleanType, float>) {
            return EmStorageItemType::Float;
        }
        else if constexpr (std::is_same_v<cleanType, double>) {
            return EmStorageItemType::Double;
        } 
        else if constexpr (std::is_same_v<cleanType, EmTagValue>) {
            return EmStorageItemType::TagValue;
        }
        else if constexpr (std::is_same_v<cleanType, const char*> || std::is_same_v<cleanType, char*>) {
            return EmStorageItemType::String;
        }
        else if constexpr (std::is_same_v<cleanType, const void*> || std::is_same_v<cleanType, void*>) {
            return EmStorageItemType::Bytes;
        } else {
            static_assert(always_false<cleanType>, "Unsupported value type!");
            return EmStorageItemType::Undefined;
        }
    }

    
    // Iterate keys for all namespaces
    bool iterateKeys(EmStorageInfoCallback callback, void* userArgs=nullptr) {
        if (m_name != nullptr) {
            return iterateNamespaceKeys(m_name, callback, userArgs);
        }
        return false;
    }

    // Iterate all storage defined namespaces and keys
    static bool iterateNamespaces(EmStorageInfoCallback callback, void* userArgs=nullptr) {
        return iterateNamespaceKeys(nullptr, callback, userArgs);
    }

    // Iterate keys for a given namespace
    static bool iterateNamespaceKeys(const char* name, 
                                     EmStorageInfoCallback callback, 
                                     void* userArgs=nullptr);

    // Generates a valid NVS key (NVS_KEY_NAME_MAX_SIZE-1 characters) from a long string.
    // If the input exceeds NVS_KEY_NAME_MAX_SIZE-1 chars, it computes a 56-bit hash and converts it to HEX.
    // keyBuffer is only used if input exceeds NVS_KEY_NAME_MAX_SIZE-1 chars.
    static const char* getNvsKey(const char* key, EmNvsKeyString& keyBuffer);

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
    template<typename T>
    void addToInitValue_(const char* key, T value) const {
        addInitItem_(new InitItem_(key, value));
    }

    bool setValue_(EmStorageItemType type,
                   const char* key, 
                   void* valueBuf,
                   size_t valueLen) const;

    // NVS methods override (used to set the 'adjusted' key)
    esp_err_t nvs_set_str_(nvs_handle_t handle, const char* key, const char* value) const {
        EmNvsKeyString keyBuffer;
        return nvs_set_str(handle, getNvsKey(key, keyBuffer), value);
    }
    
    esp_err_t nvs_get_str_(nvs_handle_t handle, const char* key, char* out_value, size_t* length) const {
        EmNvsKeyString keyBuffer;
        return nvs_get_str(handle, getNvsKey(key, keyBuffer), out_value, length);
    }


    esp_err_t nvs_set_blob_(nvs_handle_t handle, const char* key, const void* value, size_t length) const {
        EmNvsKeyString keyBuffer;
        return nvs_set_blob(handle, getNvsKey(key, keyBuffer), value, length);
    }

    esp_err_t nvs_get_blob_(nvs_handle_t handle, const char* key, void* out_value, size_t* length) const {
        EmNvsKeyString keyBuffer;
        return nvs_get_blob(handle, getNvsKey(key, keyBuffer), out_value, length);
    }

private:
    // Type traits helper
    template<typename...> static constexpr bool always_false = false;

    // The storage nvs handle and namespace
    nvs_handle_t m_handle;
    const char* m_name;

    // Initialization items linked list
    struct InitItem_ {
        EmNvsKeyString key;
        EmStorageItemType type = EmStorageItemType::Undefined;
        EmSboBuffer<char, 16> bytes;
        size_t len = 0;
        InitItem_* next = nullptr;

        template<typename T>
        InitItem_(const char* key, T value) {
            EmStorageItemType type = getItemType<T>();                        
            init_(key, type, &value, sizeof(T));
        }
        InitItem_(const char* key, const void* value, size_t len) {
            init_(key, EmStorageItemType::Bytes, value, len);
        }
        InitItem_(const char* key, const char* value) {
            init_(key, EmStorageItemType::String, value, strlen(value)+1);
        }
        InitItem_(const char* key, const EmTagValue& value) {
            EmTagValueBuffer vb(value);
            init_(key, EmStorageItemType::TagValue, vb.getBuffer(), vb.getMaxSize());
        }
        ~InitItem_() = default;

        void init_(const char* k, EmStorageItemType t, const void* v, size_t l) {
            this->next = nullptr;
            getNvsKey(k, this->key);
            this->type = t;
            this->bytes.setMaxSize(l);
            memcpy(this->bytes.getBuffer(), v, l);
            this->len = l;
        }
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

    using ValueOfT::getValue;
    using ValueOfT::setValue;

    virtual bool setValue(const T& value) override {
        // NOTE: we do not check == sizeof(value) since value might be EmTagValue
        //       and its size might differ from the stored one due to internal allocations.
        return tStorage.setValue(getKey(), value) > 0;
    }
    
    virtual T getValue() const {
        T value;
        // NOTE: we do not check == sizeof(value) since value might be EmTagValue
        //       and its size might differ from the stored one due to internal allocations.
        if (tStorage.getValue(getKey(), value)) {
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

    size_t initValue(const EmTag& tag, bool commit=true) const {
        return initValue(static_cast<const EmTagValue&>(tag.getValue()), commit);
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

    using EmStorageValueBase<EmTag, EmTagValue, tStorage>::getValue;
    using EmStorageValueBase<EmTag, EmTagValue, tStorage>::setValue;

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

    template<typename V>
    bool setValue(const V& value) {
        EmTagValue tagValue(value);
        return this->setValue(tagValue);
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