#ifndef _EM_TAG_VALUE_H__
#define _EM_TAG_VALUE_H__

#include "em_defs.h"

#ifdef EM_STD_LIB // Need standard library 

#include <type_traits>

#include "em_list.h"
#include "em_value.h"
#include "em_epoch.h"
#include "em_string.h"
#include "em_threading.h"
#include "em_value_sync.h"
#include "em_sbo_buffer.h"

// The tag value type
enum class EmTagValueType: uint8_t {
    vt_undefined = 0,
    vt_bool      = 1,
    vt_int       = 2,
    vt_uint      = 3,
    vt_real      = 4,
    vt_epoch     = 5,
    vt_string    = 6,
    // Max enum value
    _vt_MAX      = 6
};

inline bool isValidTagValueType(uint8_t type) {
    return type <= static_cast<uint8_t>(EmTagValueType::_vt_MAX);
}

// The value types based on CPU architecture (i.e. 32 or 64 bit) 
constexpr bool is_64bit = (sizeof(void*) == 8);
using EmBoolType  = bool;
using EmIntType   = typename std::conditional<is_64bit, int64_t, int32_t>::type;
using EmUIntType  = typename std::conditional<is_64bit, uint64_t, uint32_t>::type;
using EmRealType  = typename std::conditional<is_64bit, double, float>::type;
using EmEpochType = typename std::conditional<is_64bit, EmEpoch64, EmEpoch32>::type;

// Forward declaration
class EmTagValueBuffer;

// Tag value is thread safe in multithreaded capable environments.
#ifdef EM_MULTITHREAD
    #define MUTEX_LOCK EmMutexLock lock(m_mutex)
#else
    #define MUTEX_LOCK
#endif

// The tag value class.
//
// This class is used to have a concrete implementation of value since 
// 'EmTag' and 'EmTags' classes will not support templates.
class EmTagValue: EmValue<EmTagValue> {
    friend class EmTagValueBuffer;
public: 
    EmTagValue()
     : m_type(EmTagValueType::vt_undefined), m_value() {}

    explicit EmTagValue(EmBoolType value)
     : m_type(EmTagValueType::vt_bool), m_value(value) {} 

    explicit EmTagValue(EmEpochType value)
     : m_type(EmTagValueType::vt_epoch), m_value(value) {}

    explicit EmTagValue(EmStringBase& value)
     : m_type(EmTagValueType::vt_string), m_value(value) {}

    template<typename T>
    EmTagValue(const T& value) {
        using cleanType = std::decay_t<T>;
        if constexpr (std::is_integral_v<T>) {
            if constexpr (std::is_signed_v<T>) {
                set_(EmTagValueType::vt_int, static_cast<EmIntType>(value));
            } else {
                set_(EmTagValueType::vt_uint, static_cast<EmUIntType>(value));                
            }
        }
        else if constexpr (std::is_enum_v<cleanType>) {
            set_(EmTagValueType::vt_int, static_cast<EmIntType>(value));
        }
        else if constexpr (std::is_same_v<cleanType, float>) {
            set_(EmTagValueType::vt_real, static_cast<EmRealType>(value));
        }
        else if constexpr (std::is_same_v<cleanType, double>) {
            set_(EmTagValueType::vt_real, static_cast<EmRealType>(value));
        }
        else if constexpr (std::is_same_v<cleanType, char*> || std::is_same_v<cleanType, const char*>) {
            static_assert(always_false<cleanType>, "Cannot initialize a string with a char*. Please use EmString object!");
            
        } else {
            static_assert(always_false<cleanType>, "Unsupported value type!");
        }    
    }

    // Simple copy constructor
    EmTagValue(const EmTagValue& other) {
        setValue(other);
    }

    // Move constructor same as copy, non need to free anything!
    EmTagValue(EmTagValue&& other) {
        setValue(other);
    }

    // Simple copy operator
    EmTagValue& operator=(const EmTagValue& other) {
        setValue(other);
        return *this;
    }

    // Move operator same as copy, non need to free anything!
    EmTagValue& operator=(EmTagValue&& other) {
        setValue(other);
        return *this;
    }

    EmTagValueType getType() const { 
        MUTEX_LOCK;
        return m_type; 
    }

    bool isSameType(const EmTagValue& other) const {
        MUTEX_LOCK;
        return m_type == other.m_type;
    }

    bool isSameType(EmTagValueType type) const {
        MUTEX_LOCK;
        return m_type == type;
    }

    bool isUndefinedType () const { 
        MUTEX_LOCK;
        return m_type == EmTagValueType::vt_undefined; 
    }

    bool isNotUndefinedType () const { 
        return !isUndefinedType();
    }

    void setUndefinedType() {
        MUTEX_LOCK;
        clear_();
    }

    void clear() {
        MUTEX_LOCK;
        clear_();
    }

    EmBoolType asBool() const {
        MUTEX_LOCK;
        return m_value.as_bool;
    }

    EmIntType asInt() const {
        MUTEX_LOCK;
        return m_value.as_int;
    }

    EmUIntType asUInt() const {
        MUTEX_LOCK;
        return m_value.as_uint;
    }

    EmRealType asReal() const {
        MUTEX_LOCK;
        return m_value.as_real;
    }

    EmEpochType asEpoch() const {
        MUTEX_LOCK;
        return m_value.as_epoch;
    }

    // Returns null if the value is not a string or if the string is empty.
    const char* asString() const {
        MUTEX_LOCK;
        if (m_type != EmTagValueType::vt_string || m_value.as_string == nullptr) {
            return nullptr;
        }
        return m_value.as_string->c_str();
    }

    template<typename T>
    T as() const {
        using cleanType = std::decay_t<T>;
        if constexpr (std::is_same_v<cleanType, EmBoolType>) {
            return asBool();
        }
        else if constexpr (std::is_enum_v<cleanType>) {
            return static_cast<T>(asInt());
        }
        else if constexpr (std::is_integral_v<T>) {
            if constexpr (std::is_signed_v<T>) {
                return asInt();
            } else {
                return asUInt();
            }
        }
        else if constexpr (std::is_same_v<cleanType, float> || std::is_same_v<cleanType, double>) {
            return asReal();
        }
        else if constexpr (std::is_same_v<cleanType, EmEpoch32> || std::is_same_v<cleanType, EmEpoch64>) {
            return asEpoch();
        }
        else if constexpr (std::is_same_v<cleanType, char*> || std::is_same_v<cleanType, const char*> ||
                           std::is_same_v<cleanType, EmStringBase>) {
            return asString();            
        } else {
            static_assert(always_false<cleanType>, "Unsupported value type!");
        }  
        return T();
    }

    // EmValue implementation
    template<typename T>
    EmGetValueResult getValue(T& value) const {
        if (*this == value) {
            return EmGetValueResult::succeedEqualValue;
        }
        value = as<T>();
        return EmGetValueResult::succeedNotEqualValue;
    }

    EmGetValueResult getValue(EmStringBase& value) const {
        MUTEX_LOCK;
        if (m_type != EmTagValueType::vt_string) {
            return EmGetValueResult::failed;
        }
        EmGetValueResult res = (value == *m_value.as_string)
                               ? EmGetValueResult::succeedEqualValue 
                               : EmGetValueResult::succeedNotEqualValue;
        value.set(*m_value.as_string);
        return res;
    }

    EmGetValueResult getValue(EmTagValue& value) const {
        // Is already equal
        if (*this == value) {
            return EmGetValueResult::succeedEqualValue; 
        }
        // Compatible type?
        if (!isSameType(value) && value.isNotUndefinedType()) {
            return EmGetValueResult::failed;
        }
        // Set new value
        value.setValue(*this);
        return EmGetValueResult::succeedNotEqualValue;        
    }
    
    template<class T>
    void set(EmTagValueType type, T value) { 
        MUTEX_LOCK;
        set_(type, value);
    }
    
    // EmValue implementation
    bool setValue(const EmTagValue& value) {
        MUTEX_LOCK;
        m_type = value.m_type;
        m_value = value.m_value;
        return true;
    }

    // Sets a value of any type. If forceType is true, the value type will be forced to   
    // the new type, otherwise it will only be set if the type is same type or undefined.
    // NOTE: setting a string (i.e. value being char*) will ignore the forceType 
    //       parameter since a tag value string should be reference a EmString object.
    template<typename T>
    bool setValue(T value, bool forceType) {
        MUTEX_LOCK;
        using cleanType = std::decay_t<T>;
        if constexpr (std::is_same_v<cleanType, EmBoolType>) {
            if (!forceType && m_type != EmTagValueType::vt_bool && m_type != EmTagValueType::vt_undefined) {
                return false;
            }
            set_(EmTagValueType::vt_bool, value);
        }
        else if constexpr (std::is_integral_v<T>) {
            if (!forceType && m_type != EmTagValueType::vt_int && m_type != EmTagValueType::vt_undefined) {
                return false;
            }
            if constexpr (std::is_signed_v<T>) {
                set_(EmTagValueType::vt_int, static_cast<EmUIntType>(value));
            } else {
                set_(EmTagValueType::vt_uint, static_cast<EmUIntType>(value));                
            }
        }
        else if constexpr (std::is_enum_v<cleanType>) {
            if (!forceType && m_type != EmTagValueType::vt_int && m_type != EmTagValueType::vt_undefined) {
                return false;
            }
            set_(EmTagValueType::vt_int, static_cast<EmIntType>(value));
        }
        else if constexpr (std::is_same_v<cleanType, float> || std::is_same_v<cleanType, double>) {
            if (!forceType && m_type != EmTagValueType::vt_int && m_type != EmTagValueType::vt_undefined) {
                return false;
            }
            set_(EmTagValueType::vt_real, static_cast<EmRealType>(value));
        }            
        else if constexpr (std::is_same_v<cleanType, char*> || std::is_same_v<cleanType, const char*>) {
            // NOTE: this will IGNORE the forceType flag, since it is not possible to force a string type from a char*.
            return setString_(value);
        } else {
            static_assert(always_false<cleanType>, "Unsupported value type!");
        }  
        return true;  
    }

    // String value cannot be forced since it holds a pointer to the string object.
    bool setString(const EmStringBase& value) {
        return setString_(value.c_str());
    }

    bool setString(const char* value) {  
        MUTEX_LOCK;
        return setString_(value);
    }

    template<typename T>
    bool setEpoch(const EmEpoch<T>& value, bool forceType) {
        MUTEX_LOCK;
        if (!forceType && m_type != EmTagValueType::vt_epoch && m_type != EmTagValueType::vt_undefined) {
            return false;
        }
        set_(EmTagValueType::vt_epoch, value);
        return true;
    }

    bool operator==(const EmTagValue& other) const {
        MUTEX_LOCK;
        // Same type?
        if (m_type != other.m_type) {
            return false;
        }
        // Same value?
        switch (m_type) {
            case EmTagValueType::vt_undefined: return false; 
            case EmTagValueType::vt_bool:      return this->m_value.as_bool == other.m_value.as_bool;
            case EmTagValueType::vt_int:       return this->m_value.as_int == other.m_value.as_int;
            case EmTagValueType::vt_uint:      return this->m_value.as_uint == other.m_value.as_uint;
            case EmTagValueType::vt_real:      return this->m_value.as_real == other.m_value.as_real;
            case EmTagValueType::vt_epoch:     return this->m_value.as_epoch == other.m_value.as_epoch; // Richiede operator== su EmEpoch64/32
            case EmTagValueType::vt_string: 
                if (!m_value.as_string || !other.m_value.as_string) {
                    return false;
                } 
                return *(m_value.as_string) == *(other.m_value.as_string);
        }
        return false;
    }

    bool operator !=(const EmTagValue& other) const {
        return !(*this == other);
    }

    bool operator >(const EmTagValue& other) const {
        MUTEX_LOCK;
        // Same type?
        if (m_type != other.m_type) {
            return false;
        }
        // Same value?
        switch (m_type) {
            case EmTagValueType::vt_undefined: return false; 
            case EmTagValueType::vt_bool:      return m_value.as_bool > other.m_value.as_bool;
            case EmTagValueType::vt_int:       return m_value.as_int > other.m_value.as_int;
            case EmTagValueType::vt_uint:      return m_value.as_uint > other.m_value.as_uint;
            case EmTagValueType::vt_real:      return m_value.as_real > other.m_value.as_real;
            case EmTagValueType::vt_epoch:     return m_value.as_epoch > other.m_value.as_epoch; // Richiede operator== su EmEpoch64/32
            case EmTagValueType::vt_string: 
                if (!m_value.as_string || !other.m_value.as_string) {
                    return false;
                } 
                return *(m_value.as_string) > *(other.m_value.as_string);
        }
        return false;
    }

    bool operator >=(const EmTagValue& other) const {
        // Same type?
        if (m_type != other.m_type) {
            return false;
        }
        return (*this > other) || (*this == other);
    }

    bool operator <=(const EmTagValue& other) const {
        // Same type?
        if (m_type != other.m_type) {
            return false;
        }
        return !(*this > other);
    }

    bool operator <(const EmTagValue& other) const {
        // Same type?
        if (m_type != other.m_type) {
            return false;
        }
        return !(*this >= other);
    }

protected:
    // Type traits helper
    template<typename...> static constexpr bool always_false = false;

    // The tag value union is used to store the actual value of the tag. 
    union EmTagValueUnion {
        EmBoolType    as_bool = false;
        EmIntType     as_int;
        EmUIntType    as_uint;
        EmRealType    as_real;
        EmEpochType   as_epoch;
        EmStringBase* as_string;  // Storing pointers to keep size of union at 32/64 bit.

        EmTagValueUnion() = default;
        explicit EmTagValueUnion(EmBoolType value) { as_bool = value; }
        explicit EmTagValueUnion(EmEpochType value) { as_epoch = value; }
        explicit EmTagValueUnion(EmIntType value) { as_int = value; }
        explicit EmTagValueUnion(EmUIntType value) { as_uint = value; }
        explicit EmTagValueUnion(float value) { as_real = value; }
        explicit EmTagValueUnion(double value) { as_real = value; }
        explicit EmTagValueUnion(EmStringBase& value) { as_string = &value; }
    };

    void clear_() {
        m_type = EmTagValueType::vt_undefined;
        m_value = {}; // Zero out the union
    }
    
    template<class T>
    void set_(EmTagValueType type, T value) { 
        m_type = type;
        m_value = EmTagValueUnion(value); 
    }

    void set_(EmTagValueType type, const EmTagValueUnion& value) { 
        m_type = type;
        m_value = value; 
    }

    void get_(EmTagValueType& type, EmTagValueUnion& value) { 
        type = m_type;
        value = m_value; 
    }

     size_t getValueBufferSize_() const {
        MUTEX_LOCK;
        switch (m_type) {
            case EmTagValueType::vt_string:
                return m_value.as_string ? m_value.as_string->length() + 1 : 0;
            default:
                return sizeof(m_value);
        }
    }

    const void* getValueBuffer_() const {
        MUTEX_LOCK;
        switch (m_type) {
            case EmTagValueType::vt_string:
                return m_value.as_string ? m_value.as_string->c_str() : nullptr;
            default:
                return &m_value;
        }
    }

    bool setString_(const char* value) {  
        if (m_type == EmTagValueType::vt_string) {
            // Just reassign the value.
            m_value.as_string->set(value);
            return true;
        }
        // If the type is undefined or not a string, we can create a new string object.
        return false;
    }

    // Membed vars
    EmTagValueType m_type;
    EmTagValueUnion m_value;
#ifdef EM_MULTITHREAD
    mutable EmMutex m_mutex;
#endif
};


// The tag value buffer class is used to read and write a tag value in a memory buffer.
// It uses a small buffer optimization to avoid heap fragmentation when reading/writing tags from/to storage.
class EmTagValueBuffer: public EmSboBuffer<char, 128> {
public:
    EmTagValueBuffer(const EmTagValue& tagValue) {
        fromValue(tagValue);
    }

    EmTagValueBuffer(size_t bufMaxSize)
     : EmSboBuffer(bufMaxSize) {}

    ~EmTagValueBuffer() {
        clear();
    }

    void clear() {
        EmSboBuffer::clear();
        m_size = 0;
    }

    void fromValue(const EmTagValue& tagValue) {
        m_size = tagValue.getValueBufferSize_();
        setMaxSize(m_size+1);
        char* buf = getBuffer();
        if (buf) {
            buf[0] = static_cast<char>(tagValue.getType());
            if (m_size > 0) {
                memcpy(&buf[1], tagValue.getValueBuffer_(), m_size);
            }
        }
    }   

    bool toValue(EmTagValue& tagValue) {
        // Buffer stores a value?
        if (getSize() == 0) {
            return false;
        }
        char* buf = getBuffer();
        if (buf) {
            // Read type
            if (!isValidTagValueType(buf[0])) {
                return false;
            }
            EmTagValueType type = static_cast<EmTagValueType>(buf[0]);
            // Read value
            if (type == EmTagValueType::vt_string) {
                // For string type, the value is a null-terminated string stored in the buffer
                const char* strValue = &buf[1];
                tagValue.setString(strValue);
            } else {
                // For other types, the value is stored as binary data in the buffer
                tagValue.set_(type, *reinterpret_cast<const EmTagValue::EmTagValueUnion*>(&buf[1]));
            }
            return true;
        }
        return false;
    }

    size_t getSize() const {
        return m_size;
    }

private:
    size_t m_size = 0;
};

#endif // EM_STD_LIB
#endif // _EM_TAG_VALUE_H__