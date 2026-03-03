#include "em_storage.h"

#ifdef EM_NVS

// Error codes for NVS operations
const char* nvs_errors[] = { "UNDEFINED ERROR", 
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
#define nvs_error(e) (((e)>ESP_ERR_NVS_BASE)?nvs_errors[(e)&~(ESP_ERR_NVS_BASE)]:nvs_errors[0])


bool EmStorage::begin(const char * name, bool clearExisting) {
    // Already initialized?
    if (isInitialized()) {
        return false;
    }

    // Open ths nvs handle
    esp_err_t err = ESP_OK;
    err = nvs_open(name, NVS_READWRITE, &m_handle);
    if (err) {
        logError<50>("begin failed: %s", nvs_error(err));
        return false;
    }

    // Clear existing values?
    if (clearExisting) {
        if (!clear()) {
            nvs_close(m_handle);
            m_handle = EM_STORAGE_NULL_HANDLE;
            return false;
        }
    }

    // Process pending initializations
    if (m_initHead) {
        bool commitNeeded = false;
        InitItem_* cur = m_initHead;
        while (cur) {
            if (!hasKey(cur->key)) {
                switch (cur->type) {
                    case InitItemType_::bytes:
                        putBytes(cur->key, cur->bytes, cur->len, false);
                        break;
                    case InitItemType_::string:
                        putString(cur->key, cur->bytes, false);
                        break;
                }
                commitNeeded = true;
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

void EmStorage::end() {
    if (isInitialized()) {
        nvs_close(m_handle);
        m_handle = EM_STORAGE_NULL_HANDLE;
    }
}

bool EmStorage::clear() const {
    if (isNotInitialized()) {
        return false;
    }
    esp_err_t err = nvs_erase_all(m_handle);
    if (err) {
        logError<50>("nvs_erase_all fail: %s", nvs_error(err));
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
        logError<50>("nvs_commit fail: %s", nvs_error(err));
        return false;
    }
    return true;
}

size_t EmStorage::putValue(const char* key, 
                           const EmTagValue& value, 
                           bool commit,
                           bool equalityCheckBeforeWrite) const {
    EmTagValueBuffer tagBuffer(value);
    return putBytes(key, tagBuffer.buffer(), tagBuffer.size(), commit, equalityCheckBeforeWrite);
}

size_t EmStorage::putString(const char* key, 
                            const char* value, 
                            bool commit,
                            bool equalityCheckBeforeWrite) const {
    if (!isInitialized() || !key || !value) {
        return 0;
    }    
    // Avoid writing the same value again
    if (equalityCheckBeforeWrite && isSameString(key, value)) {
        return strlen(value); 
    }
    // Write the new value
    esp_err_t err = nvs_set_str(m_handle, key, value);
    if (err) {
        logError<50>("nvs_set_str fail: %s %s", key, nvs_error(err));
        return 0;
    }
    if (commit && !this->commit()) {
        return 0;
    }
    return strlen(value);
}

size_t EmStorage::putBytes(const char* key, 
                           const void* value, 
                           size_t len, 
                           bool commit,
                           bool equalityCheckBeforeWrite) const {
    if (!isInitialized() || !key || !value || !len) {
        return 0;
    }    
    // Avoid writing the same value again
    if (equalityCheckBeforeWrite && isSameBytes(key, value, len)) {
        return len; 
    }
    // Write the new value
    esp_err_t err = nvs_set_blob(m_handle, key, value, len);
    if (err) {
        logError<50>("nvs_set_blob fail: %s %s", key, nvs_error(err));
        return 0;
    }
    if (commit && !this->commit()) {
        return 0;
    }
    return len;
}

size_t EmStorage::getValue(const char* key, EmTagValue& value) const {
    size_t size = getBytesLength(key);
    if (size == 0) {
        return 0;
    }
    EmTagValueBuffer tagBuffer(size);
    if (getBytes(key, tagBuffer.buffer(), tagBuffer.size()) != size) {
        return 0;
    }
    return tagBuffer.toValue(value) ? size : 0;
}

size_t EmStorage::getString(const char* key, char* value, const size_t maxLen) const {
    size_t len = 0;
    if (!isInitialized() || !key || !value || !maxLen) {
        return 0;
    }
    esp_err_t err = nvs_get_str(m_handle, key, NULL, &len);
    if (err) {
        logDebug<50>("nvs_get_str len fail: %s %s", key, nvs_error(err));
        return 0;
    }
    if (len > maxLen) {
        logError<50>("not enough space in value: %u < %u", maxLen, len);
        return 0;
    }
    err = nvs_get_str(m_handle, key, value, &len);
    if (err) {
        logError<50>("nvs_get_str fail: %s %s", key, nvs_error(err));
        return 0;
    }
    return len;
}

size_t EmStorage::getBytesLength(const char* key) const {
    size_t len = 0;
    if (!isInitialized() || !key) {
        return 0;
    }
    esp_err_t err = nvs_get_blob(m_handle, key, NULL, &len);
    if (err) {
        logDebug<50>("nvs_get_blob len fail: %s %s", key, nvs_error(err));
        return 0;
    }
    return len;
}

size_t EmStorage::getStringLength(const char* key) const {
    size_t len = 0;
    if (!isInitialized() || !key) {
        return 0;
    }
    esp_err_t err = nvs_get_str(m_handle, key, NULL, &len);
    if (err) {
        logDebug<50>("nvs_get_str len fail: %s %s", key, nvs_error(err));
        return 0;
    }
    return len;
}

size_t EmStorage::getBytes(const char* key, void * buf, size_t maxLen) const {
    size_t len = getBytesLength(key);
    if (!len || !buf || !maxLen) {
        return len;
    }
    if (len > maxLen) {
        logError<50>("not enough space in buffer: %u < %u", maxLen, len);
        return 0;
    }
    esp_err_t err = nvs_get_blob(m_handle, key, buf, &len);
    if (err) {
        logError<50>("nvs_get_blob fail: %s %s", key, nvs_error(err));
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
    EmAutoPtr<char> currBuf(new char[len+1]);
    esp_err_t err = nvs_get_str(m_handle, key, currBuf.get(), &len);
    if (err) {
        return false;
    }
    return memcmp(value, currBuf.get(), len);
}

bool EmStorage::isSameBytes(const char* key, const void * buf, size_t len) const {
    // Check the size before allocating heap memory
    size_t currLen = getBytesLength(key);
    if (currLen != len) {
        return false;
    }
    // Check the actual bytes
    EmAutoPtr<char> currBuf(new char[len]);
    esp_err_t err = nvs_get_blob(m_handle, key, currBuf.get(), &len);
    if (err) {
        return false;
    }
    return 0==memcmp(buf, currBuf.get(), len);
}

size_t EmStorage::freeEntries() const {
    nvs_stats_t nvs_stats;
    esp_err_t err = nvs_get_stats(NULL, &nvs_stats);
    if (err) {
        logError<50>("Failed to get nvs statistics");
        return 0;
    }
    return nvs_stats.free_entries;
}

#endif