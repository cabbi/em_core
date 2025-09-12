#ifndef _EM_TAG_H_
#define _EM_TAG_H_

#include "em_sync_value.h"
#include "em_list.h"
#include <WString.h>

// The tag value type
enum class EmTagValueType: uint8_t {
    vt_undefined = 0,
    vt_boolean = 1,
    vt_integer = 2,
    vt_real = 3,
    vt_string = 4
};


// The tag value bytes union
union EmTagValueUnion {
    bool as_bool;
    int32_t as_integer;
    double as_real;
    String* as_string;
};

// The tag value bytes structure used to read and write an EmTagValue object in case 'EmTagValue' 
// will have virtual functions in the future.
struct EmTagValueStruct {
    EmTagValueStruct() 
     : m_type(EmTagValueType::vt_undefined), m_value{0} {}

    EmTagValueStruct(EmTagValueType type, 
                     EmTagValueUnion value = {0})
     : m_type(type), m_value(value) {}

    EmTagValueType m_type;
    EmTagValueUnion m_value;
};

// The tag value class.
//
// NOTE: we need to have a concrete implementation of value since 'EmTag' and "EmTags" 
//       classes will not support templates.
class EmTagValue: protected EmTagValueStruct {
public:
    EmTagValue() : EmTagValueStruct(EmTagValueType::vt_undefined) {
    }
    EmTagValue(int32_t value) : EmTagValueStruct(EmTagValueType::vt_integer) {
        m_value.as_integer = value;
    }
    EmTagValue(float value) : EmTagValueStruct(EmTagValueType::vt_real) {
        m_value.as_real = value;
    }
    EmTagValue(double value) : EmTagValueStruct(EmTagValueType::vt_real) {
        m_value.as_real = value;
    }
    EmTagValue(bool value) : EmTagValueStruct(EmTagValueType::vt_boolean) {
        m_value.as_bool = value;
    }
    EmTagValue(const char* value) : EmTagValueStruct(EmTagValueType::vt_string) {
        m_value.as_string = new String(value);
    }
    EmTagValue(const String& value) : EmTagValueStruct(EmTagValueType::vt_string) {
        m_value.as_string = new String(value);
    }
    EmTagValue(const EmTagValue& other) : EmTagValueStruct(EmTagValueType::vt_undefined) {
        copyFrom_(other);
    }

    // NOTE: keep destructor and class without virtual functions to limit RAM footprint
    ~EmTagValue() {
        clear_();
    }   

    bool isSameType(const EmTagValue& other) const {
        return m_type == other.m_type;
    }

    EmTagValue& operator=(const EmTagValue& other) {
        if (this != &other) {
            clear_();
            copyFrom_(other);
        }
        return *this;
    }
    
    bool operator==(const EmTagValue& other) const {
        if (m_type != other.m_type) {
            return false;
        }
        switch (m_type) {
            case EmTagValueType::vt_boolean:
                return m_value.as_bool == other.m_value.as_bool;
            case EmTagValueType::vt_integer:
                return m_value.as_integer == other.m_value.as_integer;
            case EmTagValueType::vt_real:
                return m_value.as_real == other.m_value.as_real;
            case EmTagValueType::vt_string:
                // Ensure both pointers are valid before dereferencing
                if (m_value.as_string && other.m_value.as_string) {
                    return *m_value.as_string == *other.m_value.as_string;
                }
                return m_value.as_string == other.m_value.as_string; // Both are nullptr
            default: // vt_undefined
                return true; // Two undefined values are considered equal
        }
    }

    bool operator!=(const EmTagValue& other) const {
        return !(*this == other);
    }

    EmTagValueType getType() const { return m_type; }

    EmGetValueResult getValue(bool& value) const {
        if (m_type != EmTagValueType::vt_boolean) {
            return EmGetValueResult::failed;
        }
        EmGetValueResult res = (value == m_value.as_bool) ? EmGetValueResult::succeedEqualValue 
                                                          : EmGetValueResult::succeedNotEqualValue;
        value = m_value.as_bool;
        return res;
    }

    void toStruct(EmTagValueStruct& out) const {
        out.m_type = m_type;
        out.m_value = m_value;
    }

    void fromStruct(const EmTagValueStruct& in) {
        clear_();
        m_type = in.m_type;
        m_value = in.m_value;
        if (m_type == EmTagValueType::vt_string && m_value.as_string != nullptr) {
            // Deep copy the string to avoid dangling pointer issues
            m_value.as_string = new String(*m_value.as_string);
        }
    }

    EmGetValueResult getValue(int32_t& value) const {
        if (m_type != EmTagValueType::vt_integer) {
            return EmGetValueResult::failed;
        }
        EmGetValueResult res = (value == m_value.as_integer) ? EmGetValueResult::succeedEqualValue
                                                             : EmGetValueResult::succeedNotEqualValue;
        value = m_value.as_integer;
        return res;
    }

    EmGetValueResult getValue(float& value) const {
        if (m_type != EmTagValueType::vt_real)  {
            return EmGetValueResult::failed;
        }
        EmGetValueResult res = (value == static_cast<float>(m_value.as_real)) ? EmGetValueResult::succeedEqualValue 
                                                                              : EmGetValueResult::succeedNotEqualValue;
        value = static_cast<float>(m_value.as_real);
        return res;
    }

    EmGetValueResult getValue(double& value) const {
        if (m_type != EmTagValueType::vt_real)  {
            return EmGetValueResult::failed;
        }
        EmGetValueResult res = (value == m_value.as_real) ? EmGetValueResult::succeedEqualValue 
                                                          : EmGetValueResult::succeedNotEqualValue;
        value = m_value.as_real;
        return res;
    }

    EmGetValueResult getValue(String& value) const {
        if (m_type != EmTagValueType::vt_string) {
            return EmGetValueResult::failed;
        }
        EmGetValueResult res = (value == *m_value.as_string) ? EmGetValueResult::succeedEqualValue 
                                                             : EmGetValueResult::succeedNotEqualValue;
        value = *m_value.as_string;
        return res;
    }

    bool setValue(bool value, bool forceType) {
        if (!forceType && m_type != EmTagValueType::vt_boolean && m_type != EmTagValueType::vt_undefined) {
            return false;
        }
        clear_(); // Clear only if we are changing type or it's a string
        m_type = EmTagValueType::vt_boolean;
        m_value.as_bool = value;
        return true;
    }

    bool setValue(int32_t value, bool forceType) {
        if (!forceType && m_type != EmTagValueType::vt_integer && m_type != EmTagValueType::vt_undefined) {
            return false;
        }
        clear_();
        m_type = EmTagValueType::vt_integer;
        m_value.as_integer = value;
        return true;
    }

    bool setValue(float value, bool forceType) {
        if (!forceType && m_type != EmTagValueType::vt_real && m_type != EmTagValueType::vt_undefined) {
            return false;
        }
        clear_();
        m_type = EmTagValueType::vt_real;
        m_value.as_real = value;
        return true;
    }

    bool setValue(double value, bool forceType) {
        if (!forceType && m_type != EmTagValueType::vt_real && m_type != EmTagValueType::vt_undefined) {
            return false;
        }
        clear_();
        m_type = EmTagValueType::vt_real;
        m_value.as_real = value;
        return true;
    }

    bool setValue(const String& value, bool forceType) {
        if (m_type == EmTagValueType::vt_string) {
            // Already a string, just reassign the value to avoid delete/new cycle.
            *m_value.as_string = value;
            return true;
        }
        if (!forceType && m_type != EmTagValueType::vt_undefined) {
            return false;
        }
        clear_();
        m_type = EmTagValueType::vt_string;
        m_value.as_string = new String(value);
        return true;
    }

    bool setValue(const char* value, bool forceType) {
        return setValue(String(value), forceType);
    }

protected:
    void clear_() {
        if (m_type == EmTagValueType::vt_string) {
            delete m_value.as_string;
        }
        m_type = EmTagValueType::vt_undefined;
        m_value.as_integer = 0; // Zero out the union
    }

    void copyFrom_(const EmTagValue& other) {
        m_type = other.m_type;
        switch (m_type) {
            case EmTagValueType::vt_boolean:
                m_value.as_bool = other.m_value.as_bool;
                break;
            case EmTagValueType::vt_integer:
                m_value.as_integer = other.m_value.as_integer;
                break;
            case EmTagValueType::vt_real:
                m_value.as_real = other.m_value.as_real;
                break;
            case EmTagValueType::vt_string:
                m_value.as_string = new String(*other.m_value.as_string);
                break;
            case EmTagValueType::vt_undefined:
            default:
                m_value.as_integer = 0;
                break;
        }
    }
};

// A tag iterface that provides synchronizable value identified by a string.
class EmTagInterface: public EmSyncValue<EmValue<EmTagValue>, EmTagValue> {
public:
    EmTagInterface(EmSyncFlags flags)
     : EmSyncValue<EmValue<EmTagValue>, EmTagValue>(flags) {}
    
    // Base methods to be implemented by derived classes
    virtual const char* getId() const = 0;
    virtual EmTagValue getValue() const = 0;

    // 'EmValue' interface to be implemented by derived classes
    virtual EmGetValueResult getValue(EmTagValue& value) const = 0;
    virtual bool setValue(const EmTagValue& value) = 0;

    // Base operators
    virtual bool operator==(const EmTagInterface& other) const {
        return match(*this, other) && getValue() == other.getValue();
    }

    virtual bool operator!=(const EmTagInterface& other) const {
        return !(*this == other);
    }

    // It makes no sense to have the = operator since setting right Tag might fail if of different type.
    EmTagInterface& operator=(const EmTagInterface& other) = delete;

    // Custom comparison function for EmList
    static bool match(const EmTagInterface& item1, const EmTagInterface& item2) {
        return strcmp(item1.getId(), item2.getId()) == 0;
    }

    // Convenience getValue overloads
    virtual EmGetValueResult getValue(bool& value) const {
        return getValue().getValue(value);
    }
    virtual EmGetValueResult getValue(int32_t& value) const {
        return getValue().getValue(value);
    }
    virtual EmGetValueResult getValue(float& value) const {
        return getValue().getValue(value);
    }
    virtual EmGetValueResult getValue(double& value) const {
        return getValue().getValue(value);
    }
    virtual EmGetValueResult getValue(String& value) const {
        return getValue().getValue(value);
    }

    // Convenience setValue overloads
    virtual bool setValue(const bool value, bool forceType) {
        return getValue().setValue(value, forceType);
    }
    virtual bool setValue(int32_t value, bool forceType) {
        return getValue().setValue(value, forceType);
    }
    virtual bool setValue(float value, bool forceType) {
        return getValue().setValue(value, forceType);
    }
    virtual bool setValue(double value, bool forceType) {
        return getValue().setValue(value, forceType);
    }
    virtual bool setValue(const String& value, bool forceType) {
        return getValue().setValue(value, forceType);
    }
    virtual bool setValue(const char* value, bool forceType) {
        return getValue().setValue(value, forceType);
    }
};

// A simple EmTag iterface implementation.
class EmTag: public EmTagInterface {
protected:
    const char* m_id;
    EmTagValue m_value;

public:
    EmTag(const char* id, EmSyncFlags flags)
        : EmTagInterface(flags), m_id(id) {}

    virtual EmTagValue getValue() const { return m_value; };
    virtual const char* getId() const override { return m_id; }

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

// A group of tags with the same ID that are synchronized together.
class EmTagSyncGroupInterface_ {
    public: 
    virtual const char* getId() const = 0; 

    static bool match(const EmTagSyncGroupInterface_& item1, const EmTagSyncGroupInterface_& item2) {
        return strcmp(item1.getId(), item2.getId()) == 0;
    }
};

class EmTagSyncGroupSearch_: public EmTagSyncGroupInterface_ {
public: 
    EmTagSyncGroupSearch_(const char* id) : m_id(id) {}
    virtual const char* getId() const override {
        return m_id;
    } 

protected:
    const char* m_id; 
};

class EmTagSyncGroup_: public EmTagSyncGroupInterface_, 
                       public EmSyncValues<EmTagValue> {
protected:
    EmList<EmTagInterface> m_tagList;

public:
    EmTagSyncGroup_() 
        : m_tagList(&EmTagInterface::match) {
    }

    virtual const char* getId() const override { 
        const EmTagInterface* first = m_tagList.first();
        return first ? first->getId() : nullptr;
    }

    void add(EmTagInterface& tag) {
        m_tagList.appendUnowned(tag);
    }

    virtual bool doSync() override {
        EmListIterator<EmTagInterface> iter(m_tagList);
        EmTagInterface* pItem = nullptr;
        while (iter.next(pItem)) {
            switch (pItem->checkNewValue(this->m_currentValue)) {
                case CheckNewValueResult::valueChanged:
                    // First changed value found: lets write all the others! 
                    return updateToNewValue_(*pItem);
                case CheckNewValueResult::mustReadFailed:
                    // A "must read" value failed to read, cannot proceed with synch!
                    return false;
                case CheckNewValueResult::pendingWrite:
                    // An "old" pending write
                    pItem->doPendingWrite(this->m_currentValue);
                    break;
                case CheckNewValueResult::noChange:
                   break;
            }
        }
        return true;
    }

protected:
    bool updateToNewValue_(EmTagInterface& changedItem) {
        bool res = true;
        EmListIterator<EmTagInterface> iter(m_tagList);
        EmTagInterface* pItem = nullptr;
        while (iter.next(pItem)) {
            if (pItem != &changedItem) {
                if (!pItem->_setCurrentValue(this->m_currentValue)) {
                    res = false;
                }
            }
        }
        return res;
    }
};


// This class holds a list of tags. Each tag with same id is considered as a group that
// will be synchronized on each 'update'.
class EmTags: public EmUpdatable {
public:
    EmTags() : m_groups(&EmTagSyncGroupInterface_::match) {}
    virtual ~EmTags() = default;

    virtual void update() override {
        EmListIterator<EmTagSyncGroupInterface_> iter(m_groups);
        EmTagSyncGroupInterface_* pItem = nullptr;
        while (iter.next(pItem)) {
            static_cast<EmTagSyncGroup_*>(pItem)->doSync();
        }
    }

    virtual size_t count() const { return m_groups.count(); }
    
    virtual void add(EmTagInterface& tag) {
        // Create a temporary group to search for an existing one.
        EmTagSyncGroupSearch_ searchGroup(tag.getId());
        EmTagSyncGroup_* group = static_cast<EmTagSyncGroup_*>(m_groups.find(searchGroup));
        if (!group) {
            group = new EmTagSyncGroup_();
            m_groups.append(group, true); // List takes ownership
        }
        group->add(tag);
    }

    EmTagInterface* find(const char* tagId) const {
        return nullptr; // TODO!
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

    virtual EmGetValueResult getValue(const char* tagId, String& value) const {
        return getValue_<String>(tagId, value);
    }

    // Convenience setValue overloads
    virtual bool setValue(const char* tagId, bool value, bool forceType) {
        return setValue_<bool>(tagId, value, forceType);
    }
    virtual bool setValue(const char* tagId, int32_t value, bool forceType) {
        return setValue_<int32_t>(tagId, value, forceType);
    }
    virtual bool setValue(const char* tagId, float value, bool forceType) {
        return setValue_<float>(tagId, value, forceType);
    }
    virtual bool setValue(const char* tagId, double value, bool forceType) {
        return setValue_<double>(tagId, value, forceType);
    }
    virtual bool setValue(const char* tagId, const String& value, bool forceType) {
        return setValue_<const String>(tagId, value, forceType);
    }
    virtual bool setValue(const char* tagId, const char* value, bool forceType) {
        return setValue_<const char*>(tagId, value, forceType);
    }

protected: 
    template<typename T>
    EmGetValueResult getValue_(const char* tagId, T& value) const {
        EmTagInterface* pTag = find(tagId);
        if (pTag == nullptr) {
            return EmGetValueResult::failed;
        } 
        EmTagValue tagValue(value);
        EmGetValueResult res = pTag->getValue(tagValue);
        if (res == EmGetValueResult::succeedNotEqualValue) {
            tagValue.getValue(value);
        }
        return res;
    }

    template<typename T>
    bool setValue_(const char* tagId, T value, bool forceType) {
        EmTagInterface* pTag = find(tagId);
        if (pTag == nullptr) {
            return false;
        } 
        // TODO: !!! handle 'forceType' !!!
        return pTag->setValue(EmTagValue(value));
    }

    EmList<EmTagSyncGroupInterface_> m_groups;
};

#endif // _EM_TAG_H_
