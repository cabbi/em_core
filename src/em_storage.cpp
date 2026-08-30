#include "em_storage.h"
#include <nvs_flash.h>

#ifdef EM_NVS

bool EmStorage::begin(const char * name, uint16_t resetVersion) {
    // Already initialized?
    if (isInitialized()) {
        return false;
    }

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }

    if (err != ESP_OK) {
        logError<100>("Init failed: %s", nvs_error(err));
        return false;
    }

    // Open ths nvs handle
    EmNvsKeyString keyBuf;
    err = nvs_open(getNvsKey(name, keyBuf), NVS_READWRITE, &m_handle);
    if (err != ESP_OK) {
        logError<100>("Begin failed: %s", nvs_error(err));
        return false;
    }
    m_name = name;

    // Reset version check
    if (resetVersion > 0 && // User asks version check!
        resetVersion != getCurrentResetVersion()) {
        if (!clear()) {
            nvs_close(m_handle);
            m_handle = EM_STORAGE_NULL_HANDLE;
            m_name = nullptr;
            return false;
        }
        setCurrentResetVersion(resetVersion);
    }

    // Process pending initializations
    if (m_initHead) {
        bool commitNeeded = false;
        InitItem_* cur = m_initHead;
        while (cur) {
            if (!hasKey(cur->key.c_str())) {
                if (setValue_(cur->type, cur->key.c_str(), (void*)cur->bytes.getBuffer(), cur->len)) {
                    commitNeeded = true;
                }
            }
            cur = cur->next;
        }
        if (commitNeeded) {
            commit();
        }
        clearInitItems_();
    }
    return true;
}

bool EmStorage::setValue_(EmStorageItemType type,
                          const char* key, 
                          void* valueBuf,
                          size_t valueLen) const {
    switch (type) {
        case EmStorageItemType::Undefined: 
            break;
        case EmStorageItemType::Bool: {
            bool value = static_cast<const char*>(valueBuf)[0] != 0;
            return setValue(key, value, false);
        }
        case EmStorageItemType::Int8: {
            int8_t value = static_cast<const char*>(valueBuf)[0];
            return setValue(key, value, false);
        }
        case EmStorageItemType::UInt8: {
            uint8_t value = static_cast<const char*>(valueBuf)[0];
            return setValue(key, value, false);
        }
        case EmStorageItemType::Int16: {
            int16_t value;
            memcpy(&value, valueBuf, sizeof(value));
            return setValue(key, value, false);
        }
        case EmStorageItemType::UInt16: {
            uint16_t value;
            memcpy(&value, valueBuf, sizeof(value));
            return setValue(key, value, false);
        }
        case EmStorageItemType::Int32: {
            int32_t value;
            memcpy(&value, valueBuf, sizeof(value));
            return setValue(key, value, false);
        }
        case EmStorageItemType::UInt32: {
            uint32_t value;
            memcpy(&value, valueBuf, sizeof(value));
            return setValue(key, value, false);
        }
        case EmStorageItemType::Int64: {
            int64_t value;
            memcpy(&value, valueBuf, sizeof(value));
            return setValue(key, value, false);
        }
        case EmStorageItemType::UInt64: {
            uint64_t value;
            memcpy(&value, valueBuf, sizeof(value));
            return setValue(key, value, false);
        }
        case EmStorageItemType::Float: {
            float value;
            memcpy(&value, valueBuf, sizeof(value));
            return setValue(key, value, false);
        }
        case EmStorageItemType::Double: {
            double value;
            memcpy(&value, valueBuf, sizeof(value));
            return setValue(key, value, false);
        }
        case EmStorageItemType::Bytes:
        case EmStorageItemType::TagValue:
            return setBytes(key, valueBuf, valueLen, false);
        case EmStorageItemType::String:
            return setString(key, (const char*)valueBuf, false);
    }
    return false;
}

void EmStorage::end() {
    if (isInitialized()) {
        nvs_close(m_handle);
        m_handle = EM_STORAGE_NULL_HANDLE;
        m_name = nullptr;
    }
}

bool EmStorage::clear() const {
    if (isNotInitialized()) {
        return false;
    }
    esp_err_t err = nvs_erase_all(m_handle);
    if (err != ESP_OK) {
        logError<100>("nvs_erase_all fail: %s", nvs_error(err));
        return false;
    }
    return commit();
}

bool EmStorage::commit() const {
    if (isNotInitialized()) {
        return false;
    }
    esp_err_t err = nvs_commit(m_handle);
    if (err != ESP_OK) {
        logError<100>("nvs_commit fail: %s", nvs_error(err));
        return false;
    }
    return true;
}

bool EmStorage::setValue(const char* key, 
                         const EmTagValue& value, 
                         bool commit,
                         bool equalityCheckBeforeWrite) const {
    EmTagValueBuffer tagBuffer(value);
    return setBytes(key, tagBuffer.getBuffer(), tagBuffer.getSize(), commit, equalityCheckBeforeWrite);
}

bool EmStorage::setBytes(const char* key, 
                         const void* value, 
                         size_t len, 
                         bool commit,
                         bool equalityCheckBeforeWrite) const {
    if (!isInitialized() || key == nullptr || value == nullptr || len == 0) {
        return false;
    }    
    // Avoid writing the same value again
    if (equalityCheckBeforeWrite && isSameBytes(key, value, len)) {
        return true; 
    }
    // Write the new value
    esp_err_t err = nvs_set_blob_(m_handle, key, value, len);
    if (err != ESP_OK) {
        logError<100>("nvs_set_blob failed: %s - %s", key, nvs_error(err));
        return 0;
    }
    if (commit && !this->commit()) {
        return false;
    }
    return true;
}

bool EmStorage::getValue(const char* key, EmTagValue& value) const {
    size_t size = getBytesLength(key);
    if (size == 0) {
        return false;
    }
    EmTagValueBuffer tagBuffer(size);
    if (!getBytes(key, tagBuffer.getBuffer(), size)) {
        return 0;
    }
    return tagBuffer.toValue(value);
}

bool EmStorage::getString(const char* key, 
                          char* value, 
                          const size_t maxLen) const {
    size_t len = 0;
    if (!isInitialized() || !key || !value || !maxLen) {
        return false;
    }
    esp_err_t err = nvs_get_str_(m_handle, key, NULL, &len);
    if (err != ESP_OK) {
        logError<100>("nvs_get_str len failed: %s - %s", key, nvs_error(err));
        return false;
    }
    if (len > maxLen) {
        logError<100>("not enough space in value: %u < %u", maxLen, len);
        return false;
    }
    err = nvs_get_str_(m_handle, key, value, &len);
    if (err != ESP_OK) {
        logError<100>("nvs_get_str failed: %s - %s", key, nvs_error(err));
        return false;
    }
    return true;
}

size_t EmStorage::getBytesLength(const char* key) const {
    size_t len = 0;
    if (!isInitialized() || !key) {
        return 0;
    }
    esp_err_t err = nvs_get_blob_(m_handle, key, NULL, &len);
    if (err != ESP_OK) {
        logDebug<100>("nvs_get_blob len failed: %s - %s", key, nvs_error(err));
        return 0;
    }
    return len;
}

size_t EmStorage::getStringLength(const char* key) const {
    size_t len = 0;
    if (!isInitialized() || !key) {
        return 0;
    }
    esp_err_t err = nvs_get_str_(m_handle, key, NULL, &len);
    if (err != ESP_OK) {
        logDebug<100>("nvs_get_str len failed: %s - %s", key, nvs_error(err));
        return 0;
    }
    return len-1; // -1 -> NVS gives back also null-terminator byte!
}

bool EmStorage::getBytes(const char* key, 
                         void * buf, 
                         size_t maxLen) const {
    size_t len = getBytesLength(key);
    if (!len || !buf || !maxLen) {
        return false;
    }
    if (len > maxLen) {
        logError<100>("not enough space in buffer: %u < %u", maxLen, len);
        return false;
    }
    esp_err_t err = nvs_get_blob_(m_handle, key, buf, &len);
    if (err != ESP_OK) {
        logError<100>("nvs_get_blob failed: %s - %s", key, nvs_error(err));
        return false;
    }
    return true;
}

bool EmStorage::isSameValue(const char* key, EmTagValue& value) const {
    // String special handling!?
    if (value.getType() == EmTagValueType::vt_string) {
        return isSameString(key, value.asString());
    }
    // Not a string, check the bytes
    EmTagValueBuffer vb(value);
    return isSameBytes(key, vb.getBuffer(), vb.getSize());
}

bool EmStorage::isSameString(const char* key, const char* value) const {
    // Check the size before allocating heap memory
    size_t len = strlen(value);
    size_t currLen = getStringLength(key);
    if (currLen != len) {
        return false;
    }
    // Check the actual string (+1 for the null termination)
    EmSboBuffer<char, 512> currBuf(len+1);
    esp_err_t err = nvs_get_str_(m_handle, key, currBuf.getBuffer(), &len+1);
    if (err != ESP_OK) {
        return false;
    }
    return 0==memcmp(value, currBuf.getBuffer(), len);
}

bool EmStorage::isSameBytes(const char* key, const void * buf, size_t len) const {
    // Check the size before allocating heap memory
    size_t currLen = getBytesLength(key);
    if (currLen != len) {
        return false;
    }
    // Check the actual bytes
    EmSboBuffer<char, 512> currBuf(len);
    esp_err_t err = nvs_get_blob_(m_handle, key, currBuf.getBuffer(), &len);
    if (err != ESP_OK) {
        return false;
    }
    return 0==memcmp(buf, currBuf.getBuffer(), len);
}

size_t EmStorage::getFreeEntriesCount() {
    nvs_stats_t nvs_stats;
    esp_err_t err = nvs_get_stats(m_name, &nvs_stats);
    if (err != ESP_OK) {
        logError<100>("Failed to get nvs statistics");
        return 0;
    }
    return nvs_stats.free_entries;
}

const char* EmStorage::getNvsKey(const char* key, EmNvsKeyString& keyBuffer) {
    if (key == nullptr) {
        keyBuffer.clear();
        return nullptr;
    }

    size_t len = strlen(key);

    // Key already fits within the native NVS limits (i.e. < NVS_KEY_NAME_MAX_SIZE)
    if (len < NVS_KEY_NAME_MAX_SIZE) {
        keyBuffer.set(key);
        return key;
    }

    // Key is too long (i.e >= NVS_KEY_NAME_MAX_SIZE). Compute FNV-1a 64-bit Hash
    uint64_t hash = 14695981039346656037ULL; // FNV offset basis
    const uint64_t fnvPrime = 1099511628211ULL; // FNV prime

    for (size_t i = 0; i < len; ++i) {
        hash ^= static_cast<uint8_t>(key[i]);
        hash *= fnvPrime;
    }

    // Convert the 7 bytes (56 bits) into hex (avoids heavy sprintf)
    const char hexChars[] = "0123456789ABCDEF";
    for (int i = 14; i >= 0; --i) {
        keyBuffer.setAt(i, hexChars[hash & 0x0F]);
        hash >>= 4;
    }
    keyBuffer.setAt(NVS_KEY_NAME_MAX_SIZE-1, 0);
    return keyBuffer.c_str();
}

bool EmStorage::iterateNamespaceKeys(const char* name, EmStorageInfoCallback callback, void* userArgs) {
    if (callback == NULL) {
        return false;
    }

    // "nvs.st.ns" is the internal NVS system namespace that indexes all user namespaces
    EmNvsKeyString nameBuffer; 
    nvs_iterator_t it = NULL;
    esp_err_t res = nvs_entry_find("nvs", getNvsKey(name, nameBuffer), NVS_TYPE_ANY, &it);
    if (res == ESP_ERR_NVS_NOT_FOUND) {
        return true;
    } else if (res != ESP_OK) {
        return false;
    }

    while (res == ESP_OK) {
        nvs_entry_info_t info;
        nvs_entry_info(it, &info);
        callback(userArgs, info.namespace_name, info.key);
        res = nvs_entry_next(&it);
    }
    nvs_release_iterator(it);
    return true;
}
#endif