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
    if (err) {
        logError<100>("Begin failed: %s", nvs_error(err));
        return false;
    }
    m_name = name;

    // Reset version check
    if (resetVersion > 0 &&                          // User asks version check!
        resetVersion != getCurrentResetVersion() &&  // We have new version!
        0 != setCurrentResetVersion(resetVersion)) { // We can set new version!
        if (!clear()) {
            nvs_close(m_handle);
            m_handle = EM_STORAGE_NULL_HANDLE;
            m_name = nullptr;
            return false;
        }
    }

    // Process pending initializations
    if (m_initHead) {
        bool commitNeeded = false;
        InitItem_* cur = m_initHead;
        while (cur) {
            if (!hasKey(cur->key)) {
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
    if (err) {
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
    if (err) {
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
    return setBytes(key, tagBuffer.getBuffer(), tagBuffer.getMaxSize(), commit, equalityCheckBeforeWrite);
}

bool EmStorage::setBytes(const char* key, 
                         const void* value, 
                         size_t len, 
                         bool commit,
                         bool equalityCheckBeforeWrite) const {
    if (!isInitialized() || !key || !value || !len) {
        return false;
    }    
    // Avoid writing the same value again
    if (equalityCheckBeforeWrite && isSameBytes(key, value, len)) {
        return true; 
    }
    // Write the new value
    esp_err_t err = nvs_set_blob_(m_handle, key, value, len);
    if (err) {
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
        return 0;
    }
    EmTagValueBuffer tagBuffer(size);
    if (getBytes(key, tagBuffer.getBuffer(), tagBuffer.getMaxSize()) != size) {
        return 0;
    }
    return tagBuffer.toValue(value) ? size : 0;
}

bool EmStorage::getString(const char* key, 
                          char* value, 
                          const size_t maxLen) const {
    size_t len = 0;
    if (!isInitialized() || !key || !value || !maxLen) {
        return 0;
    }
    esp_err_t err = nvs_get_str_(m_handle, key, NULL, &len);
    if (err) {
        logDebug<100>("nvs_get_str len failed: %s - %s", key, nvs_error(err));
        return 0;
    }
    if (len > maxLen) {
        logError<100>("not enough space in value: %u < %u", maxLen, len);
        return 0;
    }
    err = nvs_get_str_(m_handle, key, value, &len);
    if (err) {
        logError<100>("nvs_get_str failed: %s - %s", key, nvs_error(err));
        return 0;
    }
    return len;
}

size_t EmStorage::getBytesLength(const char* key) const {
    size_t len = 0;
    if (!isInitialized() || !key) {
        return 0;
    }
    esp_err_t err = nvs_get_blob_(m_handle, key, NULL, &len);
    if (err) {
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
    if (err) {
        logDebug<100>("nvs_get_str len failed: %s - %s", key, nvs_error(err));
        return 0;
    }
    return len;
}

bool EmStorage::getBytes(const char* key, 
                         void * buf, 
                         size_t maxLen) const {
    size_t len = getBytesLength(key);
    if (!len || !buf || !maxLen) {
        return len;
    }
    if (len > maxLen) {
        logError<100>("not enough space in buffer: %u < %u", maxLen, len);
        return 0;
    }
    esp_err_t err = nvs_get_blob_(m_handle, key, buf, &len);
    if (err) {
        logError<100>("nvs_get_blob failed: %s - %s", key, nvs_error(err));
        return 0;
    }
    return len;
}

bool EmStorage::isSameValue(const char* key, EmTagValue& value) const {
    // String special handling!?
    if (value.getType() == EmTagValueType::vt_string) {
        return isSameString(key, value.asString());
    }
    // Not a string, check the bytes
    return isSameBytes(key, &value.asStruct(), sizeof(EmTagValueStruct));
}

bool EmStorage::isSameString(const char* key, const char* value) const {
    // Check the size before allocating heap memory
    size_t len = strlen(value);
    size_t currLen = getStringLength(key);
    if (currLen != len) {
        return false;
    }
    // Check the actual string
    EmSboBuffer<char, 512> currBuf(len+1);
    esp_err_t err = nvs_get_str_(m_handle, key, currBuf.getBuffer(), &len);
    if (err) {
        return false;
    }
    return memcmp(value, currBuf.getBuffer(), len);
}

bool EmStorage::isSameBytes(const char* key, const void * buf, size_t len) const {
    // Check the size before allocating heap memory
    size_t currLen = getBytesLength(key);
    if (currLen != len) {
        return false;
    }
    // Check the actual bytes
    EmAutoPtr<char[]> currBuf(new char[len]);
    esp_err_t err = nvs_get_blob_(m_handle, key, currBuf.get(), &len);
    if (err) {
        return false;
    }
    return 0==memcmp(buf, currBuf.get(), len);
}

size_t EmStorage::getFreeEntriesCount() {
    nvs_stats_t nvs_stats;
    esp_err_t err = nvs_get_stats(m_name, &nvs_stats);
    if (err) {
        logError<100>("Failed to get nvs statistics");
        return 0;
    }
    return nvs_stats.free_entries;
}

const char* EmStorage::getNvsKey(const char* key, EmNvsKeyString& keyBuffer) {
    if (key == nullptr || keyBuffer == nullptr) {
        return nullptr;
    }

    size_t len = strlen(key);

    // Key already fits within the native NVS limits (i.e. < NVS_KEY_NAME_MAX_SIZE)
    if (len < NVS_KEY_NAME_MAX_SIZE) {
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
    const char hexChars[] = "0123456789ABCDF";
    for (int i = 14; i >= 0; --i) {
        keyBuffer.setAt(i, hexChars[hash & 0x0F]);
        hash >>= 4;
    }
    keyBuffer.setAt(NVS_KEY_NAME_MAX_SIZE-1, 0);
::logInfo<100>("EmStorage", "Set hashed key: %s", keyBuffer.c_str()); // CABBI
    return keyBuffer.c_str();
}

bool EmStorage::iterateNamespaces(EmStorageNamespaceInfoCallback callback, void* user_arg) {
    if (callback == NULL) {
        return false;
    }
    
    esp_partition_iterator_t part_it = esp_partition_find(ESP_PARTITION_TYPE_DATA, 
                                                          ESP_PARTITION_SUBTYPE_DATA_NVS, 
                                                          NULL);
    if (part_it == NULL) {
        return false;
    }

    while (part_it != NULL) {
        const esp_partition_t* part = esp_partition_get(part_it);
        callback(user_arg, part->label);
        part_it = esp_partition_next(part_it);
    }
    
    esp_partition_iterator_release(part_it);
    return true;
}

bool EmStorage::iterateNamespaceKeys(const char* name, EmStorageKeyInfoCallback callback, void* user_arg) {
    if (callback == NULL) {
        return false;
    }

    // "nvs.st.ns" is the internal NVS system namespace that indexes all user namespaces
    nvs_iterator_t it = NULL;
    esp_err_t res = nvs_entry_find(name, "nvs.st.ns", NVS_TYPE_ANY, &it);
    if (res == ESP_ERR_NVS_NOT_FOUND) {
        return true;
    } else if (res != ESP_OK) {
        return false;
    }

    while (res == ESP_OK) {
        nvs_entry_info_t info;
        nvs_entry_info(it, &info);
        callback(user_arg, info.namespace_name, info.key);
        res = nvs_entry_next(&it);
    }
    nvs_release_iterator(it);
    return true;
}
#endif