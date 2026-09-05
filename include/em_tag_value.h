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
    // Max enum value
    _vt_MAX      = 5
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

// The numeric tag value class.
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

    explicit EmTagValue(EmIntType value)
     : m_type(EmTagValueType::vt_int), m_value(value) {}

    explicit EmTagValue(EmUIntType value)
     : m_type(EmTagValueType::vt_uint), m_value(value) {}

    explicit EmTagValue(int value)
     : m_type(EmTagValueType::vt_int), m_value(static_cast<EmIntType>(value)) {}

    explicit EmTagValue(unsigned int value)
     : m_type(EmTagValueType::vt_uint), m_value(static_cast<EmUIntType>(value)) {}

    explicit EmTagValue(float value)
     : m_type(EmTagValueType::vt_real), m_value(static_cast<EmRealType>(value)) {}

    explicit EmTagValue(double value)
     : m_type(EmTagValueType::vt_real), m_value(static_cast<EmRealType>(value)) {}


    EmTagValue(const EmTagValue& other) {
        m_type = other.m_type;
        m_value = other.m_value; // TODO: can we have this? Not that other goes out of scope sooner than this!?
    };

    EmTagValue& operator=(const EmTagValue& other) {
        if (this != &other) {
            m_type = other.m_type;
            m_value = other.m_value; // TODO: can we have this? Not that other goes out of scope sooner than this!?
        }
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

    bool isNotSameType(const EmTagValue& other) const {
        return !isSameType(other);
    }

    bool isNotSameType(EmTagValueType type) const {
        return !isSameType(type);
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

    EmGetValueResult getValue(EmTagValue& value) const {
        // Is already equal
        if (*this == value) {
            return EmGetValueResult::succeedEqualValue; 
        }
        // Compatible type?
        if (isNotSameType(value) && value.isNotUndefinedType()) {
            return EmGetValueResult::failed;
        }
        // Set new value
        if (value.setValue(*this)) {
            return EmGetValueResult::succeedNotEqualValue;
        }
        return EmGetValueResult::failed;
    }

    EmGetValueResult getValue(EmBoolType& value) const {
        MUTEX_LOCK;
        if (m_value.as_bool == value) {
            return EmGetValueResult::succeedEqualValue;
        }
        value = m_value.as_bool;
        return EmGetValueResult::succeedNotEqualValue;
    }

    EmGetValueResult getValue(EmIntType& value) const {
        MUTEX_LOCK;
        if (m_value.as_int == value) {
            return EmGetValueResult::succeedEqualValue;
        }
        value = m_value.as_int;
        return EmGetValueResult::succeedNotEqualValue;
    }

    EmGetValueResult getValue(EmUIntType& value) const {
        MUTEX_LOCK;
        if (m_value.as_uint == value) {
            return EmGetValueResult::succeedEqualValue;
        }
        value = m_value.as_uint;
        return EmGetValueResult::succeedNotEqualValue;
    }

    EmGetValueResult getValue(int& value) const {
        return getValue(reinterpret_cast<EmIntType&>(value));
    }

    EmGetValueResult getValue(unsigned int& value) const {
        return getValue(reinterpret_cast<EmUIntType&>(value));
    }

    EmGetValueResult getValue(float& value) const {
        MUTEX_LOCK;
        if (static_cast<float>(m_value.as_real) == value) {
            return EmGetValueResult::succeedEqualValue;
        }
        value = static_cast<float>(m_value.as_real);
        return EmGetValueResult::succeedNotEqualValue;
    }

    EmGetValueResult getValue(double& value) const {
        MUTEX_LOCK;
        if (static_cast<double>(m_value.as_real) == value) {
            return EmGetValueResult::succeedEqualValue;
        }
        value = static_cast<double>(m_value.as_real);
        return EmGetValueResult::succeedNotEqualValue;
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
    bool setValue(EmBoolType value, bool forceType) {
        MUTEX_LOCK;
        if (!forceType && m_type != EmTagValueType::vt_bool && m_type != EmTagValueType::vt_undefined) {
            return false;
        }
        m_type = EmTagValueType::vt_bool;
        m_value.as_bool = value;
        return true;  
    }

    bool setValue(EmIntType value, bool forceType) {
        MUTEX_LOCK;
        if (!forceType && m_type != EmTagValueType::vt_int && m_type != EmTagValueType::vt_undefined) {
            return false;
        }
        m_type = EmTagValueType::vt_int;
        m_value.as_int = value;
        return true;  
    }

    bool setValue(EmUIntType value, bool forceType) {
        MUTEX_LOCK;
        if (!forceType && m_type != EmTagValueType::vt_uint && m_type != EmTagValueType::vt_undefined) {
            return false;
        }
        m_type = EmTagValueType::vt_uint;
        m_value.as_uint = value;
        return true;  
    }

    bool setValue(int value, bool forceType) {
        return setValue(static_cast<EmIntType>(value), forceType);
    }

    bool setValue(unsigned int value, bool forceType) {
        return setValue(static_cast<EmUIntType>(value), forceType); 
    }

    bool setValue(float value, bool forceType) {
        MUTEX_LOCK;
        if (!forceType && m_type != EmTagValueType::vt_real && m_type != EmTagValueType::vt_undefined) {
            return false;
        }
        m_type = EmTagValueType::vt_real;
        m_value.as_real = static_cast<EmRealType>(value);
        return true;  
    }

    bool setValue(double value, bool forceType) {
        MUTEX_LOCK;
        if (!forceType && m_type != EmTagValueType::vt_real && m_type != EmTagValueType::vt_undefined) {
            return false;
        }
        m_type = EmTagValueType::vt_real;
        m_value.as_real = static_cast<EmRealType>(value);
        return true;  
    }

    // Epoch value
    bool setEpoch(const EmEpochType& value, bool forceType) {
        MUTEX_LOCK;
        if (!forceType && m_type != EmTagValueType::vt_epoch && m_type != EmTagValueType::vt_undefined) {
            return false;
        }
        m_type = EmTagValueType::vt_epoch;
        m_value.as_epoch = value;
        return true;
    }

    // Comparison operators
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
        }
        return false;
    }

    bool operator >=(const EmTagValue& other) const {
        // Same type?
        if (isNotSameType(other)) {
            return false;
        }
        return (*this > other) || (*this == other);
    }

    bool operator <=(const EmTagValue& other) const {
        // Same type?
        if (isNotSameType(other)) {
            return false;
        }
        return !(*this > other);
    }

    bool operator <(const EmTagValue& other) const {
        // Same type?
        if (isNotSameType(other)) {
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

        EmTagValueUnion() = default;
        explicit EmTagValueUnion(EmBoolType value) { as_bool = value; }
        explicit EmTagValueUnion(EmEpochType value) { as_epoch = value; }
        explicit EmTagValueUnion(EmIntType value) { as_int = value; }
        explicit EmTagValueUnion(EmUIntType value) { as_uint = value; }
        explicit EmTagValueUnion(float value) { as_real = value; }
        explicit EmTagValueUnion(double value) { as_real = value; }
    };

    void clear_() {
        m_type = EmTagValueType::vt_undefined;
        m_value = {}; // Zero out the union
    }
    
    void set_(EmTagValueType type, const EmTagValueUnion& value) { 
        m_type = type;
        m_value = value; 
    }

    void setValue_(EmTagValueType type, const EmTagValueUnion& value) { 
        MUTEX_LOCK;
        set_(type, value);
    }

    size_t getValueBufferSize_() const {
        return sizeof(m_value);
    }

    const void* getValueBuffer_() const {
        MUTEX_LOCK;
        return &m_value;
    }

    // Membed vars
    EmTagValueType m_type;
    EmTagValueUnion m_value;
#ifdef EM_MULTITHREAD
    mutable EmMutex m_mutex;
#endif
};


// The tag value buffer class is used to read and write a tag value in a memory buffer.
class EmTagValueBuffer {
public:
    EmTagValueBuffer() {
        clear();
    }

    EmTagValueBuffer(const EmTagValue& tagValue) {
        fromValue(tagValue);
    }

    ~EmTagValueBuffer() {
        clear();
    }

    void clear() {
        memset(m_buf, 0, sizeof(m_buf));
    }

    void fromValue(const EmTagValue& tagValue) {
        m_buf[0] = static_cast<char>(tagValue.getType());
        memcpy(&m_buf[1], &tagValue.m_value, sizeof(EmTagValue::EmTagValueUnion));
    }   

    bool toValue(EmTagValue& tagValue) {
        // Read type
        if (!isValidTagValueType(m_buf[0])) {
            return false;
        }
        EmTagValueType type = static_cast<EmTagValueType>(m_buf[0]);
        // Read value
        tagValue.setValue_(type, *reinterpret_cast<const EmTagValue::EmTagValueUnion*>(&m_buf[1]));
        return true;
    }

    char* getBuffer() const {
        return (char*)m_buf;
    }

    size_t getSize() const {
        return sizeof(m_buf);
    }

protected:
    char m_buf[sizeof(EmTagValueType) + sizeof(EmTagValue::EmTagValueUnion)];
};

#endif // EM_STD_LIB
#endif // _EM_TAG_VALUE_H__