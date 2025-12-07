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


bool EmStorage::begin(const char * name) {
    if (isInitialized()) {
        return false;
    }
    esp_err_t err = ESP_OK;
    err = nvs_open(name, NVS_READWRITE, &m_handle);
    if (err) {
        logError<50>("begin failed: %s", nvs_error(err));
        return false;
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

size_t EmStorage::putValue(const char* key, const EmTagValue& value, bool commit) const {
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
    const EmTagValueStruct& valueBytes = value.asStruct();
    return putBytes(key, &valueBytes, sizeof(valueBytes), commit);
}

size_t EmStorage::putString(const char* key, const char* value, bool commit) const {
    if (!isInitialized() || !key || !value) {
        return 0;
    }
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

size_t EmStorage::putString(const char* key, const String& value, bool commit) const {
    return putString(key, value.c_str(), commit);
}

size_t EmStorage::putBytes(const char* key, const void* value, size_t len, bool commit) const {
    if (!isInitialized() || !key || !value || !len) {
        return 0;
    }
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

size_t EmStorage::getString(const char* key, char* value, const size_t maxLen) const {
    size_t len = 0;
    if (!isInitialized() || !key || !value || !maxLen) {
        return 0;
    }
    esp_err_t err = nvs_get_str(m_handle, key, NULL, &len);
    if (err) {
        logError<50>("nvs_get_str len fail: %s %s", key, nvs_error(err));
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

String EmStorage::getString(const char* key, const char* defaultValue) const {
    size_t len = 0;
    if (!isInitialized() || !key) {
        return String(defaultValue);
    }
    esp_err_t err = nvs_get_str(m_handle, key, nullptr, &len);
    if (err) {
        logError<50>("nvs_get_str len fail: %s %s", key, nvs_error(err));
        return String(defaultValue);
    }
    EmAutoPtr<char> buf(new char[len+1]);
    err = nvs_get_str(m_handle, key, buf.get(), &len);
    if (err) {
        logError<50>("nvs_get_str fail: %s %s", key, nvs_error(err));
        return String(defaultValue);
    }
    return String(buf.get());
}

size_t EmStorage::getBytesLength(const char* key) const {
    size_t len = 0;
    if (!isInitialized() || !key) {
        return 0;
    }
    esp_err_t err = nvs_get_blob(m_handle, key, NULL, &len);
    if (err) {
        logError<50>("nvs_get_blob len fail: %s %s", key, nvs_error(err));
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
        logError<50>("nvs_get_str len fail: %s %s", key, nvs_error(err));
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