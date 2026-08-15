#ifndef __EM_STRING_MAP_H__
#define __EM_STRING_MAP_H__
#include <map>
#include <cstddef> // For size_t

#include "em_string.h"

#include <map>
#include <cstddef>
#include <type_traits>

// A tiny string map class that stores hash instead of the string itself to safe memory.
template <typename T>
class EmStringMap {
public:
    // Destructor: Only executes cleanup code if T is a pointer type
    ~EmStringMap() {
        clear();
    }

    // Rule of Five: Delete copy/move to prevent double-free bugs when using pointers
    EmStringMap(const EmStringMap&) = delete;
    EmStringMap& operator=(const EmStringMap&) = delete;
    EmStringMap(EmStringMap&&) = delete;
    EmStringMap& operator=(EmStringMap&&) = delete;

    EmStringMap() = default;

    void set(const EmStringBase& key, const T& value) {
        set(key.c_str(), value);
    }

    void set(const char* key, const T& value) {
        size_t hash = calculateHash(key);
        
        // Compile-time check: If T is a pointer, safely delete the old object before overwriting
        if constexpr (std::is_pointer_v<T>) {
            auto it = m_map.find(hash);
            if (it != m_map.end() && it->second != value) {
                delete it->second;
            }
        }
        
        m_map[hash] = value;
    }

    void get(const EmStringBase& key, T& outValue) const {
        get(key.c_str(), outValue);
    }

    bool get(const char* key, T& outValue) const {
        size_t hash = calculateHash(key);
        auto it = m_map.find(hash);
        if (it != m_map.end()) {
            outValue = it->second;
            return true;
        }
        return false;
    }

    T getOrDefault(const EmStringBase& key, const T& defaultValue) const {
        return getOrDefault(key.c_str(), defaultValue);
    }

    T getOrDefault(const char* key, const T& defaultValue) const {
        size_t hash = calculateHash(key);
        auto it = m_map.find(hash);
        if (it != m_map.end()) {
            return it->second;
        }
        return defaultValue;
    }

    T& operator[](const EmStringBase& key) {
        return operator[](key.c_str());
    }

    T& operator[](const char* key) {
        return m_map[calculateHash(key)];
    }

    T operator[](const EmStringBase& key) const {
        return operator[](key.c_str());
    }

    T operator[](const char* key) const {
        size_t hash = calculateHash(key);
        auto it = m_map.find(hash);
        if (it != m_map.end()) {
            return it->second;
        }
        return T(); // Returns 0/false for primitives, nullptr for pointers
    }

    bool contains(const EmStringBase& key) const {
        return contains(key.c_str());
    }

    bool contains(const char* key) const {
        return m_map.find(calculateHash(key)) != m_map.end();
    }

    void clear() {
        // Compile-time check: Loop and delete objects only if T is a pointer type
        if constexpr (std::is_pointer_v<T>) {
            for (auto& pair : m_map) {
                delete pair.second;
            }
        }
        m_map.clear();
    }

private:
    std::map<size_t, T> m_map;
};


#endif // __EM_STRING_MAP_H__