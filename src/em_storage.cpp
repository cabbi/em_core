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
        m_handle = -1;
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
    char * value = NULL;
    size_t len = 0;
    if (!isInitialized() || !key) {
        return String(defaultValue);
    }
    esp_err_t err = nvs_get_str(m_handle, key, value, &len);
    if (err) {
        logError<50>("nvs_get_str len fail: %s %s", key, nvs_error(err));
        return String(defaultValue);
    }
    char buf[len];
    value = buf;
    err = nvs_get_str(m_handle, key, value, &len);
    if (err) {
        logError<50>("nvs_get_str fail: %s %s", key, nvs_error(err));
        return String(defaultValue);
    }
    return String(buf);
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