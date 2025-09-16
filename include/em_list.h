#ifndef __EM_LIST_H__
#define __EM_LIST_H__

#include <stdint.h>
#include "em_iterator.h"

template<class T> class _EmListElement;

// Defines the result of an iteration callback
enum class EmIterResult : int8_t {
    moveNext = 0, // Default is continue iteration by moving to next list value
    stopSucceed,
    stopFailed,
    removeMoveNext,
    removeStopSucceed,
    removeStopFailed,
};

// Forward declarations
template<class T> class EmList;

// List iterator
template<class T>
class EmListIterator : public EmIterator<T> {
public:
    EmListIterator(EmList<T>& list)
        : m_pItem(nullptr),
          m_pFirst(list.m_pFirst),
          m_pNext(nullptr) {}

    operator T*() const { return m_pItem; }
    T* item() const { return m_pItem; }

    // Reset the iterator
    void reset() override {
        m_pItem = nullptr;
        m_pNext = nullptr;
    }

    // Returns true if next item is available or false if iterable is empty or end of iteration is reached
    bool next(T*& pItem) override {
        if (_isBegin()) {
            _copyFrom(m_pFirst);
        } else {
            _copyFrom(m_pNext);
        }
        pItem = m_pItem;
        return pItem != nullptr;
    }

protected:
    bool _isBegin() const {
        return m_pItem == nullptr && m_pNext == nullptr;
    }

    void _copyFrom(class _EmListElement<T>* pElem) {
        if (pElem == nullptr) {
            m_pItem = nullptr;
            m_pNext = nullptr;
        } else {
            m_pItem = pElem->m_pItem;
            m_pNext = pElem->next();
        }
    }

    T* m_pItem;
    class _EmListElement<T>* m_pFirst;
    class _EmListElement<T>* m_pNext;
};

template<class T>
class _EmListElement : public EmListIterator<T> {
    friend class EmList<T>;
    friend class EmListIterator<T>;
private:
    _EmListElement(EmList<T>& list, T* pItem, bool takeOwnership)
        : EmListIterator<T>(list), 
          m_ownsItem(takeOwnership) {
        this->m_pItem = pItem;
        this->m_pNext = nullptr;
    }
    
    // NOTE: keep destructor and class without virtual functions to limit RAM footprint
    ~_EmListElement() {
        if (this->m_pItem != nullptr && m_ownsItem) {
            delete this->m_pItem;
        }
    }

    virtual bool next(T*& pItem) override {
        _EmListElement<T>* pNextElement = this->next();
        pItem = pNextElement != nullptr ? pNextElement->m_pItem : nullptr;
        return pItem != nullptr;
    }

    _EmListElement<T>* next() const {
        return static_cast<_EmListElement<T>*>(this->m_pNext);
    }

    bool m_ownsItem;
};

// Items matching callback prototype
// NOTE: Arduino platform does not have std::functional definition! :()
template<class T> using ItemsMatchCb = bool(*)(const T& item1, const T& item2);
template<class T> using IterationCb = EmIterResult(*)(T& item);
template<class T, class V>
using IterationExCb = EmIterResult(*)(T& item, bool isFirst, bool isLast, V* pUserData);

// Default items matching callback function
template<class T>
inline bool defItemsMatch(const T& item1, const T& item2) {
    return item1 == item2;
}

// List implementation
template<class T>
class EmList {
    friend class EmListIterator<T>;
public:
    EmList(ItemsMatchCb<T> itemsMatch = defItemsMatch<T>)
        : m_pFirst(nullptr), m_itemsMatch(itemsMatch) {}

    EmList(EmList<T>& list)
        : EmList(list.m_itemsMatch) {
        append(list);
    }

    // Per Rule of Three/Five, since this class has a custom destructor to manage resources,
    // it should also have a copy constructor and copy assignment operator, or have them deleted.
    // The original copy constructor was shallow and dangerous. Disabling them is the safest default.
    EmList(const EmList<T>&) = delete;
    EmList& operator=(const EmList<T>&) = delete;

    // NOTE: keep destructor and class without virtual functions to limit RAM footprint
    ~EmList() { clear(); }

    // Append an element by creating a copy.
    // 
    // If 'takeOwnership' is true, the list takes ownership creating a copy of 'item'
    // and deleting it when no longer needed.
    // If 'takeOwnership' is false, the caller is responsible for ensuring that the 
    // lifetime of 'item' exceeds the lifetime of this list. Use with caution, 
    // typically for objects with static or global scope.
    void append(T& item, bool takeOwnership) {
        if (takeOwnership) {
            append_(new T(item), true);
        } else {
            append_(&item, false);
        }
    }

    // Append an element pointer at the end of the list.
    //
    // If takeOwnership is true, the list takes ownership of 'item' deleting it when
    // no longer needed.
    // If 'takeOwnership' is false, the caller is responsible for freeing 'item' once
    // it is no longer needed.
    void append(T* item, bool takeOwnership) {
        if (item != nullptr) {
            append_(item, takeOwnership);
        }
    }

    // Appends an instance that will not be owned (e.g. global objects).
    //
    // This method has been added for clarity. It might be used for those objects that
    // does not have a copy constructor (i.e. cannot une the 'append(item, false)' because
    // compiler will give an error).
    void appendUnowned(T& item) {
        append_(&item, false);
    }

    // Extend this list by appending all elements from another list.
    //
    // If 'takeOwnership' is true, the list takes ownership creating a copy of each 
    // 'list' item and deleting it when no longer needed.
    // If 'takeOwnership' is false, the caller is responsible for ensuring that the 
    // lifetime of 'item' exceeds the lifetime of this list. Use with caution, 
    // typically for objects with static or global scope.
    void extend(EmList<T>& list, bool takeOwnership) {
        _EmListElement<T>* elem = list.m_pFirst;
        while (elem != nullptr) {
            append_(elem->m_pItem, takeOwnership);
            elem = elem->next();
        }
    }

    // Sets the list elements to the specified 'list' elements.
    //
    // This is same as calling 'clear' and 'extend'
    void set(EmList<T>& list, bool takeOwnership) {
        clear();
        extend(list, takeOwnership);
    }

    // Remove an element from list.
    //
    // Returns true if element has been found and removed.
    bool remove(T& item) {
        _EmListElement<T>* pPrev = nullptr;
        _EmListElement<T>* elem = m_pFirst;
        while (elem != nullptr) {
            if (m_itemsMatch(*(elem->m_pItem), item)) {
                // Found!
                remove_(elem, pPrev);
                return true;
            }
            pPrev = elem;
            elem = elem->next();
        }
        return false;
    }

    bool remove(T* item) {
        if (item == nullptr) return false;
        return remove(*item);
    }

    // Remove all elements in that equals 'list' elements.
    //
    // Returns false if at least one element in the removal list was not found
    bool remove(EmList<T>& list) {
        bool res = true;
        _EmListElement<T>* elem = list.m_pFirst;
        while (elem != nullptr) {
            if (!remove(*elem->m_pItem)) {
                res = false;
            }
            elem = elem->next();
        }
        return res;
    }

    // Find the same element of the list. T should have right equality operator.
    //
    // Return NULL if element is not found.
    T* find(const T& item) const {
        _EmListElement<T>* elem = m_pFirst;
        while (elem != nullptr) {
            if (m_itemsMatch(*elem->m_pItem, item)) {
                return elem->m_pItem;
            }
            elem = elem->next();
        }
        return nullptr;
    }

    T* find(const T* item) const {
        if (item == nullptr) return nullptr;
        return find(*item);
    }

    // Return the number of elements in the list
    uint16_t count() const {
        uint16_t count = 0;
        _EmListElement<T>* elem = m_pFirst;
        while (elem != nullptr) {
            ++count;
            elem = elem->next();
        }
        return count;
    }

    bool isEmpty() const { return m_pFirst == nullptr; }
    bool isNotEmpty() const { return !isEmpty(); }

    void clear() {
        _EmListElement<T>* item = m_pFirst;
        while (item != nullptr) {
            _EmListElement<T>* next = item->next();
            delete item;
            item = next;
        }
        m_pFirst = nullptr;
    }

    EmIterResult forEach(IterationCb<T> iter) {
        return forEach_<void>((void*)iter, false, nullptr);
    }

    template<class V = void>
    EmIterResult forEach(IterationExCb<T, V> iter, V* pUserData = nullptr) {
        return forEach_<V>((void*)iter, true, pUserData);
    }

    T* first() { return m_pFirst ? m_pFirst->m_pItem : nullptr; }
    const T* first() const { return m_pFirst ? m_pFirst->m_pItem : nullptr; }
    T* last() {
        _EmListElement<T>* last = last_();
        return last ? last->m_pItem : nullptr;
    }
    const T* last() const {
        _EmListElement<T>* last = last_();
        return last ? last->m_pItem : nullptr;
    }

protected:
    void append_(T* pItem, bool takeOwnership) {
        _EmListElement<T>* last = last_();
        if (last) {
            last->m_pNext = new _EmListElement<T>(*this, pItem, takeOwnership);
        } else {
            m_pFirst = new _EmListElement<T>(*this, pItem, takeOwnership);
        }
    }

    template<class V>
    EmIterResult forEach_(void* iter, bool isExtendedCb, V* pUserData) {
        _EmListElement<T>* pPrev = nullptr;
        _EmListElement<T>* pItem = m_pFirst;
        EmIterResult res = EmIterResult::moveNext;
        while (pItem != nullptr) {
            res = isExtendedCb ?
                ((IterationExCb<T, V>)iter)(*pItem->m_pItem, pItem == m_pFirst, pItem->m_pNext == nullptr, pUserData) :
                ((IterationCb<T>)iter)(*pItem->m_pItem);
            switch (res) {
                case EmIterResult::stopSucceed: return res;
                case EmIterResult::stopFailed: return res;
                case EmIterResult::removeMoveNext:
                    pItem = remove_(pItem, pPrev);
                    break;
                case EmIterResult::removeStopSucceed:
                    pItem = remove_(pItem, pPrev);
                    return res;
                case EmIterResult::removeStopFailed:
                    pItem = remove_(pItem, pPrev);
                    return res;
                default:
                    pPrev = pItem;
                    pItem = pItem->next();
            }
        }
        return res;
    }

    _EmListElement<T>* last_() const {
        _EmListElement<T>* elem = m_pFirst;
        while (elem && elem->m_pNext) {
            elem = elem->next();
        }
        return elem;
    }

    _EmListElement<T>* remove_(_EmListElement<T>* item, _EmListElement<T>* prev) {
        _EmListElement<T>* next = item->next();
        if (prev) {
            prev->m_pNext = item->m_pNext;
        } else {
            m_pFirst = item->next();
        }
        delete item;
        return next;
    }

private:
    _EmListElement<T>* m_pFirst;
    ItemsMatchCb<T> m_itemsMatch;
};

#endif // __EM_LIST_H__
