#ifndef _EM_TAG_H__
#define _EM_TAG_H__

#include "em_defs.h"

#ifdef EM_STD_LIB // Need standard library 

#include "em_list.h"
#include "em_value_sync.h"
#include "em_tag_value.h"


// The tag hold in a tag list
class EmTagElement: public EmUpdatable,
                    public EmSyncValue<EmValue<EmTagElement>, EmTagElement> {
public:    
    virtual const char* getId() const = 0;
    virtual EmTagValueType getType() const = 0;
    virtual bool operator ==(const EmTagElement& other) const = 0;
    
    // EmValue overrides
    virtual EmGetValueResult getValue(EmTagElement& value) const = 0;
    virtual bool setValue(const EmTagElement& value) = 0;

    // Default update doing nothing.
    // This method is called by EmTagList::update.
    virtual void update() override {
    }
    
    // Used to sync tags with same id when list 'update' is called
    template<typename T>
    EmGetValueResult syncValue(EmTagValue<T>& dest, EmTagValue<T>& source) const {
        return source.getValue(dest);
    }

    // Custom comparison function for EmList
    static bool match(EmTagElement& item1, EmTagElement& item2) {
        return strcmp(item1.getId(), item2.getId()) == 0;
    }
    static bool match(EmTagElement* const & item1, EmTagElement* const & item2) {
        return strcmp(item1->getId(), item2->getId()) == 0;
    }
};


// Forward declaration
class EmTagSyncGroup;

// This is the EmSyncValues class used for the EmTagElement.
class EmSyncTagValues: public EmSyncValues<EmTagElement, EmTagElement> {
    friend class EmTagSyncGroup;
public:
    EmSyncTagValues(EmTagElement& value)
     : m_currentValue(value) {}

protected:    
    virtual EmTagElement& getCurrentValue_() const override {
        return m_currentValue;
    }

    virtual bool setCurrentValue_(const EmTagElement& value) override {
        m_currentValue = value;
    }

    EmTagElement& m_currentValue;
};

// The abstract tag class that provides synchronizable value identified by a string id.
// Tags are syncable and updatable. Sync and Update is called from a tag list on its update. 
template<typename T>
class EmTagBase: public EmTagSyncElement {
public:
    EmTagBase(EmSyncFlags flags)
     : EmTagSyncElement(flags) {}

    // No default copy and move constructor and assignment
    EmTagBase(const EmTagBase& other) = delete;
    EmTagBase(EmTagBase&& other) = delete;
    EmTagBase& operator=(const EmTagBase& other) = delete;
    EmTagBase& operator=(EmTagBase&& other) = delete;

    // 'EmValue' interface to be implemented by derived classes
    virtual EmGetValueResult getValue(EmTagValue<T>& value) const = 0;
    virtual bool setValue(const EmTagValue<T>& value) = 0;
    
    // Convenience methods
    virtual EmTagValue<T>& getValue() const = 0;

    virtual EmGetValueResult getValue(T& value) const {
        return getValue().getValue(value);
    }
    
    virtual bool setValue(const T& value) {
        return setValue(EmTagValue<T>(value));
    }

    // Base operators
    virtual bool operator==(const EmTagBase& other) const override {
        if (!match(*this, other)) {
            return false;
        }
        return this->getValue() == other.getValue();
    }

    virtual bool operator!=(const EmTagBase& other) const {
        return !(*this == other);
    }
};

// Forward declaration
class EmTagSyncGroup;

// Abstract class used to define a tag list that allows adding tags.
class EmTagsAdd {
public:    
    virtual void add(EmTagElement& tag) = 0;
    virtual void add(EmTagElement& tag, EmTagSyncGroup*& group) = 0;
};

// The simple EmTagBase implementation holding the string id and the tag value. 
template<typename T>
class EmTag: public EmTagBase<T> {
protected:
    const char* m_id;
    EmTagValue<T>& m_value;

public:
    EmTag(const char* id, 
          EmTagValue<T>& initValue,
          EmSyncFlags flags)
      : EmTagBase<T>(flags), 
        m_id(id), 
        m_value(initValue) {}

    EmTag(const char* id, 
          EmTagValue<T>& initValue,
          EmSyncFlags flags, 
          EmTagsAdd& tags);
    
    // EmTagElement implementation
    virtual const char* getId() const override { return m_id; }
    virtual EmTagValueType getType() const override { return m_value.getType(); }


    using EmTagBase<T>::getValue;
    using EmTagBase<T>::setValue;

    virtual EmTagValue<T>& getValue() const override {
        return m_value;
    }

    virtual EmGetValueResult getValue(EmTagValue<T>& value) const override {
        EmGetValueResult res = (value == m_value) ? EmGetValueResult::succeedEqualValue
                                                  : EmGetValueResult::succeedNotEqualValue;
        if (res == EmGetValueResult::succeedNotEqualValue) {
            if (!value.setValue(m_value)) {
                return EmGetValueResult::failed;
            }
        }
        return res;
    }

    virtual bool setValue(const EmTagValue<T>& value) override {
        return m_value.setValue(value);
    }
};


// This class provides 'EmTagBase' plus an 'onSetValue' callback.
template<typename T, EmOnSetValueCallbackType<EmTagBase<T>, EmTagValue<T>> OnSetValue>
class EmTagBaseEx: public EmValueEx<EmTagBase<T>, EmTagBase<T>, EmTagValue<T>, OnSetValue> {
public:
    using EmValueEx<EmTagBase<T>, EmTagBase<T>, EmTagValue<T>, OnSetValue>::EmValueEx;
};


// This class provides 'EmTag' plus an 'onSetValue' callback.
template<typename T, EmOnSetValueCallbackType<EmTag<T>, EmTagValue<T>> OnSetValue>
class EmTagEx: public EmValueEx<EmTag<T>, EmTag<T>, EmTagValue<T>, OnSetValue> {
public:
    using EmValueEx<EmTag<T>, EmTag<T>, EmTagValue<T>, OnSetValue>::EmValueEx;
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
                      public EmSyncTagValues {
protected:
    EmList<EmTagSyncElement> m_tagList;
    EmListIterator<EmTagSyncElement> m_iterator;

    static bool match(const EmTagSyncElement& item1, const EmTagSyncElement& item2) {
        return strcmp(item1.getId(), item2.getId()) == 0;
    }

public:
    EmTagSyncGroup(EmTagElement& currentElement)
     : EmTagSyncGroupBase(), 
       EmSyncTagValues(currentElement),
       m_tagList(&EmTagSyncGroup::match),
       m_iterator(m_tagList) {}

    virtual ~EmTagSyncGroup() = default;

    virtual const char* getId() const override { 
        EmTagSyncElement* first = m_tagList.first();
        return first ? first->getId() : nullptr;
    }

    virtual EmIterator<EmTagSyncElement>& iterator(bool reset) override {
        if (reset) {
            m_iterator.reset();
        }
        return m_iterator;
    }

    virtual void update() override {
        for(auto& item : m_tagList) {
            item.update();
        }
    }

    virtual void add(EmTagSyncElement& tag) {
        m_tagList.appendUnowned(tag);
    }

    virtual size_t count() const {
        return m_tagList.count();
    } 
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

    // This will synch all the tags by its 'id' (tags with same id will be synched)
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
    
    virtual void add(EmTagElement& tag) override { 
        EmTagSyncGroup* group;
        add(tag, group);
    }

    virtual void add(EmTagElement& tag, EmTagSyncGroup*& group) override {
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
    virtual void add(EmTagElement* tag, ...) {
        va_list args;
        va_start(args, tag);
        add(tag, args);
        va_end(args);
    }

    virtual void add(EmTagElement* tag, va_list args) {
        EmTagElement* pTag = tag;
        do {
            add(*pTag);
        } while ((pTag = va_arg(args, EmTagElement*)) != nullptr);
    }

    EmTagSyncGroup* find(const char* tagId) const {
        EmTagSyncGroupBase* pGroup = m_groups.find(EmTagSyncGroupSearch(tagId));
        return static_cast<EmTagSyncGroup*>(pGroup);
    }

    template<typename T>
    EmGetValueResult getValue(const char* tagId, EmTagValue<T>& value) {
        // Find the group for the given tagId
        EmTagSyncGroup* pTagGroup = find(tagId);
        if (pTagGroup == nullptr) {
            return EmGetValueResult::failed;
        } 
        // Retrieve the current value from the group and then get the value for the specific type.
        return pTagGroup->getValue(value);
    }    

    template<typename T>
    bool setValue(const char* tagId, const EmTagValue<T>& value, bool doSync) {
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

    EmList<EmTagSyncGroupBase> m_groups;
};


template<typename T>
inline EmTag<T>::EmTag(const char* id, EmTagValue<T>& value, EmSyncFlags flags, EmTagsAdd& tags)
  : EmTag<T>(id, value,flags) {
    tags.add(*this);
}

#endif // EM_STD_LIB
#endif // _EM_TAG_H__
