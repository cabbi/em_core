#include "em_json_reader.h"
#include <em_log.h>

bool EmJsonDictReader::isValidJson() const {
    if (!m_jsonStr || m_jsonLen < 2) return false;
    size_t first = strspn(m_jsonStr, " \t\r\n");
    if (m_jsonStr[first] != '{') return false;

    size_t last = m_jsonLen - 1;
    while (last > first && (m_jsonStr[last] == ' ' || m_jsonStr[last] == '\t' || 
                            m_jsonStr[last] == '\r' || m_jsonStr[last] == '\n')) {
        last--;
    }
    return m_jsonStr[last] == '}';
}

bool EmJsonDictReader::getNestedObject(const char* parentKey, EmJsonInnerObj& outDict, const EmJsonInnerObj& parentScope) const {
    // Search inside the provided parent scope boundaries
    const char* valPtr = findValuePointer(parentKey, parentScope);
    if (!valPtr || *valPtr != '{') return false; 

    outDict.startIdx = valPtr - m_jsonStr; 
    
    int braceCount = 0;
    for (size_t i = outDict.startIdx; i < m_jsonLen; i++) {
        if (m_jsonStr[i] == '{') braceCount++;
        else if (m_jsonStr[i] == '}') braceCount--;

        if (braceCount == 0) {
            outDict.endIdx = i; 
            return true;
        }
    }
    return false; 
}

bool EmJsonDictReader::getString(const char* key, char* dest, size_t destMaxLen, const EmJsonInnerObj& scope) const {
    const char* valPtr = findValuePointer(key, scope);
    if (!valPtr || *valPtr != '"') return false;

    valPtr++; 
    const char* endQuote = strchr(valPtr, '"');
    if (!endQuote) return false;

    // Enforce the boundary limits of the current dictionary scope slice
    size_t absoluteEndLimit = (scope.endIdx > 0) ? scope.endIdx : m_jsonLen;
    if (endQuote >= (m_jsonStr + absoluteEndLimit)) return false;

    size_t valLen = endQuote - valPtr;
    if (valLen >= destMaxLen) valLen = destMaxLen - 1;

    strncpy(dest, valPtr, valLen);
    dest[valLen] = '\0';
    return true;
}

bool EmJsonDictReader::getInt(const char* key, int32_t& outValue, const EmJsonInnerObj& scope) const {
    const char* valPtr = findValuePointer(key, scope);
    if (!valPtr) return false;
    outValue = strtoul(valPtr, nullptr, 10);
    return true;
}

bool EmJsonDictReader::getFloat(const char* key, float& outValue, const EmJsonInnerObj& scope) const {
    const char* valPtr = findValuePointer(key, scope);
    if (!valPtr) return false;
    outValue = strtof(valPtr, nullptr);
    return true;
}

bool EmJsonDictReader::getDouble(const char* key, double& outValue, const EmJsonInnerObj& scope) const {
    const char* valPtr = findValuePointer(key, scope);
    if (!valPtr) return false;
    outValue = strtod(valPtr, nullptr);
    return true;
}

bool EmJsonDictReader::getBool(const char* key, bool& outValue, const EmJsonInnerObj& scope) const {
    const char* valPtr = findValuePointer(key, scope);
    if (!valPtr) return false;
    if (strncmp(valPtr, "true", 4) == 0) { outValue = true; return true; }
    if (strncmp(valPtr, "false", 5) == 0) { outValue = false; return true; }
    return false;
}

bool EmJsonDictReader::getTagValue(const char* key, EmTagValue& dest, const EmJsonInnerObj& scope) const {
    bool res = true;
    EmJsonValueType valueType = getValueType(key, scope);
    switch (valueType){
        case (EmJsonValueType::vt_string): {
                EmStringM value;
                getString(key, value);
                logDebug<100>("Json", "getTagValue(%s, %s) [STR]",key, value.c_str());                 
                res = dest.setString(value.c_str());                
            } break;
        case (EmJsonValueType::vt_integer): {
                int32_t value;
                getInt(key, value);
                logDebug<100>("Json", "getTagValue(%s, %d) [INT]",key, value);                 
                res = dest.setValue(value, true);                
            } break;
        case (EmJsonValueType::vt_real): {
                float value;
                getFloat(key, value);
                logDebug<100>("Json", "getTagValue(%s, %g) [FLOAT]",key, value);                 
                res = dest.setValue(value, true);
            } break;
        case (EmJsonValueType::vt_bool): {
                bool value;
                getBool(key, value);
                logDebug<100>("Json", "getTagValue(%s, %s) [BOOL]",key, value?"TRUE":"FALSE");                 
                res = dest.setValue(value, true);
            } break;
        case (EmJsonValueType::vt_timestamp): {
                EmEpoch32 value;
                getEpoch(key, value);
                res = dest.setEpoch(value, true);
            } break;
        default:
            res = false;
            break;
    }
    return res;
}

EmJsonValueType EmJsonDictReader::getValueType(const char* key, const EmJsonInnerObj& scope) const {
    const char* valptr = findValuePointer(key, scope);
    if (!valptr) {
        return EmJsonValueType::vt_undefined;
    }

    char firstchar = *valptr;

    // 1. Strings & Timestamps
    if (firstchar == '"') {
        // ISO 8601 timestamp check: looks for 4 digits followed by a dash (e.g., "YYYY-")
        // valptr points to the opening quote, so:
        // valptr[1] to valptr[4] must be digits, and valptr[5] must be '-'
        if (valptr[1] >= '1' && valptr[1] <= '2' &&
            valptr[2] >= '0' && valptr[2] <= '9' &&
            valptr[3] >= '0' && valptr[3] <= '9' &&
            valptr[4] >= '0' && valptr[4] <= '9' &&
            valptr[5] == '-') {
            return EmJsonValueType::vt_timestamp;
        }
        return EmJsonValueType::vt_string;
    }

    // 2. Dictionary / Object
    if (firstchar == '{') {
        return EmJsonValueType::vt_dict;
    }

    // 3. List / Array
    if (firstchar == '[') {
        return EmJsonValueType::vt_list;
    }

    // 4. Null
    if (strncmp(valptr, "null", 4) == 0) {
        return EmJsonValueType::vt_null;
    }

    // 5. Boolean
    if (strncmp(valptr, "true", 4) == 0 || strncmp(valptr, "false", 5) == 0) {
        return EmJsonValueType::vt_bool;
    }

    // 6. Numbers (Integer vs Real)
    if (firstchar == '-' || (firstchar >= '0' && firstchar <= '9')) {
        const char* p = valptr;
        // Scan the token until a JSON delimiter or whitespace is found
        while (*p && *p != ',' && *p != '}' && *p != ']' && *p != ' ' && *p != '\r' && *p != '\n') {
            if (*p == '.' || *p == 'e' || *p == 'E') {
                return EmJsonValueType::vt_real;
            }
            p++;
        }
        return EmJsonValueType::vt_integer;
    }

    return EmJsonValueType::vt_undefined;
}

const char* EmJsonDictReader::findValuePointer(const char* key, const EmJsonInnerObj& scope) const {
    if (!m_jsonStr || scope.startIdx >= m_jsonLen) return nullptr;
    
    size_t endIdx = (scope.endIdx == 0) ? m_jsonLen : scope.endIdx;

    char keyQuery[64];
    snprintf(keyQuery, sizeof(keyQuery), "\"%s\"", key);

    const char* currentSearchPtr = m_jsonStr + scope.startIdx;
    
    while (currentSearchPtr < (m_jsonStr + endIdx)) {
        const char* keyLoc = strstr(currentSearchPtr, keyQuery);
        if (!keyLoc || keyLoc >= (m_jsonStr + endIdx)) return nullptr;

        keyLoc += strlen(keyQuery);
        const char* colon = strchr(keyLoc, ':');
        if (!colon || colon >= (m_jsonStr + endIdx)) {
            currentSearchPtr = keyLoc; 
            continue;
        }

        const char* valStart = colon + 1;
        valStart += strspn(valStart, " \t\r\n");
        return valStart;
    }
    return nullptr;
}
