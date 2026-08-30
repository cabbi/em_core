#ifndef _EM_TAG_VALUE_H__
#define _EM_TAG_VALUE_H__

#include "em_defs.h"

#ifdef EM_STD_LIB // Need standard library 

#include <type_traits>

#include "em_list.h"
#include "em_string.h"
#include "em_threading.h"
#include "em_value_sync.h"
#include "em_sbo_buffer.h"

// Mutex parameter in case of multithreading
#ifdef EM_MULTITHREAD
#define MUTEX_PARAM0 EmMutex& mutex
#define MUTEX_PARAM1 EmMutex& mutex,
#define MUTEX_MEMBER_VAR0 m_mutex
#define MUTEX_MEMBER_VAR1 m_mutex,
#define MUTEX_VAR0 mutex
#define MUTEX_VAR1 mutex,
#define MUTEX_LOCK EmMutexLock lock(mutex)
#define MUTEX_UNLOCK m_mutex.unlock()
#else
#define MUTEX_PARAM0
#define MUTEX_PARAM1
#define MUTEX_MEMBER_VAR0
#define MUTEX_MEMBER_VAR1
#define MUTEX_VAR0
#define MUTEX_VAR1
#define MUTEX_LOCK
#define MUTEX_UNLOCK
#endif

// The tag value type
enum class EmTagValueType: uint8_t {
    vt_undefined = 0,
    vt_boolean = 1,
    vt_integer = 2,
    vt_epoch = 3,
    vt_real = 4,
    vt_string = 5
};

inline bool isValidTagValueType(uint8_t type) {
    return type <= static_cast<uint8_t>(EmTagValueType::vt_string);
}

// The value types (NOTE: we want to have max 32 bit values here!)
using EmBoolType    = bool;
using EmIntegerType = int32_t;
using EmRealType    = float;
using EmStringType  = EmStringBase;
using EmStringInst  = EmStringM;

// The tag value bytes union
union EmTagValueUnion {
    EmBoolType as_bool;
    EmIntegerType as_integer;
    EmEpoch32 as_epoch;
    EmRealType as_real;
    EmStringType* as_string;  // Storing pointer to keep size of union small.

    EmTagValueUnion() { as_integer = 0; }
    EmTagValueUnion(EmBoolType value) { as_bool = value; }
    EmTagValueUnion(EmEpoch32 value) { as_epoch = value; }
    EmTagValueUnion(int value) { as_integer = value; }
    EmTagValueUnion(int8_t value) { as_integer = value; }
    EmTagValueUnion(int16_t value) { as_integer = value; }
    EmTagValueUnion(int32_t value) { as_integer = value; } 
    EmTagValueUnion(uint8_t value) { as_integer = value; }
    EmTagValueUnion(uint16_t value) { as_integer = value; }
    EmTagValueUnion(uint32_t value) { as_integer = value; }
    EmTagValueUnion(float value) { as_real = static_cast<EmRealType>(value); }
    EmTagValueUnion(double value) { as_real = static_cast<EmRealType>(value); }
    EmTagValueUnion(EmStringType* value) { as_string = value; }

    template<typename T>
    EmTagValueUnion& operator = (const T& value) {
        EmTagValueUnion temp(value);
        *this = temp;
        return *this;
    }

    template<typename T>
    typename std::enable_if<!std::is_pointer<T>::value, T>::type as() const {
        return AsHelper<T>::get(*this);
    }        
    
    template<typename T>
    typename std::enable_if<std::is_pointer<T>::value, T>::type as() const {
        typedef typename std::remove_pointer<T>::type P;
        return AsHelper<P*>::get(*this);
    }

    template<typename T>
    operator T() const {
        return static_cast<T>(AsHelper<T>::get(*this));
    }

    template<typename T>
    operator T*() const {
        return static_cast<T*>(AsHelper<T*>::get(*this));
    }

private:
    // C++11 helper for as() to generate a compile-time error for unsupported types.
    template<typename T, typename V = void>
    struct AsHelper {
        static_assert(std::is_same<T, void>::value && !std::is_same<T, void>::value,
                      "Unsupported type in EmTagValueUnion::as<T>()");
    };

    // Specializations for supported types
    template<typename T> struct AsHelper<T, typename std::enable_if<std::is_same<T, bool>::value>::type> {
        static T get(const EmTagValueUnion& u) { return u.as_bool; } 
    };
    template<typename T> struct AsHelper<T, typename std::enable_if<std::is_integral<T>::value && !std::is_same<T, bool>::value && sizeof(T) <= sizeof(EmIntegerType)>::type> {
        static T get(const EmTagValueUnion& u) { return static_cast<T>(u.as_integer); } 
    };
    template<typename T> struct AsHelper<T, typename std::enable_if<std::is_floating_point<T>::value>::type> {
        static T get(const EmTagValueUnion& u) { return static_cast<T>(u.as_real); } 
    };
    template<typename T> struct AsHelper<T, typename std::enable_if<std::is_same<T, EmEpoch32>::value>::type> {
        static T get(const EmTagValueUnion& u) { return u.as_epoch; } 
    };
    // Specializations for pointer types
    template<typename T> struct AsHelper<T*, typename std::enable_if<std::is_same<T, EmStringType>::value>::type> {
        static T* get(const EmTagValueUnion& u) { return u.as_string; } 
    };
    template<typename T> struct AsHelper<T*, typename std::enable_if<std::is_same<T, const char>::value>::type> {
        static T* get(const EmTagValueUnion& u) { return u.as_string ? u.as_string->c_str() : nullptr; } 
    };
};

// Forward declaration
class EmTagValueBuffer;


// The tag value data structure used to read and write an EmTagValue object.
//
// We keep this tag data struct with basic type members an non virtual methods
// in order to allow memory copy of it (i.e. a POD with constructors and some base methods).
// In case of multithreading capability methods get a mutex to avoid concurrency. The mutex
// is external to keep this struct memory only for its members (i.e. type and value).
struct EmTagValueStruct {
    friend class EmTagValueBuffer;

    EmTagValueStruct(): m_type(EmTagValueType::vt_undefined), m_value() {}
    EmTagValueStruct(EmBoolType value): m_type(EmTagValueType::vt_boolean), m_value(value) {} 
    EmTagValueStruct(EmEpoch32 value): m_type(EmTagValueType::vt_epoch), m_value(value) {}
    EmTagValueStruct(int value): m_type(EmTagValueType::vt_integer), m_value(value) {} 
    EmTagValueStruct(int8_t value): m_type(EmTagValueType::vt_integer), m_value(value) {} 
    EmTagValueStruct(int16_t value): m_type(EmTagValueType::vt_integer), m_value(value) {} 
    EmTagValueStruct(int32_t value): m_type(EmTagValueType::vt_integer), m_value(value) {} 
    EmTagValueStruct(uint8_t value): m_type(EmTagValueType::vt_integer), m_value(value) {} 
    EmTagValueStruct(uint16_t value): m_type(EmTagValueType::vt_integer), m_value(value) {} 
    EmTagValueStruct(uint32_t value): m_type(EmTagValueType::vt_integer), m_value(value) {} 
    EmTagValueStruct(float value): m_type(EmTagValueType::vt_real), m_value(value) {} 
    EmTagValueStruct(double value): m_type(EmTagValueType::vt_real), m_value(value) {} 
    EmTagValueStruct(EmStringType* value): m_type(EmTagValueType::vt_string), m_value(value) {}
    EmTagValueStruct(EmTagValueType type): m_type(type), m_value() {}
    EmTagValueStruct(EmTagValueType type, EmTagValueUnion value): m_type(type), m_value(value) {}

    EmTagValueType getType() const { 
        return m_type; 
    }

    EmTagValueType getTypeTs(MUTEX_PARAM0) const { 
        MUTEX_LOCK;
        return m_type; 
    }
    
    void setType(EmTagValueType type) { 
        m_type = type;
    }

    void setTypeTs(MUTEX_PARAM1 EmTagValueType type) { 
        MUTEX_LOCK;
        m_type = type;
    }

    EmTagValueUnion getValue() const { 
        return m_value; 
    }
    
    EmTagValueUnion getValueTs(MUTEX_PARAM0) const { 
        MUTEX_LOCK;
        return m_value; 
    }
    
    void get(EmTagValueType& type, EmTagValueUnion& value) const { 
        type = m_type;
        value = m_value; 
    }

    void getTs(MUTEX_PARAM1 EmTagValueType& type, EmTagValueUnion& value) const { 
        MUTEX_LOCK;
        type = m_type;
        value = m_value; 
    }

    template<class T>
    void set(EmTagValueType type, T value) { 
        set_(type, value);
    }

    template<class T>
    void setTs(MUTEX_PARAM1 EmTagValueType type, T value) { 
        MUTEX_LOCK;
        set_(type, value);
    }

    void clear() {
        clear_();
    }

    void clearTs(MUTEX_PARAM0) {
        MUTEX_LOCK;
        clear_();
    }

    bool setValue(bool value, bool forceType) {
        if (!forceType && m_type != EmTagValueType::vt_boolean && m_type != EmTagValueType::vt_undefined) {
            return false;
        }
        set_(EmTagValueType::vt_boolean, value);
        return true;
    }

    bool setValueTs(MUTEX_PARAM1 bool value, bool forceType) {
        MUTEX_LOCK;
        return setValue(value, forceType);
    }

    bool setValue(int32_t value, bool forceType) {
        if (!forceType && m_type != EmTagValueType::vt_integer && m_type != EmTagValueType::vt_undefined) {
            return false;
        }
        set_(EmTagValueType::vt_integer, value);
        return true;
    }

    bool setValueTs(MUTEX_PARAM1 int32_t value, bool forceType) {
        MUTEX_LOCK;
        return setValue(value, forceType);
    }

    bool setValue(float value, bool forceType) {
        if (!forceType && m_type != EmTagValueType::vt_real && m_type != EmTagValueType::vt_undefined) {
            return false;
        }
        set_(EmTagValueType::vt_real, value);
        return true;
    }

    bool setValueTs(MUTEX_PARAM1 float value, bool forceType) {
        MUTEX_LOCK;
        return setValue(value, forceType);
    }

    bool setValue(double value, bool forceType) {
        if (!forceType && m_type != EmTagValueType::vt_real && m_type != EmTagValueType::vt_undefined) {
            return false;
        }
        set_(EmTagValueType::vt_real, value);
        return true;
    }

    bool setValueTs(MUTEX_PARAM1 double value, bool forceType) {
        MUTEX_LOCK;
        return setValue(value, forceType);
    }

    bool setValue(const EmStringType& value, bool forceType) {
        return setValue(value.c_str(), forceType);
    }

    bool setValueTs(MUTEX_PARAM1 const EmStringType& value, bool forceType) {
        return setValueTs(MUTEX_VAR1 value.c_str(), forceType);
    }

    bool setValue(const char* value, bool forceType) {
        if (m_type == EmTagValueType::vt_string) {
            // Already a string, just reassign the value to avoid delete/new cycle.
            m_value.as_string->set(value);
            return true;
        }
        if (!forceType && m_type != EmTagValueType::vt_undefined) {
            return false;
        }
        set_(EmTagValueType::vt_string, new EmStringInst(value));
        return true;
    }

    bool setValueTs(MUTEX_PARAM1 const char* value, bool forceType) {
        MUTEX_LOCK;
        return setValue(value, forceType);
    }

    bool setEpoch(const EmEpoch32& value, bool forceType) {
        if (!forceType && m_type != EmTagValueType::vt_epoch && m_type != EmTagValueType::vt_undefined) {
            return false;
        }
        set_(EmTagValueType::vt_epoch, value);
        return true;
    }

    bool setEpochTs(MUTEX_PARAM1 const EmEpoch32& value, bool forceType) {
        MUTEX_LOCK;
        return setEpoch(value, forceType);
    }

    void toStruct(EmTagValueStruct& out) const {
        out.set_(this->m_type, this->m_value);
    }

    void toStructTs(MUTEX_PARAM1 EmTagValueStruct& out) const {
        MUTEX_LOCK;
        toStruct(out);
    }

    void fromStruct(const EmTagValueStruct& in) {
        if (in.m_type == EmTagValueType::vt_string) {
            if (m_type == EmTagValueType::vt_string) {
                // Already a string, just reassign the value to avoid delete/new cycle.
                m_value.as_string->set(*(in.m_value.as_string));
            } else {
                set_(EmTagValueType::vt_string, new EmStringInst(*(in.m_value.as_string)));
            }
            return;
        }
        set_(in.m_type, in.m_value);
    }

    void fromStructTs(MUTEX_PARAM1 const EmTagValueStruct& in) {
        MUTEX_LOCK;
        fromStruct(in);
    }

    size_t getValueBufferSize() const {
        switch (m_type) {
            case EmTagValueType::vt_string:
                return m_value.as_string ? m_value.as_string->length() + 1 : 0;
            default:
                return sizeof(m_value);
        }
    }

    const void* getValueBuffer() const {
        switch (m_type) {
            case EmTagValueType::vt_string:
                return m_value.as_string ? m_value.as_string->c_str() : nullptr;
            default:
                return &m_value;
        }
    }

protected:
    void clear_() {
        if (m_type == EmTagValueType::vt_string) {
            delete m_value.as_string;
        }
        m_type = EmTagValueType::vt_undefined;
        m_value.as_integer = 0; // Zero out the union
    }

    template<class T>
    void set_(EmTagValueType type, T value) { 
        clear_();
        m_type = type;
        m_value = value; 
    }

    EmTagValueType m_type;
    EmTagValueUnion m_value;
};


// The tag value buffer class is used to read and write a tag value in a memory buffer.
// It uses a small buffer optimization to avoid heap fragmentation when reading/writing tags from/to storage.
class EmTagValueBuffer: public EmSboBuffer<char, 128> {
public:
    EmTagValueBuffer(const EmTagValueStruct& tagValue) {
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

    void fromValue(const EmTagValueStruct& tagValue) {
        m_size = tagValue.getValueBufferSize();
        setMaxSize(m_size+1);
        char* buf = getBuffer();
        if (buf) {
            buf[0] = static_cast<char>(tagValue.getType());
            if (m_size > 0) {
                memcpy(&buf[1], tagValue.getValueBuffer(), m_size);
            }
        }
    }   

    bool toValue(EmTagValueStruct& tagValue) {
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
                tagValue.setValue(strValue, true);
            } else {
                // For other types, the value is stored as binary data in the buffer
                tagValue.set_(type, *reinterpret_cast<const EmTagValueUnion*>(&buf[1]));
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


// The tag value class.
//
// This class is used to have a concrete implementation of value since 
// 'EmTag' and 'EmTags' classes will not support templates.
class EmTagValue: public EmTagValueStruct {  
public:
    EmTagValue() : EmTagValueStruct(EmTagValueType::vt_undefined) {}
    EmTagValue(EmTagValueType type) : EmTagValueStruct(type) {}
    EmTagValue(EmBoolType value) : EmTagValueStruct(value) {}
    EmTagValue(EmEpoch32 value) : EmTagValueStruct(value) {}
    EmTagValue(int value) : EmTagValueStruct(value) {}
    EmTagValue(int8_t value) : EmTagValueStruct(value) {}
    EmTagValue(int16_t value) : EmTagValueStruct(value) {}
    EmTagValue(int32_t value) : EmTagValueStruct(value) {}
    EmTagValue(uint8_t value) : EmTagValueStruct(value) {}
    EmTagValue(uint16_t value) : EmTagValueStruct(value) {}
    EmTagValue(uint32_t value) : EmTagValueStruct(value) {}
    EmTagValue(float value) : EmTagValueStruct(value) {}
    EmTagValue(double value) : EmTagValueStruct(value) {}
    EmTagValue(const char* value) : EmTagValueStruct(new EmStringInst(value)) {}
    EmTagValue(const EmStringType& value) : EmTagValueStruct(new EmStringInst(value)) {}
    EmTagValue(const EmTagValue& other) : EmTagValueStruct(EmTagValueType::vt_undefined) {
        fromValue(other);
    }

    // NOTE: keep destructor and class without virtual functions to limit RAM footprint
    ~EmTagValue() {
        clearTs(MUTEX_MEMBER_VAR0);
    }   

    template<typename T>
    bool operator ==(const T& other) const {
        return this->m_value.as<T>() == other;
    }

    template<typename P>
    bool operator ==(const P* other) const {
        return this->m_value.as<P*>() == other;
    }

    bool operator ==(const EmTagValue& other) const {
        // Get the other type & value
        EmTagValueType otherType;
        EmTagValueUnion otherValue;
        other.get(otherType, otherValue);

        // Type check
        EmTagValueType type = getType();
        if (type != otherType) {
            return false;
        }

        // Value check
        EmTagValueUnion value = getValue();
        switch (type) {
            case EmTagValueType::vt_undefined:    
                return true; // Two undefined values are considered equal
            case EmTagValueType::vt_string:
                // Ensure both pointers are valid before dereferencing
                if (value.as_string && otherValue.as_string) {
                    return *value.as_string == *otherValue.as_string;
                }
                return value.as_string == otherValue.as_string; // Both are nullptr
            default: 
                return value.as_real == otherValue.as_real;
        }
    }

    bool operator !=(const EmTagValue& other) const {
        return !(*this == other);
    }

    bool operator >(const EmTagValue& other) const {
        // Get the other type & value
        EmTagValueType otherType;
        EmTagValueUnion otherValue;
        other.get(otherType, otherValue);

        // Type check
        EmTagValueType type = getType();
        if (type != otherType) {
            return false;
        }
        // Value check
        EmTagValueUnion value = getValue();
        switch (type) {
            case EmTagValueType::vt_undefined:    
                return false; // Two undefined values are considered comparable
            case EmTagValueType::vt_string:
                // Ensure both pointers are valid before dereferencing
                if (value.as_string && otherValue.as_string) {
                    return *value.as_string > *otherValue.as_string;
                }
                return false;
            default: 
                return value.as_real > otherValue.as_real;
        }
    }

    bool operator >=(const EmTagValue& other) const {
        return (*this > other) || (*this == other);
    }

    bool operator <(const EmTagValue& other) const {
        return !(*this > other) && (*this != other);
    }

    bool operator <=(const EmTagValue& other) const {
        return !(*this > other);
    }

    EmTagValue& operator=(const EmTagValue& other) {
        if (this != &other) {
            fromValue(other);
        }
        return *this;
    }

    EmTagValueType getType() const {
        return EmTagValueStruct::getTypeTs(MUTEX_MEMBER_VAR0);
    }

    EmTagValueUnion getValue() const {
        return EmTagValueStruct::getValueTs(MUTEX_MEMBER_VAR0);
    }

    void get(EmTagValueType& type, EmTagValueUnion& value) const { 
        return EmTagValueStruct::getTs(MUTEX_MEMBER_VAR1 type, value);
    }

    bool isSameType(const EmTagValue& other) const {
        return getType() == other.getType();
    }

    bool isType(EmTagValueType type) const { 
        return getType() == type; 
    }

    bool isNotType(EmTagValueType type) const { 
        return getType() != type; 
    }

    bool isUndefinedType() const { 
        return getType() == EmTagValueType::vt_undefined;  
    }

    bool isNotUndefinedType() const { 
        return !isUndefinedType();  
    }

    void setUndefinedType() { 
        setTypeTs(MUTEX_MEMBER_VAR1 EmTagValueType::vt_undefined);
    }

    EmBoolType asBool() const {
        return (getType() == EmTagValueType::vt_boolean) ? getValue().as_bool : false;
    }
    
    EmIntegerType asInteger() const {
        return (getType() == EmTagValueType::vt_integer) ? getValue().as_integer : 0;
    }

    EmEpoch32 asEpoch() const {
        return (getType() == EmTagValueType::vt_epoch) ? getValue().as_epoch : EmEpoch32(0);
    }
    
    EmRealType asReal() const {
        return (getType() == EmTagValueType::vt_real) ? getValue().as_real : static_cast<EmRealType>(0.0);
    }
    
    EmStringType* asStringPtr() const {
        EmTagValueType type;
        EmTagValueUnion value;
        get(type, value);
        return (type == EmTagValueType::vt_string && value.as_string != nullptr) ? value.as_string : nullptr;
    }
    
    const char* asString() const {
        EmTagValueType type;
        EmTagValueUnion value;
        get(type, value);
        return (type == EmTagValueType::vt_string && value.as_string != nullptr) ? value.as_string->c_str() : "";
    }
    
    const EmTagValueStruct& asStruct() const {
        return *this;
    }

    void fromValue(const EmTagValue& in) {
        EmTagValueStruct::fromStructTs(MUTEX_MEMBER_VAR1 in);
    }

    void toStruct(EmTagValueStruct& out) const {
        out.fromStructTs(MUTEX_MEMBER_VAR1 *this);
    }

    void fromStruct(const EmTagValueStruct& in) {
        EmTagValueStruct::fromStructTs(MUTEX_MEMBER_VAR1 in);
    }

    template<typename T>
    EmGetValueResult getValue(T& value) const {
        // Get type and value 
        EmTagValueType thisType;
        EmTagValueUnion thisValue;
        get(thisType, thisValue);
        
        if (std::is_same<T, bool>::value) {
            if (thisType != EmTagValueType::vt_boolean) {
                return EmGetValueResult::failed;
            }
            EmGetValueResult res = (value == thisValue.as_bool) 
                            ? EmGetValueResult::succeedEqualValue 
                            : EmGetValueResult::succeedNotEqualValue;
            value = thisValue.as_bool;
            return res;
        } else
        if (std::is_integral<T>::value) {
            if (thisType != EmTagValueType::vt_integer) {
                return EmGetValueResult::failed;
            }
            EmGetValueResult res = (static_cast<EmIntegerType>(value) == thisValue.as_integer)
                                ? EmGetValueResult::succeedEqualValue
                                : EmGetValueResult::succeedNotEqualValue;
            value = static_cast<T>(thisValue.as_integer);
            return res;
        } else
        if (std::is_floating_point<T>::value) {
            if (thisType != EmTagValueType::vt_real)  {
                return EmGetValueResult::failed;
            }
            EmGetValueResult res = (static_cast<EmRealType>(value) == thisValue.as_real)
                                ? EmGetValueResult::succeedEqualValue
                                : EmGetValueResult::succeedNotEqualValue;
            value = static_cast<T>(thisValue.as_real);
            return res;
        }
        return EmGetValueResult::failed;
    }

    EmGetValueResult getValue(EmStringType& value) const {
        // Get type and value 
        EmTagValueType thisType;
        EmTagValueUnion thisValue;
        get(thisType, thisValue);

        if (thisType != EmTagValueType::vt_string) {
            return EmGetValueResult::failed;
        }
        EmGetValueResult res = (value == *thisValue.as_string)
                               ? EmGetValueResult::succeedEqualValue 
                               : EmGetValueResult::succeedNotEqualValue;
        value.set(*thisValue.as_string);
        return res;
    }

    EmGetValueResult getValue(EmTagValue& value) const {
        // Is already equal
        if (*this == value) {
            return EmGetValueResult::succeedEqualValue; 
        }
        // Compatible type?
        if (!isSameType(value) && value.getType() != EmTagValueType::vt_undefined) {
            return EmGetValueResult::failed;
        }
        // Set new value
        value.fromValue(*this);
        return EmGetValueResult::succeedNotEqualValue;        
    }

    template<typename T>
    T as() const {
        // Get type and value 
        EmTagValueType thisType;
        EmTagValueUnion thisValue;
        get(thisType, thisValue);

        switch (thisType) {
            case EmTagValueType::vt_boolean:
                return static_cast<T>(thisValue.as_bool);
            case EmTagValueType::vt_integer:
                return static_cast<T>(thisValue.as_integer);
            case EmTagValueType::vt_real:
                return static_cast<T>(thisValue.as_real);
            case EmTagValueType::vt_string:
                // TODO: handle string to numeric conversion?
                return static_cast<T>(0);
            case EmTagValueType::vt_undefined:
            default:
                return static_cast<T>(0);
        }
    }

    bool setValue(bool value, bool forceType) {
        return EmTagValueStruct::setValueTs(MUTEX_MEMBER_VAR1 value, forceType);
    }

    bool setValue(int32_t value, bool forceType) {
        return EmTagValueStruct::setValueTs(MUTEX_MEMBER_VAR1 value, forceType);
    }

    bool setValue(float value, bool forceType) {
        return EmTagValueStruct::setValueTs(MUTEX_MEMBER_VAR1 value, forceType);
    }

    bool setValue(double value, bool forceType) {
        return EmTagValueStruct::setValueTs(MUTEX_MEMBER_VAR1 value, forceType);
    }

    bool setValue(const EmStringType& value, bool forceType) {
        return EmTagValueStruct::setValueTs(MUTEX_MEMBER_VAR1 value, forceType);
    }

    bool setValue(const char* value, bool forceType) {
        return EmTagValueStruct::setValueTs(MUTEX_MEMBER_VAR1 value, forceType);
    }
    
    bool setValue(const EmTagValue& value, bool forceType) {
        EmTagValueType otherType;
        EmTagValueUnion otherValue;
        value.get(otherType, otherValue);

        EmTagValueType thisType = getType();
        if (!forceType && thisType != otherType && thisType != EmTagValueType::vt_undefined) {
            return false;
        }
        setTs(MUTEX_MEMBER_VAR1 otherType, otherValue);
        return true;
    }
    
    bool setValue(const EmTagValue& value) {
        return setValue(value, false);
    }

protected:
    void fromValue_(const EmTagValue& in) {
        fromStruct_(in);
    }

    void toStruct_(EmTagValueStruct& out) const {
        toStruct(out);
    }

    void fromStruct_(const EmTagValueStruct& in) {
        EmTagValueStruct::fromStruct(in);
    }


    // Member vars
#ifdef EM_MULTITHREAD
    mutable EmMutex m_mutex;
#endif
};

#endif // EM_STD_LIB
#endif // _EM_TAG_VALUE_H__