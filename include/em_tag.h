#ifndef _EM_TAG_H__
#define _EM_TAG_H__

#include "em_defs.h"

#ifdef EM_STD_LIB // Need standard library 

#include "em_tag_value.h"


// The abstract tag class that provides synchronizable value identified by a string id.
// Tags are syncable and updatable. Sync and Update is called from a tag list on its update. 
class EmTagBase: public EmSyncValue<EmValue<EmTagValue>, EmTagValue>, 
                 public EmUpdatable {
public:
    EmTagBase(EmSyncFlags flags)
     : EmSyncValue<EmValue<EmTagValue>, EmTagValue>(flags) {}

    // No default copy and move constructor and assignment
    EmTagBase(const EmTagBase& other) = delete;
    EmTagBase(EmTagBase&& other) = delete;
    EmTagBase& operator=(const EmTagBase& other) = delete;
    EmTagBase& operator=(EmTagBase&& other) = delete;

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
        if (!match(*this, other)) {
            return false;
        }
        EmTagValue thisValue, otherValue;
        getValue(thisValue);
        other.getValue(otherValue);
        return thisValue == otherValue;
    }

    virtual bool operator!=(const EmTagBase& other) const {
        return !(*this == other);
    }

    // Custom comparison function for EmList
    static bool match(const EmTagBase& item1, const EmTagBase& item2) {
        return strcmp(item1.getId(), item2.getId()) == 0;
    }

    // Convenience getValue overloads
    template<typename T>
    EmGetValueResult getValue(T& value) const {
        EmTagValue v;
        EmGetValueResult res = getValue(v);
        if (res != EmGetValueResult::failed) {
            return v.getValue(value);
        }
        return res;
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
          bool initValue,
          EmSyncFlags flags)
      : EmTagBase(flags), 
        m_id(id), 
        m_value(initValue) {}

    EmTag(const char* id, 
          const EmTagValue& initValue,
          EmSyncFlags flags)
      : EmTagBase(flags), 
        m_id(id), 
        m_value(initValue) {}

    EmTag(const char* id, 
          EmIntType initValue,
          EmSyncFlags flags)
      : EmTagBase(flags), 
        m_id(id), 
        m_value(initValue) {}

    EmTag(const char* id, 
          EmUIntType initValue,
          EmSyncFlags flags)
      : EmTagBase(flags), 
        m_id(id), 
        m_value(initValue) {}

    EmTag(const char* id, 
          int initValue,
          EmSyncFlags flags)
      : EmTagBase(flags), 
        m_id(id), 
        m_value(initValue) {}

    EmTag(const char* id, 
          unsigned int initValue,
          EmSyncFlags flags)
      : EmTagBase(flags), 
        m_id(id), 
        m_value(initValue) {}

    EmTag(const char* id, 
          float initValue,
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

 
    virtual const char* getId() const override { return m_id; }

    using EmTagBase::getValue;
    using EmTagBase::setValue;

    virtual EmGetValueResult getValue(EmTagValue& value) const override {
        EmGetValueResult res = (value == m_value) ? EmGetValueResult::succeedEqualValue
                                                  : EmGetValueResult::succeedNotEqualValue;
        if (res == EmGetValueResult::succeedNotEqualValue) {
            if (!value.setValue(m_value)) {
                return EmGetValueResult::failed;
            }
        }
        return res;
    }

    virtual bool setValue(const EmTagValue& value) override {
        return m_value.setValue(value);
    }

    EmBoolType asBool() const {
        return m_value.asBool();
    }

    EmIntType asInt() const {
        return m_value.asInt();
    }

    EmUIntType asUInt() const {
        return m_value.asUInt();
    }

    EmRealType asReal() const {
        return m_value.asReal();
    }

    EmEpochType asEpoch() const {
        return m_value.asEpoch();
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

    virtual EmGetValueResult getValue(const char* tagId, EmTagValue& value) {
        // Find the group for the given tagId
        EmTagSyncGroup* pTagGroup = find(tagId);
        if (pTagGroup == nullptr) {
            return EmGetValueResult::failed;
        } 
        // Retrieve the current value from the group and then get the value for the specific type.
        return pTagGroup->getValue(value);
    }    

    virtual bool setValue(const char* tagId, const EmTagValue& value, bool doSync) {
        EmTagSyncGroup* pTagGroup = find(tagId);
        if (pTagGroup == nullptr) {
            return false;
        } 
        bool res = pTagGroup->setValue(value, false);
        if (res && doSync) {
            return pTagGroup->doSync();
        }
        return res;
    }

/* TODO: review if those methods are needed
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

    virtual EmGetValueResult getValue(const char* tagId, EmStringBase& value) const {
        return getValue_<EmStringBase>(tagId, value);
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

    virtual bool setValue(const char* tagId, const EmStringBase& value, bool doSync) {
        return setValue_<EmStringBase>(tagId, value, doSync);
    }

    virtual bool setValue(const char* tagId, const char* value, bool doSync) {
        return setValue_<const char*>(tagId, value, doSync);
    }

    virtual bool setValue(const char* tagId, const EmTagValue& value, bool doSync) {
        return setValue_<EmTagValue>(tagId, value, doSync);
    }
*/
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

    /* TODO: review if those methods are needed
    template<typename T>
    EmGetValueResult getValue_(const char* tagId, T& value) const {
        // Find the group for the given tagId
        EmTagSyncGroup* pTagGroup = find(tagId);
        if (pTagGroup == nullptr) {
            return EmGetValueResult::failed;
        } 
        // Retrieve the current value from the group and then get the value for the specific type.
        EmTagValue currentValue;
        EmGetValueResult res = pTagGroup->getValue(currentValue);
        if (res == EmGetValueResult::failed) {
            return res;
        }
        return currentValue.getValue(value);
    }

    template<typename T>
    bool setValue_(const char* tagId, const T& value, bool doSync) {
        EmTagSyncGroup* pTagGroup = find(tagId);
        if (pTagGroup == nullptr) {
            return false;
        } 
        if (pTagGroup->setValue(value, false)) {
            if (doSync) {
                return pTagGroup->doSync();
            }
            return true;
        }
        return false;
    }
    */
    EmList<EmTagSyncGroupBase> m_groups;
};

inline EmTag::EmTag(const char* id, EmSyncFlags flags, EmTagsAdd& tags)
  : EmTagBase(flags), m_id(id) {
    tags.add(*this);
}
#endif // EM_STD_LIB

#endif // _EM_TAG_H__
