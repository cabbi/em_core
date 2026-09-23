#ifndef _EM_TAG_VALUE_H__
#define _EM_TAG_VALUE_H__

#include "em_defs.h"

#ifdef EM_STD_LIB // Need standard library 
#include <type_traits>

#include "em_value.h"
#include "em_epoch.h"
#include "em_string.h"
#include "em_threading.h"
#include "em_value_sync.h"


enum class EmTagValueType: uint8_t {
    vt_undefined = 0,
    // The base types
    vt_bool      = 1,
    vt_int       = 2,
    vt_uint      = 3,
    vt_real      = 4,
    vt_epoch     = 5,
    vt_string    = 6,
    // Any other custom object type
    vt_object    = 7,
    // Max enum value
    _vt_MAX      = vt_object
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
template<size_t SizeOfT> class EmTagValueBuffer;

// Tag value is thread safe in multithreaded capable environments.
#ifdef EM_MULTITHREAD
    #define MUTEX_LOCK EmMutexLock lock(this->m_mutex)
    #define MUTEX_DUAL_LOCK(other) EmDualLock dual_lock(this->m_mutex, other.m_mutex)
#else
    #define MUTEX_LOCK
    #define MUTEX_DUAL_LOCK(other)
#endif

// The base tag value class holding a value of type T.
// NOTES: 
//   - derived classes SHOULD override the 'getType()' method!
//     This class is not abstract because in some cases we need to create a default value of it.
//   - T must have the '==' operator defined for returning  
//     the correct 'EmGetValueResult' in the 'getValue' method.
template<typename T>
class EmTagValue: public EmValue<T> {
    template<size_t SizeOfT>
    friend class EmTagValueBuffer;
public:
    EmTagValue() = default;
    explicit EmTagValue(const EmTagValue& value) : m_value(value.m_value) {}
    explicit EmTagValue(EmTagValue&& value) noexcept : m_value(std::move(value.m_value)) {}
    explicit EmTagValue(const T& value) : m_value(value) {}
    explicit EmTagValue(T&& value) noexcept : m_value(std::move(value)) {}
    
    virtual ~EmTagValue() = default;

    // Need to be overridden on derived class.
    // Type is used as "kind of" reflection in some classes like EmStorage.
    virtual EmTagValueType getType() const {
        return EmTagValueType::vt_undefined;
    }

    // Copy and move operators
    EmTagValue& operator =(const EmTagValue& value) {
        if (this == &value) {
            return *this;
        }
        MUTEX_DUAL_LOCK(value);
        assign_(this->m_value, value.m_value);
        return *this;
    }

    EmTagValue& operator =(EmTagValue&& value) noexcept {
        if (this == &value) {
            return *this;
        } 
        MUTEX_DUAL_LOCK(value);
        move_(this->m_value, value.m_value);
        return *this;
    }

    EmTagValue& operator =(const T& value) {
        MUTEX_LOCK;
        assign_(this->m_value, value);
        return *this;
    }

    EmTagValue& operator =(T&& value) noexcept {
        MUTEX_LOCK;
        move_(this->m_value, value);
        return *this;
    }

    // The 'getValue' methods
    virtual EmGetValueResult getValue(T& value) const override {
        MUTEX_LOCK;
        if (m_value == value) {
            return EmGetValueResult::succeedEqualValue;
        }
        assign_(value, this->m_value);
        return EmGetValueResult::succeedNotEqualValue;
    }        

    virtual EmGetValueResult getValue(EmTagValue& value) const {
        if (this == &value) {
            return EmGetValueResult::succeedEqualValue;
        }
        MUTEX_DUAL_LOCK(value);
        if (this->m_value == value.m_value) {
            return EmGetValueResult::succeedEqualValue;
        }
        assign_(value.m_value, this->m_value);
        return EmGetValueResult::succeedNotEqualValue;
    }        
    
    // The 'setValue' methods
    virtual bool setValue(const T& value) override {
        MUTEX_LOCK;
        assign_(this->m_value, value);
        return true;
    }
    
    virtual bool setValue(const EmTagValue& value) {
        if (this == &value) {
            return true;
        }            
        MUTEX_DUAL_LOCK(value);
        assign_(this->m_value, value.m_value);
        return true;
    }

    // Equality operators
    bool operator==(const EmTagValue& value) const {
        if (this == &value) {
            return true;
        }
        MUTEX_DUAL_LOCK(value);
        return this->m_value == value.m_value;
    } 

    bool operator!=(const EmTagValue& value) const {
       !(*this == value);
    } 

    bool operator==(const T& value) const {
        MUTEX_LOCK;
        return this->m_value == value;
    } 

    bool operator!=(const T& value) const {
        MUTEX_LOCK;
        return this->m_value != value;
    } 

protected:
    explicit EmTagValue(T& value) : m_value(value) {}

    // The two methods used by the 'EmTagValueBuffer' class
    // NOTE: 
    // you need to override those two methods in case of "complex" T class!
    virtual size_t getValueSize_() const {
        return sizeof(T);
    }
    virtual char* getValueBuffer_() {
        return reinterpret_cast<char*>(&this->m_value);
    }

    // Internal assignment and move methods
    void assign_(T& dest, const T& src) const {
        if constexpr (std::is_base_of_v<EmStringBase, T>) {
            dest.set(src); 
        } else {
            dest = src;
        }
    }
    void move_(T& dest, const T& src) const {
        if constexpr (std::is_base_of_v<EmStringBase, T>) {
            dest = std::move(src);
        } else {
            dest = src;
        }
    }

    // Member vars
    T m_value; 
#ifdef EM_MULTITHREAD
    mutable EmMutex m_mutex;
#endif
};    

// The abstract tag value class where T is comparable (>, >=, < and <= ). 
template<typename T>
class EmTagComparableValue: public EmTagValue<T> {
public:
    using EmTagValue<T>::EmTagValue;

    bool operator>(const EmTagComparableValue& value) const {
        if (this == &value) {
            return true;
        }
        MUTEX_DUAL_LOCK(value);
        return this->m_value > value.m_value;
    } 

    bool operator<(const EmTagComparableValue& value) const {
        if (this == &value) {
            return true;
        }
        MUTEX_DUAL_LOCK(value);
        return this->m_value < value.m_value;
    } 

    bool operator>=(const EmTagComparableValue& value) const {
        if (this == &value) {
            return true;
        }
        MUTEX_DUAL_LOCK(value);
        return this->m_value >= value.m_value;
    } 

    bool operator<=(const EmTagComparableValue& value) const {
        if (this == &value) {
            return true;
        }
        MUTEX_DUAL_LOCK(value);
        return this->m_value <= value.m_value;
    } 

    bool operator>(const T& value) const { MUTEX_LOCK; return this->m_value > value; } 
    bool operator<(const T& value) const { MUTEX_LOCK; return this->m_value < value; } 
    bool operator>=(const T& value) const { MUTEX_LOCK; return this->m_value >= value; } 
    bool operator<=(const T& value) const { MUTEX_LOCK; return this->m_value <= value; } 
};

// The abstract "basic" type tag value class where T is a base type (e.g. int, float, etc.)
// This class defines basic methods
template<typename T>
class EmTagBaseTypeValue: public EmTagComparableValue<T> {
public:
    using EmTagComparableValue<T>::EmTagComparableValue;
    EmTagBaseTypeValue() : EmTagComparableValue<T>(0) {}

    using EmTagComparableValue<T>::getValue;
    virtual T getValue() const { 
        MUTEX_LOCK;
        return this->m_value; 
    }
};

// The basic types' tag value classes. 
class EmBoolTagValue: public EmTagBaseTypeValue<EmBoolType> {
public:
    using EmTagBaseTypeValue::EmTagBaseTypeValue;

    virtual EmTagValueType getType() const override { 
        return EmTagValueType::vt_bool; 
    }
};

class EmIntTagValue: public EmTagBaseTypeValue<EmIntType> {
public:
    using EmTagBaseTypeValue::EmTagBaseTypeValue;

    virtual EmTagValueType getType() const override { 
        return EmTagValueType::vt_int; 
    }
};

class EmUIntTagValue: public EmTagBaseTypeValue<EmUIntType> {
public:
    using EmTagBaseTypeValue::EmTagBaseTypeValue;

    virtual EmTagValueType getType() const override { 
        return EmTagValueType::vt_uint; 
    }
};

class EmRealTagValue: public EmTagBaseTypeValue<EmRealType> {
public:
    using EmTagBaseTypeValue::EmTagBaseTypeValue;

    virtual EmTagValueType getType() const override { 
        return EmTagValueType::vt_real; 
    }
};

class EmEpochTagValue: public EmTagBaseTypeValue<EmEpochType> {
public:
    using EmTagBaseTypeValue::EmTagBaseTypeValue;

    virtual EmTagValueType getType() const override { 
        return EmTagValueType::vt_epoch; 
    }
};

// The string tag value class. 
class EmStringTagValue: public EmTagComparableValue<EmStringBase> {
public:  
    // Explict string constructor
    explicit EmStringTagValue(EmStringBase& value)
     : EmTagComparableValue<EmStringBase>(value) {}

    // No copy and move constructors for a string tag!
    EmStringTagValue(const EmStringTagValue&) = delete;
    EmStringTagValue(EmStringTagValue&&) = delete;

    virtual EmTagValueType getType() const override { 
        return EmTagValueType::vt_string; 
    }

    // Removed assignment operators
    EmStringTagValue& operator =(const EmStringTagValue& value) = delete;
    EmStringTagValue& operator =(EmStringTagValue&& value) = delete;
    EmStringTagValue& operator =(const EmStringBase& value) = delete;
    EmStringTagValue& operator =(EmStringBase&& value) = delete;

    // The 'getValue' methods
    virtual EmGetValueResult getValue(EmStringBase& value) const override {
        MUTEX_LOCK;
        if (this->m_value == value) {
            return EmGetValueResult::succeedEqualValue;
        }
        if (value.set(this->m_value)) {
            return EmGetValueResult::succeedNotEqualValue;
        }
        return EmGetValueResult::failed;
    }        

    virtual EmGetValueResult getValue(EmStringTagValue& value) const {
        if (this == &value) {
            return EmGetValueResult::succeedEqualValue;
        }
        MUTEX_DUAL_LOCK(value);
        if (this->m_value == value.m_value) {
            return EmGetValueResult::succeedEqualValue;
        }
        if (value.m_value.set(this->m_value)) {
            return EmGetValueResult::succeedNotEqualValue;
        }
        return EmGetValueResult::failed;
    }        
    
    // The 'setValue' methods
    virtual bool setValue(const EmStringBase& value) override {
        MUTEX_LOCK;
        return this->m_value.set(value);
    }
    
    virtual bool setValue(const EmStringTagValue& value) {
        if (this == &value) {
            return true;
        }            
        MUTEX_DUAL_LOCK(value);
        return this->m_value.set(value.m_value);
    }

    virtual bool setValue(const char* value) {
        MUTEX_LOCK;
        return this->m_value.set(value);
    }

    bool operator==(const char* value) const {
        MUTEX_LOCK;
        return this->m_value == value;
    } 

    // String handling methods
    const char* c_str() const {
        return this->m_value.c_str();
    }

protected:
    // The two methods used by the 'EmTagValueBuffer' class
    virtual size_t getValueSize_() const override {
        return this->m_value.length();
    }
    virtual char* getValueBuffer_() override {
        return this->m_value.buffer();
    }
};


// The generic tag value buffer class is used to read and write a tag value in a memory buffer.
template<size_t SizeOfTBuf>
class EmTagValueBuffer {
public:
    EmTagValueBuffer() {
        clear();
    }

    template<typename T>
    EmTagValueBuffer(const EmTagValue<T>& tagValue) {
        fromValue(tagValue);
    }

    ~EmTagValueBuffer() {
        clear();
    }

    void clear() {
        memset(m_buf, 0, sizeof(m_buf));
    }

    template<typename T>
    void fromValue(const EmTagValue<T>& tagValue) {
        m_buf[0] = static_cast<char>(tagValue.getType());
        memcpy(&m_buf[1], tagValue.getValueBuffer_(), tagValue.getValueSize_());
    }   

    template<typename T>
    bool toValue(EmTagValue<T>& tagValue) {
        // Read type
        if (!isValidTagValueType(m_buf[0])) {
            return false;
        }
        EmTagValueType type = static_cast<EmTagValueType>(m_buf[0]);
        // Read value
        memcpy(&tagValue.getValueBuffer_(), &m_buf[1], getSize());
        return true;
    }

    char* getBuffer() const {
        return (char*)m_buf;
    }

    size_t getSize() const {
        return sizeof(m_buf);
    }

protected:
    char m_buf[sizeof(EmTagValueType) + SizeOfTBuf];
};


// The value buffer for base types (i.e. EmTagBaseTypeValue)
template<typename T>
using EmTagBaseValueBuffer = EmTagValueBuffer<sizeof(T)>;

// The string value buffer (just a redefinition to have clear naming)
template<size_t Capacity>
using EmTagStringValueBuffer = EmTagValueBuffer<Capacity>;


#endif // EM_STD_LIB
#endif // _EM_TAG_VALUE_H__