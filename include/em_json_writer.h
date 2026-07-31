#ifndef __EM_JSON_WRITER_H
#define __EM_JSON_WRITER_H

#include <type_traits>

#include "em_string.h"
#include "em_tag.h"

// A tiny JSON writer that can append key-value pairs to a string buffer without using ArduinoJson or any other library.
// This is not perfect and nor complete Json handling class, but it is sufficient for our needs to append values to a JSON string without allocating memory on the heap.
//
// Example usage:
// -------------.
//     EmStringL jsonBuffer;
//     EmJsonDictWriter writer(jsonBuffer);
//     writer.add("key1", "value1");
//     writer.add("key2", 42);      
//     writer.beginObject("nested");
//     writer.add("nestedKey", true);
//     writer.endObject();  
//     writer.end(); // Close the root object
//
class EmJsonDictWriter {
public:
    // Bind to your custom string reference
    EmJsonDictWriter(EmStringBase& target) : m_target(target) {
        m_target.append("{"); // Start the root object
        m_level = 1;
    }

    ~EmJsonDictWriter() {
        end();
    }

    // Closes the object JSON composition by closing all open nested objects in case.
    // Notes:
    //  - this will not close the root object, so you should call end() at the end of your JSON composition.
    //  - this will also closed nested open objects consdering tha those are dictionaries (i.e. enclosed by '{' and '}')
    void end() {
        while (m_level--) {
            m_target.append("}");
        }
    }

    // Append String Values
    bool addString(const char* key, const char* value) {
        if (m_level == 0) {
            return false;
        }
        prefix();
        return m_target.appendFormat(false, "\"%s\":\"%s\"", key, value ? value : "") == EmStrResult::success;
    }

    // Append Integer Values
    template<typename T>
    bool addInt(const char* key, T value) {
        static_assert(std::is_integral_v<T>, "An integer type is requested for this function.");        
        if (m_level == 0) {
            return false;
        }
        prefix();
        if constexpr (std::is_signed_v<T>) {
            return m_target.appendFormat(false, "\"%s\":%jd", key, static_cast<intmax_t>(value)) == EmStrResult::success;
        } else {
            return m_target.appendFormat(false, "\"%s\":%ju", key, static_cast<uintmax_t>(value)) == EmStrResult::success;
        }
    }

    // Append real Values
    template<typename T>
    bool addReal(const char* key, T value) {
        static_assert(std::is_floating_point_v<T>, "An real type is requested for this function.");        
        if (m_level == 0) {
            return false;
        }
        prefix();
        return m_target.appendFormat(false, "\"%s\":%g", key, value) == EmStrResult::success;
    }

    // Append Boolean Values
    bool addBool(const char* key, bool value) {
        if (m_level == 0) {
            return false;
        }
        prefix();
        return m_target.appendFormat(false, "\"%s\":%s", key, value ? "true" : "false") == EmStrResult::success;
    }

    // Append Tag Value
    bool addTag(const char* key, const EmTagValue& value) {
        switch (value.getType()) {
            case EmTagValueType::vt_boolean:
                return addBool(key, value.asBool());
            case EmTagValueType::vt_integer:
                return addInt(key, value.asInteger());
            case EmTagValueType::vt_real:
                return addReal(key, value.asReal());
            case EmTagValueType::vt_string:
                return addString(key, value.asString());
            case EmTagValueType::vt_epoch:
                return addTimestamp(key, value.asEpoch());
            case EmTagValueType::vt_undefined:
                return addNull(key);
        }
        return false;
    }

    // Append a timestamp epoch value
    bool addTimestamp(const char* key, EmEpochType timestamp) {
        if (m_level == 0) {
            return false;
        }
        EmStringS dt;
        if (!dt.toTimestamp(timestamp)) {
            return false;
        }
        prefix();
        return m_target.appendFormat(false, "\"%s\":\"%s\"", key, dt.c_str()) == EmStrResult::success;
    }


    // Append a null value
    bool addNull(const char* key) {
        if (m_level == 0) {
            return false;
        }
        prefix();
        return m_target.appendFormat(false, "\"%s\":null", key) == EmStrResult::success;
    }

    // Open a nested object dictionary (default) or array (e.g., "params": { )
    // If 'key' is nullptr or empty, it will just open a new object without a key (useful for arrays)
    bool beginObject(const char* key, char openChar = '{') {
        if (m_level == 0) {
            return false;
        }
        prefix();
        if (key == nullptr || strlen(key) == 0) {
            m_target.appendFormat(false, "%c", openChar);
        } else {
            m_target.appendFormat(false, "\"%s\":%c", key, openChar);
        }
        m_level++;
        return true;
    }

    // Close a nested object manually (appends the closing '}' or ']' character)
    void endObject(char closeChar = '}') {
        if (m_level < 1) {
            return;
        }
        m_target.appendFormat(false, "%c", closeChar);
        m_level--;
        m_isFirstElement = false; // The nested object itself is now complete
    }

    // Adds a generic object content (e.g.dict or list)
    // This method is used in case you already have a well formattes dict or list string.
    bool addObject(const char* key, const char* content) {
        if (m_level == 0) {
            return false;
        }
        prefix();
        return m_target.appendFormat(false, "\"%s\":%s", key, content) == EmStrResult::success;    
    }

    // Lists handling
    template<typename... Args>
    void addIntList(const char* key, Args... args) {
        prefix(); 
        m_target.appendFormat(false, "\"%s\":[", key);
        appendListItems(args...);
        m_target.append("]");
    }

    template<typename... Args>
    void addFloatList(const char* key, Args... args) {
        prefix(); m_target.appendFormat(false, "\"%s\":[", key);
        appendListItems_(args...);
        m_target.append("]");
    }

    template<typename... Args>
    void addStringList(const char* key, Args... args) {
        prefix(); m_target.appendFormat(false, "\"%s\":[", key);
        appendListItems_(args...);
        m_target.append("]");
    }

    template<typename... Args>
    void addBoolList(const char* key, Args... args) {
        prefix(); 
        m_target.appendFormat(false, "\"%s\":[", key);
        appendListItems_(args...);
        m_target.append("]");
    }
private:

    // Base case: exactly one item remaining (no trailing comma needed)
    void appendListItem_(int32_t item)      { m_target.appendFormat(false, "%d", item); }
    void appendListItem_(uint32_t item)     { m_target.appendFormat(false, "%u", item); }
    void appendListItem_(double item)       { m_target.appendFormat(false, "%.4f", item); }
    void appendListItem_(const char* item)  { m_target.appendFormat(false, "\"%s\"", item ? item : ""); }
    void appendListItem_(bool item)         { m_target.appendFormat(false, "%s", item ? "true" : "false"); }

    // Fallback base case for completely empty lists e.g. addIntList("empty")
    void appendListItems_() {} 

    // Single item left base case
    template<typename T>
    void appendListItems_(T head) {
        appendListItem_(head);
    }

    // Multiple items remaining: print head, add comma separator, recurse tail
    template<typename T, typename... Args>
    void appendListItems_(T head, Args... tail) {
        appendListItem(head);
        m_target.append(",");
        appendListItems_(tail...); // Tail recursion unrolling
    }

    // Helper to handle commas between key-value pairs automatically
    void prefix() {
        if (!m_isFirstElement) {
            m_target.append(",");
        }
        m_isFirstElement = false;
    }

    EmStringBase& m_target;
    bool m_isFirstElement = true;
    uint16_t m_level = 0;
};

#endif // __EM_JSON_WRITER_H