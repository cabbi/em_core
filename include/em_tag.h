#ifndef _EM_TAG_H__
#define _EM_TAG_H__

#include "em_defs.h"

#ifdef EM_STD_LIB // Need standard library 

#include <WString.h>
#include <type_traits>

#include "em_list.h"
#include "em_string.h"
#include "em_threading.h"
#include "em_value_sync.h"

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

// The value types
using EmBoolType    = bool;
using EmIntegerType = int32_t;
using EmEpochType   = uint32_t;
using EmRealType    = float;
using EmStringType  = String;

// The tag value bytes union
union EmTagValueUnion {
    EmBoolType as_bool;
    EmIntegerType as_integer;
    EmEpochType as_epoch;
    EmRealType as_real;
    EmStringType* as_string;

    EmTagValueUnion() { as_integer = 0; }
    EmTagValueUnion(EmBoolType value) { as_bool = value; }
    EmTagValueUnion(EmIntegerType value) { as_integer = value; }
    EmTagValueUnion(EmEpochType value) { as_epoch = value; }
    EmTagValueUnion(float value) { as_real = static_cast<EmRealType>(value); }
    EmTagValueUnion(double value) { as_real = static_cast<EmRealType>(value); }
    EmTagValueUnion(EmStringType* value) { as_string = value; }

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
    template<typename T> struct AsHelper<T, typename std::enable_if<std::is_same<T, EmEpochType>::value>::type> {
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


// The tag value data structure used to read and write an EmTagValue object.
//
// We keep this tag data struct with basic type members an non virtual methods
// in order to allow memory copy of it (i.e. a POD with constructors and some base methods).
// In case of multithreading capability methods get a mutex to avoid concurrency. The mutex
// is external to keep this struct memory only for its members (i.e. type and value).
struct EmTagValueStruct {
    EmTagValueStruct(): m_type(EmTagValueType::vt_undefined), m_value{0} {}
    EmTagValueStruct(EmBoolType value): m_type(EmTagValueType::vt_boolean), m_value(value) {}
    EmTagValueStruct(EmIntegerType value): m_type(EmTagValueType::vt_integer), m_value(value) {}
    EmTagValueStruct(EmEpochType value): m_type(EmTagValueType::vt_epoch), m_value(value) {} 
    EmTagValueStruct(float value): m_type(EmTagValueType::vt_real), m_value(value) {}
    EmTagValueStruct(double value): m_type(EmTagValueType::vt_real), m_value(value) {}
    EmTagValueStruct(EmStringType* value): m_type(EmTagValueType::vt_string), m_value(value) {}
    EmTagValueStruct(EmTagValueType type, EmTagValueUnion value = {0}): m_type(type), m_value(value) {}

    EmTagValueType getType(MUTEX_PARAM0) const { 
        MUTEX_LOCK;
        return m_type; 
    }
    
    void setType(MUTEX_PARAM1 EmTagValueType type) { 
        MUTEX_LOCK;
        m_type = type;
    }

    EmTagValueUnion getValue(MUTEX_PARAM0) const { 
        MUTEX_LOCK;
        return m_value; 
    }
    
    void get(MUTEX_PARAM1 EmTagValueType& type, EmTagValueUnion& value) const { 
        MUTEX_LOCK;
        type = m_type;
        value = m_value; 
    }

    template<class T>
    void set(MUTEX_PARAM1 EmTagValueType type, T value) { 
        MUTEX_LOCK;
        set_(type, value);
    }

    void clear(MUTEX_PARAM0) {
        MUTEX_LOCK;
        clear_();
    }

    bool setValue(MUTEX_PARAM1 bool value, bool forceType) {
        MUTEX_LOCK;
        if (!forceType && m_type != EmTagValueType::vt_boolean && m_type != EmTagValueType::vt_undefined) {
            return false;
        }
        set_(EmTagValueType::vt_boolean, value);
        return true;
    }

    bool setValue(MUTEX_PARAM1 int32_t value, bool forceType) {
        MUTEX_LOCK;
        if (!forceType && m_type != EmTagValueType::vt_integer && m_type != EmTagValueType::vt_undefined) {
            return false;
        }
        set_(EmTagValueType::vt_integer, value);
        return true;
    }

    bool setValue(MUTEX_PARAM1 float value, bool forceType) {
        MUTEX_LOCK;
        if (!forceType && m_type != EmTagValueType::vt_real && m_type != EmTagValueType::vt_undefined) {
            return false;
        }
        set_(EmTagValueType::vt_real, value);
        return true;
    }

    bool setValue(MUTEX_PARAM1 double value, bool forceType) {
        MUTEX_LOCK;
        if (!forceType && m_type != EmTagValueType::vt_real && m_type != EmTagValueType::vt_undefined) {
            return false;
        }
        set_(EmTagValueType::vt_real, value);
        return true;
    }

    bool setValue(MUTEX_PARAM1 const EmStringType& value, bool forceType) {
        return setValue(MUTEX_VAR1 value.c_str(), forceType);
    }

    bool setValue(MUTEX_PARAM1 const char* value, bool forceType) {
        MUTEX_LOCK;
        if (m_type == EmTagValueType::vt_string) {
            // Already a string, just reassign the value to avoid delete/new cycle.
            *m_value.as_string = value;
            return true;
        }
        if (!forceType && m_type != EmTagValueType::vt_undefined) {
            return false;
        }
        set_(EmTagValueType::vt_string, new EmStringType(value));
        return true;
    }

    void toStruct(MUTEX_PARAM1 EmTagValueStruct& out) const {
        MUTEX_LOCK;
        toStruct_(out);
    }

    void fromStruct(MUTEX_PARAM1 const EmTagValueStruct& in) {
        MUTEX_LOCK;
        fromStruct_(in);
    }

protected:
    void toStruct_(EmTagValueStruct& out) const {
        out.set_(this->m_type, this->m_value);
    }

    void fromStruct_(const EmTagValueStruct& in) {
        if (in.m_type == EmTagValueType::vt_string) {
            if (m_type == EmTagValueType::vt_string) {
                // Already a string, just reassign the value to avoid delete/new cycle.
                *m_value.as_string = *(in.m_value.as_string);
            } else {
                set_(EmTagValueType::vt_string, new EmStringType(*(in.m_value.as_string)));
            }
            return;
        }
        set_(in.m_type, in.m_value);
    }

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

// The tag value class.
//
// NOTE: we need to have a concrete implementation of value since 'EmTag' and "EmTags" 
//       classes will not support templates.
class EmTagValue: public EmTagValueStruct {
    
    // TODO: handle the Epoch type!
    
public:
    EmTagValue() : EmTagValueStruct(EmTagValueType::vt_undefined) {}
    EmTagValue(EmTagValueType type) : EmTagValueStruct(type) {}
    EmTagValue(EmIntegerType value) : EmTagValueStruct(value) {}
    EmTagValue(float value) : EmTagValueStruct(value) {}
    EmTagValue(double value) : EmTagValueStruct(value) {}
    EmTagValue(EmBoolType value) : EmTagValueStruct(value) {}
    EmTagValue(const char* value) : EmTagValueStruct(new EmStringType(value)) {}
    EmTagValue(const EmStringType& value) : EmTagValueStruct(new EmStringType(value)) {}
    EmTagValue(const EmTagValue& other) : EmTagValueStruct(EmTagValueType::vt_undefined) {
        fromValue_(other);
    }

    // NOTE: keep destructor and class without virtual functions to limit RAM footprint
    ~EmTagValue() {
        clear(MUTEX_MEMBER_VAR0);
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
        return (*this > other) || (*this != other);
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
        return EmTagValueStruct::getType(MUTEX_MEMBER_VAR0);
    }

    EmTagValueUnion getValue() const {
        return EmTagValueStruct::getValue(MUTEX_MEMBER_VAR0);
    }

    void get(EmTagValueType& type, EmTagValueUnion& value) const { 
        return EmTagValueStruct::get(MUTEX_MEMBER_VAR1 type, value);
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
        setType(MUTEX_MEMBER_VAR1 EmTagValueType::vt_undefined);
    }

    EmBoolType asBool() const {
        return (getType() == EmTagValueType::vt_boolean) ? getValue().as_bool : false;
    }
    
    EmIntegerType asInteger() const {
        return (getType() == EmTagValueType::vt_integer) ? getValue().as_integer : 0;
    }

    EmEpochType asEpoch() const {
        return (getType() == EmTagValueType::vt_epoch) ? getValue().as_epoch : 0;
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
        EmTagValueStruct::fromStruct(MUTEX_MEMBER_VAR1 in);
    }

    void toStruct(EmTagValueStruct& out) const {
        out.fromStruct(MUTEX_MEMBER_VAR1 *this);
    }

    void fromStruct(const EmTagValueStruct& in) {
        EmTagValueStruct::fromStruct(MUTEX_MEMBER_VAR1 in);
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
        value = *thisValue.as_string;
        return res;
    }

    template<size_t size>
    EmGetValueResult getValue(EmString<size>& value) const {
        // Get type and value 
        EmTagValueType thisType;
        EmTagValueUnion thisValue;
        get(thisType, thisValue);

        if (thisType != EmTagValueType::vt_string) {
            return EmGetValueResult::failed;
        }
        EmGetValueResult res = (value == thisValue.as_string->c_str())
                               ? EmGetValueResult::succeedEqualValue 
                               : EmGetValueResult::succeedNotEqualValue;
        value.set(thisValue.as_string->c_str());
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
        return EmTagValueStruct::setValue(MUTEX_MEMBER_VAR1 value, forceType);
    }

    bool setValue(int32_t value, bool forceType) {
        return EmTagValueStruct::setValue(MUTEX_MEMBER_VAR1 value, forceType);
    }

    bool setValue(float value, bool forceType) {
        return EmTagValueStruct::setValue(MUTEX_MEMBER_VAR1 value, forceType);
    }

    bool setValue(double value, bool forceType) {
        return EmTagValueStruct::setValue(MUTEX_MEMBER_VAR1 value, forceType);
    }

    bool setValue(const EmStringType& value, bool forceType) {
        return EmTagValueStruct::setValue(MUTEX_MEMBER_VAR1 value, forceType);
    }

    bool setValue(const char* value, bool forceType) {
        return EmTagValueStruct::setValue(MUTEX_MEMBER_VAR1 value, forceType);
    }
    
    bool setValue(const EmTagValue& value, bool forceType) {
        EmTagValueType otherType;
        EmTagValueUnion otherValue;
        value.get(otherType, otherValue);

        EmTagValueType thisType = getType();
        if (!forceType && thisType != otherType && thisType != EmTagValueType::vt_undefined) {
            return false;
        }
        set(MUTEX_MEMBER_VAR1 otherType, otherValue);
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
        toStruct_(out);
    }

    void fromStruct_(const EmTagValueStruct& in) {
        EmTagValueStruct::fromStruct_(in);
    }


    // Member vars
#ifdef EM_MULTITHREAD
    mutable EmMutex m_mutex;
#endif
};


// The abstract tag class that provides synchronizable value identified by a string.
// Tags are syncable and updatable. Sync and Update is called from a tag list on its update. 
class EmTagBase: public EmSyncValue<EmValue<EmTagValue>, EmTagValue>, 
                 public EmUpdatable {
public:
    EmTagBase(EmSyncFlags flags)
     : EmSyncValue<EmValue<EmTagValue>, EmTagValue>(flags) {}
    
    // Base methods to be implemented by derived classes
    virtual const char* getId() const = 0;

    // 'EmValue' interface to be implemented by derived classes
    virtual EmGetValueResult getValue(EmTagValue& value) const = 0;
    virtual bool setValue(const EmTagValue& value) = 0;

    virtual void update() override {
        // Default update doing nothing.
        // This method is called by EmTagList::update.
    }

    // Base operators
    virtual bool operator==(const EmTagBase& other) const {
        EmTagValue thisValue, otherValue;
        getValue(thisValue);
        other.getValue(otherValue);
        return match(*this, other) && thisValue == otherValue;
    }

    virtual bool operator!=(const EmTagBase& other) const {
        return !(*this == other);
    }

    // It makes no sense to have the = operator since setting right Tag might fail if of different type.
    EmTagBase& operator=(const EmTagBase& other) = delete;

    // Custom comparison function for EmList
    static bool match(const EmTagBase& item1, const EmTagBase& item2) {
        return strcmp(item1.getId(), item2.getId()) == 0;
    }

    // Convenience getValue overloads
    template<typename T>
    EmGetValueResult getValue(T& value) const {
        EmTagValue v;
        getValue(v);
        return v.getValue<T>(value);
    }

    EmGetValueResult getValue(EmStringType& value) const {
        EmTagValue v;
        getValue(v);
        return v.getValue(value);
    }

    template<size_t size>
    EmGetValueResult getValue(EmString<size>& value) const {
        EmTagValue v;
        getValue(v);
        return v.getValue(value);
    }

    // Convenience setValue overloads
    virtual bool setValue(const bool value, bool forceType) {
        EmTagValue v;
        getValue(v);
        return v.setValue(value, forceType);
    }
    virtual bool setValue(int32_t value, bool forceType) {
        EmTagValue v;
        getValue(v);
        return v.setValue(value, forceType);
    }
    virtual bool setValue(float value, bool forceType) {
        EmTagValue v;
        getValue(v);
        return v.setValue(value, forceType);
    }
    virtual bool setValue(double value, bool forceType) {
        EmTagValue v;
        getValue(v);
        return v.setValue(value, forceType);
    }
    virtual bool setValue(const EmStringType& value, bool forceType) {
        EmTagValue v;
        getValue(v);
        return v.setValue(value, forceType);
    }
    virtual bool setValue(const char* value, bool forceType) {
        EmTagValue v;
        getValue(v);
        return v.setValue(value, forceType);
    }

    template<typename T>
    T as() const {
        T value;
        EmTagValue tagVal;
        if (getValue(tagVal) != EmGetValueResult::failed) {
            return tagVal.as<T>();
        }
        return T();
    }
};

class EmTagsAdd;

// A tag implementation. 
class EmTag: public EmTagBase {
protected:
    const char* m_id;
    EmTagValue m_value;

public:
    EmTag(const char* id, EmSyncFlags flags)
      : EmTagBase(flags), m_id(id) {}

    EmTag(const char* id, EmSyncFlags flags, EmTagsAdd& tags);

    EmTag(const char* id, 
          const EmTagValue& initValue,
          EmSyncFlags flags)
      : EmTagBase(flags), 
        m_id(id), 
        m_value(initValue) {}

    EmTag(const char* id, 
          bool initValue,
          EmSyncFlags flags)
      : EmTagBase(flags), 
        m_id(id), 
        m_value(initValue) {}

    EmTag(const char* id, 
          int32_t initValue,
          EmSyncFlags flags)
      : EmTagBase(flags), 
        m_id(id), 
        m_value(initValue) {}

    EmTag(const char* id, 
          double initValue,
          EmSyncFlags flags)
      : EmTagBase(flags), 
        m_id(id), 
        m_value(initValue) {}

    EmTag(const char* id, 
          const char* initValue,
          EmSyncFlags flags)
      : EmTagBase(flags), 
        m_id(id), 
        m_value(initValue) {}

    EmTag(const char* id, 
          const EmStringType& initValue,
          EmSyncFlags flags)
      : EmTagBase(flags), 
        m_id(id), 
        m_value(initValue) {}

 
    virtual const char* getId() const override { return m_id; }

    using EmTagBase::getValue;
    using EmTagBase::setValue;

    virtual EmTagValue getValue() const { return m_value; };

    virtual EmGetValueResult getValue(EmTagValue& value) const override {
        EmGetValueResult res = (value == m_value) ? EmGetValueResult::succeedEqualValue
                                                  : EmGetValueResult::succeedNotEqualValue;
        if (res == EmGetValueResult::succeedNotEqualValue) {
            value = m_value;
        }
        return res;
    }

    virtual bool setValue(const EmTagValue& value) override {
        m_value = value;
        return true;
    }
};


// This class provides 'EmTagBase' plus an 'onSetValue' callback.
template<EmOnSetValueCallbackType<EmTagBase, EmTagValue> OnSetValue>
class EmTagBaseEx: public EmValueEx<EmTagBase, EmTagBase, EmTagValue, OnSetValue> {
public:
    using EmValueEx<EmTagBase, EmTagBase, EmTagValue, OnSetValue>::EmValueEx;
};


// This class provides 'EmTag' plus an 'onSetValue' callback.
template<EmOnSetValueCallbackType<EmTag, EmTagValue> OnSetValue>
class EmTagEx: public EmValueEx<EmTag, EmTag, EmTagValue, OnSetValue> {
public:
    using EmValueEx<EmTag, EmTag, EmTagValue, OnSetValue>::EmValueEx;
};


// A group of tags with the same ID that are synchronized together.
//
// Tags are added to a 'EmTags' object by grouping tags with the same 'id'.
// Tags with same 'id' are synchronized (i.e. will get same value) on 
// each 'EmTags::update' call.
class EmTagSyncGroupBase: public EmUpdatable {
    public: 
    EmTagSyncGroupBase() = default;
    virtual ~EmTagSyncGroupBase() = default;

    // Base methods to be implemented by derived classes
    virtual const char* getId() const = 0; 

    static bool match(const EmTagSyncGroupBase& item1, const EmTagSyncGroupBase& item2) {
        return strcmp(item1.getId(), item2.getId()) == 0;
    }
};


// A basic concrete implementation of 'EmTagSyncGroupBase' class.
class EmTagSyncGroup: public EmTagSyncGroupBase, 
                      public EmSyncValues<EmTagBase, EmTagValue> {
protected:
    EmList<EmTagBase> m_tagList;

public:
    EmTagSyncGroup() : m_tagList(&EmTagBase::match) {}
    virtual ~EmTagSyncGroup() = default;

    virtual const char* getId() const override { 
        const EmTagBase* first = m_tagList.first();
        return first ? first->getId() : nullptr;
    }

    virtual EmIterator<EmTagBase>* iterator() {
        return new EmListIterator<EmTagBase>(m_tagList);
    }

    virtual void update() override {
        for(auto& item : m_tagList) {
            item.update();
        }
    }

    virtual void add(EmTagBase& tag) {
        m_tagList.appendUnowned(tag);
    }

    virtual size_t count() const {
        return m_tagList.count();
    } 
};

// Abstract class used to define a tag list that allows adding tags.
class EmTagsAdd {
public:    
    virtual void add(EmTagBase& tag) = 0;
    virtual void add(EmTagBase& tag, EmTagSyncGroup*& group) = 0;
};

// This class holds a list of tags. Each tag with same id is considered as a group that
// will be synchronized on each 'update'.
class EmTags: public EmTagsAdd, public EmUpdatable {
public:
    EmTags() : m_groups(&EmTagSyncGroupBase::match) {}
    virtual ~EmTags() {
        clear();
    }

    virtual void clear() {
        m_groups.clear();
    }

    virtual void update() override {
        // Do the groups synch and update
        for(auto& group : m_groups) {
            // Synchronize group tags (i.e. setting to same value)
            static_cast<EmTagSyncGroup&>(group).doSync();
            // Call 'update' for each tag within this group 
            group.update();
        }
    }

    virtual size_t count() const { return m_groups.count(); }
    
    virtual void add(EmTagBase& tag) override { 
        EmTagSyncGroup* group;
        add(tag, group);
    }

    virtual void add(EmTagBase& tag, EmTagSyncGroup*& group) override {
        // Create a temporary group to search for an existing one.
        EmTagSyncGroupSearch searchGroup(tag.getId());
        group = static_cast<EmTagSyncGroup*>(m_groups.find(searchGroup));
        if (!group) {
            group = new EmTagSyncGroup();
            m_groups.append(group, true); // List takes ownership
        }
        group->add(tag);
    }

    // Convenience add overloads to add multiple tag pointers at once.
    // NOTE: the list MUST end with a nullptr.
    virtual void add(EmTagBase* tag, ...) {
        va_list args;
        va_start(args, tag);
        add(tag, args);
        va_end(args);
    }

    virtual void add(EmTagBase* tag, va_list args) {
        EmTagBase* pTag = tag;
        do {
            add(*pTag);
        } while ((pTag = va_arg(args, EmTagBase*)) != nullptr);
    }

    EmTagSyncGroup* find(const char* tagId) const {
        EmTagSyncGroupBase* pGroup = m_groups.find(EmTagSyncGroupSearch(tagId));
        return static_cast<EmTagSyncGroup*>(pGroup);
    }

    // Convenience getValue overloads
    virtual EmGetValueResult getValue(const char* tagId, bool& value) const {
        return getValue_<bool>(tagId, value);
    }

    virtual EmGetValueResult getValue(const char* tagId, int32_t& value) const {
        return getValue_<int32_t>(tagId, value);
    }

    virtual EmGetValueResult getValue(const char* tagId, float& value) const {
        return getValue_<float>(tagId, value);
    }

    virtual EmGetValueResult getValue(const char* tagId, double& value) const {
        return getValue_<double>(tagId, value);
    }

    virtual EmGetValueResult getValue(const char* tagId, EmStringType& value) const {
        return getValue_<EmStringType>(tagId, value);
    }

    virtual EmGetValueResult getValue(const char* tagId, EmTagValue& value) const {
        return getValue_<EmTagValue>(tagId, value);
    }

    // Convenience setValue overloads
    virtual bool setValue(const char* tagId, bool value, bool doSync) {
        return setValue_<bool>(tagId, value, doSync);
    }

    virtual bool setValue(const char* tagId, int32_t value, bool doSync) {
        return setValue_<int32_t>(tagId, value, doSync);
    }

    virtual bool setValue(const char* tagId, float value, bool doSync) {
        return setValue_<float>(tagId, value, doSync);
    }

    virtual bool setValue(const char* tagId, double value, bool doSync) {
        return setValue_<double>(tagId, value, doSync);
    }

    virtual bool setValue(const char* tagId, const EmStringType& value, bool doSync) {
        return setValue_<EmStringType>(tagId, value, doSync);
    }

    virtual bool setValue(const char* tagId, const char* value, bool doSync) {
        return setValue_<const char*>(tagId, value, doSync);
    }

    virtual bool setValue(const char* tagId, const EmTagValue& value, bool doSync) {
        return setValue_<EmTagValue>(tagId, value, doSync);
    }

protected: 
    // A "dummy" class used to seach of existing groups.
    class EmTagSyncGroupSearch: public EmTagSyncGroupBase {
    public: 
        EmTagSyncGroupSearch(const char* id) : m_id(id) {}
        virtual const char* getId() const override {
            return m_id;
        } 

        virtual void update() override {} // Nothing to do in a search group

    protected:
        const char* m_id; 
    };

    template<typename T>
    EmGetValueResult getValue_(const char* tagId, T& value) const {
        EmTagSyncGroup* pTagGroup = find(tagId);
        if (pTagGroup == nullptr) {
            return EmGetValueResult::failed;
        } 
        EmTagValue tagValue(value);
        EmGetValueResult res = pTagGroup->getValue(tagValue);
        if (res == EmGetValueResult::succeedNotEqualValue) {
            tagValue.getValue(value);
        }
        return res;
    }

    template<typename T>
    bool setValue_(const char* tagId, const T& value, bool doSync) {
        EmTagSyncGroup* pTagGroup = find(tagId);
        if (pTagGroup == nullptr) {
            return false;
        } 
        return pTagGroup->setValue(value, doSync);
    }

    EmList<EmTagSyncGroupBase> m_groups;
};

inline EmTag::EmTag(const char* id, EmSyncFlags flags, EmTagsAdd& tags)
  : EmTagBase(flags), m_id(id) {
    tags.add(*this);
}
#endif // EM_STD_LIB

#endif // _EM_TAG_H__
