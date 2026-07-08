#ifndef __EM_JSON_READER_H
#define __EM_JSON_READER_H

#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <type_traits>

#include "em_string.h"
#include "em_tag.h"

// The tag value type
enum class EmJsonValueType: uint8_t {
    vt_undefined = 0,
    vt_null = 1,
    vt_boolean = 2,
    vt_integer = 3,
    vt_real = 4,
    vt_string = 5,
    vt_timestamp = 6,
    vt_dict = 7,
    vt_list = 8
};

struct EmJsonInnerObj {
    size_t startIdx = 0;
    size_t endIdx = 0;

    bool isValid() const { 
        return (endIdx > startIdx && endIdx != 0); 
    }
};

// This is a tiny JSON reader that can extract values from a JSON string without using ArduinoJson or any other library.
// This is not perfect nor complete Json handling class, but it is sufficient for our needs to extract values from
// a JSON string without allocating memory on the heap.
class EmJsonDictReader {
public:
    EmJsonDictReader(const char* jsonStr)
     : m_jsonStr(jsonStr), m_jsonLen(0) {
        if (m_jsonStr) {
            m_jsonLen = strlen(m_jsonStr);
        }
    }

    bool isValidJson() const;

    bool getNestedObject(const char* parentKey, EmJsonInnerObj& outDict, const EmJsonInnerObj& parentScope = EmJsonInnerObj()) const;

    EmJsonValueType getValueType(const char* key, const EmJsonInnerObj& scope = EmJsonInnerObj()) const;

    bool getString(const char* key, char* dest, size_t destMaxLen, const EmJsonInnerObj& scope = EmJsonInnerObj()) const;
    template<size_t maxLen>
    bool getString(const char* key, EmString<maxLen>& dest, const EmJsonInnerObj& scope = EmJsonInnerObj()) const {
        return getString(key, dest.buffer(), dest.capacity(), scope);
    }
    bool getBool(const char* key, bool& outValue, const EmJsonInnerObj& scope = EmJsonInnerObj()) const; 
    bool getInt(const char* key, int32_t& outValue, const EmJsonInnerObj& scope = EmJsonInnerObj()) const;
    bool getFloat(const char* key, float& outValue, const EmJsonInnerObj& scope = EmJsonInnerObj()) const;
    bool getDouble(const char* key, double& outValue, const EmJsonInnerObj& scope = EmJsonInnerObj()) const;
    bool getEpoch(const char* key, uint32_t& dest, const EmJsonInnerObj& scope = EmJsonInnerObj()) const;
    bool getTagValue(const char* key, EmTagValue& dest, const EmJsonInnerObj& scope = EmJsonInnerObj()) const;

    // Array handling methods
    bool getNestedArray(const char* parentKey, EmJsonInnerObj& outArray, const EmJsonInnerObj& parentScope = EmJsonInnerObj()) const {
        const char* valPtr = findValuePointer(parentKey, parentScope);
        if (!valPtr || *valPtr != '[') return false; 

        outArray.startIdx = valPtr - m_jsonStr; 
        
        int bracketCount = 0;
        for (size_t i = outArray.startIdx; i < m_jsonLen; i++) {
            if (m_jsonStr[i] == '[') bracketCount++;
            else if (m_jsonStr[i] == ']') bracketCount--;

            if (bracketCount == 0) {
                outArray.endIdx = i; 
                return true;
            }
        }
        return false; 
    }

    size_t getIntArray(const EmJsonInnerObj& arrayScope, int32_t* destArray, size_t maxSize) const {
        if (!arrayScope.isValid() || !destArray || maxSize == 0) return 0;

        size_t count = 0;
        forEachNumber<int32_t>(arrayScope, [destArray, maxSize, &count](int32_t value) {
            if (count < maxSize) {
                destArray[count++] = value;
            }
        });
        return count; // Returns the total number of items actually extracted
    }

    // 2. Extract JSON array into a C float/double array
    size_t getFloatArray(const EmJsonInnerObj& arrayScope, float* destArray, size_t maxSize) const {
        if (!arrayScope.isValid() || !destArray || maxSize == 0) return 0;

        size_t count = 0;
        forEachNumber<float>(arrayScope, [destArray, maxSize, &count](float value) {
            if (count < maxSize) {
                destArray[count++] = value;
            }
        });
        return count;
    }

    // 3. Extract JSON array into a C boolean array
    size_t getBoolArray(const EmJsonInnerObj& arrayScope, bool* destArray, size_t maxSize) const {
        if (!arrayScope.isValid() || !destArray || maxSize == 0) return 0;

        size_t count = 0;
        forEachBool(arrayScope, [destArray, maxSize, &count](bool value) {
            if (count < maxSize) {
                destArray[count++] = value;
            }
        });
        return count;
    }

    // 4. Extract JSON array of strings into a 2D C character array (Array of strings)
    // Example: char myStrings[5][16]; -> maxSize = 5, strMaxLen = 16
    size_t getStringArray(const EmJsonInnerObj& arrayScope, char* destArray, size_t maxSize, size_t strMaxLen) const {
        if (!arrayScope.isValid() || !destArray || maxSize == 0 || strMaxLen == 0) return 0;

        size_t count = 0;
        forEachString(arrayScope, [destArray, maxSize, strMaxLen, &count](const char* strValue) {
            if (count < maxSize) {
                // Calculate pointer location for current row: destArray[count]
                char* targetRow = destArray + (count * strMaxLen);
                
                strncpy(targetRow, strValue, strMaxLen - 1);
                targetRow[strMaxLen - 1] = '\0'; // Guarantee null-termination
                count++;
            }
        });
        return count;
    }

    template<typename F>
    void forEachString(const EmJsonInnerObj& arrayScope, F callback) const {
        if (!arrayScope.isValid()) return;

        // Start scanning just past the opening bracket '['
        size_t currentIdx = arrayScope.startIdx + 1;
        char itemBuffer[64]; // Fixed stack buffer for parsing each individual string safely

        while (currentIdx < arrayScope.endIdx) {
            // Find the next opening double quote
            const char* startQuote = strchr(m_jsonStr + currentIdx, '"');
            if (!startQuote || startQuote >= (m_jsonStr + arrayScope.endIdx)) break;

            startQuote++; // Skip the quote character itself
            const char* endQuote = strchr(startQuote, '"');
            if (!endQuote || endQuote >= (m_jsonStr + arrayScope.endIdx)) break;

            // Extract the isolated element into our stack buffer safely
            size_t valLen = endQuote - startQuote;
            if (valLen >= sizeof(itemBuffer)) valLen = sizeof(itemBuffer) - 1;
            
            strncpy(itemBuffer, startQuote, valLen);
            itemBuffer[valLen] = '\0';

            // Fire the user's callback instantly with the parsed string
            callback(itemBuffer);

            // Move past the closing quote and look for the next item
            currentIdx = (endQuote - m_jsonStr) + 1;
        }
    }

    template<typename T, typename F>
    void forEachNumber(const EmJsonInnerObj& arrayScope, F callback) const {
        if (!arrayScope.isValid()) return;

        const char* scanPtr = m_jsonStr + arrayScope.startIdx + 1;
        const char* endPtr = m_jsonStr + arrayScope.endIdx;

        while (scanPtr < endPtr) {
            // Skip characters like whitespace, brackets, and commas to reach the next digit
            size_t spaces = strspn(scanPtr, " \t\r\n,[]");
            scanPtr += spaces;
            if (scanPtr >= endPtr || *scanPtr == '\0') break;

            // Parse the number inline based on the template type
            char* nextToken = nullptr;
            if constexpr (std::is_same_v<T, float> || std::is_same_v<T, double>) {
                double val = strtod(scanPtr, &nextToken);
                if (nextToken == scanPtr) break; // Parse error
                callback(static_cast<T>(val));
            } else if constexpr (std::is_unsigned_v<T>) {
                unsigned long val = strtoul(scanPtr, &nextToken, 10);
                if (nextToken == scanPtr) break;
                callback(static_cast<T>(val));
            } else {
                long val = strtol(scanPtr, &nextToken, 10);
                if (nextToken == scanPtr) break;
                callback(static_cast<T>(val));
            }

            scanPtr = nextToken; // Advance past the processed numeric string digits
        }
    }

    template<typename F>
    void forEachBool(const EmJsonInnerObj& arrayScope, F callback) const {
        if (!arrayScope.isValid()) return;

        const char* scanPtr = m_jsonStr + arrayScope.startIdx + 1;
        const char* endPtr = m_jsonStr + arrayScope.endIdx;

        while (scanPtr < endPtr) {
            // Skip whitespaces, brackets, and separators to find the next alphanumeric character
            size_t spaces = strspn(scanPtr, " \t\r\n,[]");
            scanPtr += spaces;
            if (scanPtr >= endPtr || *scanPtr == '\0') break;

            // Inspect the prefix tokens directly
            if (strncmp(scanPtr, "true", 4) == 0) {
                callback(true);
                scanPtr += 4; // Advance past the string token 'true'
            } else if (strncmp(scanPtr, "false", 5) == 0) {
                callback(false);
                scanPtr += 5; // Advance past the string token 'false'
            } else {
                // If it encounters an unrecognizable structure token, step forward 1 byte to prevent infinite lock loops
                scanPtr++;
            }
        }
    }

private:
    const char* findValuePointer(const char* key, const EmJsonInnerObj& scope) const;

    // Member vars
    const char* m_jsonStr;
    size_t m_jsonLen;
};

#endif // __EM_JSON_READER_H