#pragma once

#include <stdint.h>


enum class EmIterResult: int8_t {
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
template<class T> class EmListIterator {
    friend class EmList<T>;
public:
    EmListIterator()
     : m_pItem(NULL),
       m_pNext(NULL) {}

    // NOTE: keep destructor and class without virtual functions to avoid extra RAM consumption
    ~EmListIterator() {}

    operator T*() const { 
        return m_pItem; 
    }

    T* Item() const {
        return m_pItem;
    }

    // Reset the iterator
    void Reset() {
        m_pItem = NULL;
        m_pNext = NULL;
    }

protected:    
    EmListIterator(T* pItem, EmListIterator<T>* pNext = NULL)
    : m_pItem(pItem), 
      m_pNext(pNext) {}

    bool _isBegin() {
        return NULL == m_pItem && NULL == m_pNext;
    }

    void _copyFrom(EmListIterator<T>* pIter) {
        m_pItem = NULL == pIter ? NULL : pIter->m_pItem;
        m_pNext = NULL == pIter ? NULL : pIter->m_pNext;
    }

    T* m_pItem;
    EmListIterator<T>* m_pNext;
};


// List element (private!)
template<class T> class _EmListElement: protected EmListIterator<T> {
    friend class EmList<T>;
private:
    _EmListElement(T* pItem, bool shouldBeDeleted)
    : EmListIterator<T>(pItem),
      m_ShouldBeDeleted(shouldBeDeleted) {}

    // NOTE: keep destructor and class without virtual functions to avoid extra RAM consumption
    ~_EmListElement() {
        if (NULL != this->m_pItem && m_ShouldBeDeleted) {
            delete this->m_pItem;
        }
    }

    _EmListElement<T>* Next() const {
        return (_EmListElement<T>*)this->m_pNext;
    }

    bool m_ShouldBeDeleted;
};


// Items matching callback prototype
// NOTE: Arduino platform does not have std::functional definition! :()
template<class T> using ItemsMatchCb = bool(*)(const T& item1, const T& item2);
template<class T> using IterationCb = EmIterResult(*)(T& item); 
template<class T, class V> 
    using IterationExCb = EmIterResult(*)(T& item, 
                                          bool isFirst,
                                          bool isLast,
                                          V* pUserData);

// Default items matching callback function
template<class T> inline bool DefItemsMatch(const T& item1, const T& item2) {
    return item1 == item2;
}


/***
    An easy list implementation 
 ***/
template<class T> class EmList {
public:
    EmList(ItemsMatchCb<T> itemsMatch)
     : m_pFirst(NULL),
       m_ItemsMatch(itemsMatch) {}

    EmList(const EmList<T>& list)
     : EmList(list.m_ItemsMatch) {
        Append(list);
    }

    // NOTE: keep destructor and class without virtual functions to avoid extra RAM consumption
    ~EmList() {
        Clear();
    }

    // Append an element at the end of the list.
    void Append(T& item) {
        _append(item, false);
    }

    // Append an element pointer at the end of the list.
    // If 'shouldBeDeleted' is true, then list will free 
    // the item object once released from the list 
    void Append(T* item, bool shouldBeDeleted) {
        // NOTE: Cannot append NULLs to list!
        if (NULL != item) {
            _append(*item, shouldBeDeleted);
        }
    }

    // Append 'list' elements at the end of this list.
    void Append(const EmList<T>& list) {
        // 'list' object will not change, so it's safe to cast it as non-const
        ((EmList<T>&)list).ForEach<EmList<T>>([](T& item, 
                                              bool,
                                              bool,
                                              EmList<T>* pThis) -> EmIterResult {
            pThis->_append(item, false);
            return EmIterResult::moveNext;
        }, this);
    }

    // Remove an element from list.
    // Returns true if element has been found and removed.
    bool Remove(T& item) {
        _EmListElement<T>* pPrev = NULL;
        _EmListElement<T>* elem = m_pFirst;
        while (NULL != elem) {
            if (m_ItemsMatch(*(elem->m_pItem), item)) {
                // Found!
                _remove(elem, pPrev);
                return true;
            }
            pPrev = elem;
            elem = elem->Next();
        }
        return false;
    }

    bool Remove(T* item) {
        if (NULL == item) {
            return NULL;
        }
        return Remove(*item);
    }

    // Returns false if at least one element in the reoval list is not found
    bool Remove(const EmList<T>& list) {
        bool res = true;
        // 'list' object will not change, so it's safe to cast it as non-const
        return ((EmList<T>&)list).ForEach<bool>([](T& item, 
                                                   bool, 
                                                   bool, 
                                                   bool* pRes) -> EmIterResult {
            if (!Remove(item)) {
                *pRes = false;
            }
            return EmIterResult::moveNext; 
        }, &res);
    }

    // Find the same element of the list. T should have right equality operator.
    // Return NULL if element is not found.
    T* Find(const T& item) const {
        _EmListElement<T>* elem = m_pFirst;
        while (NULL != elem) {
            if (m_ItemsMatch(*elem->m_pItem, item)) {
                return elem->m_pItem;
            }
            elem = elem->Next();
        }
        return NULL;
    }

    T* Find(const T* item) const {
        if (NULL == item) {
            return NULL;
        }
        return Find(*item);
    }

    // Return the number of elements in the list
    uint16_t Count() const {
        uint16_t count = 0;
        _EmListElement<T>* last = m_pFirst;
        while (NULL != last) {
            count++;
            last = last->Next();
        }
        return count;
    }

    // Return true if list is empty
    bool IsEmpty() {
        return m_pFirst == NULL;
    }

    // Return true if list is not empty
    bool IsNotEmpty() {
        return !IsEmpty();
    }

    // Clear the list
    void Clear() {
        _EmListElement<T>* next = NULL;
        _EmListElement<T>* item = m_pFirst;
        while (NULL != item) {
            next = (_EmListElement<T>*)item->m_pNext;
            delete item;
            item = next;
        }
        m_pFirst = NULL;
    }

    // Iterate the list elements. 
    bool ForEach(IterationCb<T> iter) {
        return _forEach<void>((void*)iter, false, NULL);
    }

    // Iterate the list elements with extended callback. 
    template<class V=void> bool ForEach(IterationExCb<T, V> iter, V* pUserData=NULL) {
        return _forEach<V>((void*)iter, true, pUserData);
    }

    // Return the first element in the list or NULL if list is empty
    T* First() const {
        return NULL == m_pFirst ? NULL : m_pFirst->m_pItem;
    }

    // Return the last element in the list or NULL if list is empty
    T* Last() const  {
        _EmListElement<T>* last = _last();
        return NULL == last ? NULL : last->m_pItem;
    }

    // Iterate list items. 
    // Return false once end of list is reached.
    bool Iterate(EmListIterator<T>& iterator) const {
        if (iterator._isBegin()) {
            iterator._copyFrom(m_pFirst);
        } else {
            iterator._copyFrom(iterator.m_pNext);
        }
        return NULL != iterator.m_pItem;
    }

protected:
    void _append(T& item, bool shouldBeDeleted) {
        _EmListElement<T>* last = _last();
        if (NULL != last) {
            last->m_pNext = new _EmListElement<T>(&item, shouldBeDeleted);
        } else {
            m_pFirst = new _EmListElement<T>(&item, shouldBeDeleted);
        }
    }

    template<class V> bool _forEach(void* iter, bool isExtendedCb, V* pUserData) {
        _EmListElement<T>* pPrev = NULL;
        _EmListElement<T>* pItem = _first();
        while (NULL != pItem) {
            EmIterResult res = isExtendedCb ? 
                               ((IterationExCb<T, V>)iter)(*pItem->m_pItem, 
                                                           pItem == _first(),
                                                           pItem->m_pNext == NULL,
                                                           pUserData) :
                               ((IterationCb<T>)iter)(*pItem->m_pItem);
            switch (res) {
                case EmIterResult::stopSucceed:
                    return true;
                case EmIterResult::stopFailed:
                    return false;
                case EmIterResult::removeMoveNext:
                    pItem = _remove(pItem, pPrev);
                    break;
                case EmIterResult::removeStopSucceed:
                    pItem = _remove(pItem, pPrev);
                    return true;
                case EmIterResult::removeStopFailed:
                    pItem = _remove(pItem, pPrev);
                    return false;
                default:
                    pPrev = pItem;
                    pItem = pItem->Next();
            }
        }
        return true;
    }

    _EmListElement<T>* _first() const {
        return m_pFirst;
    }

    _EmListElement<T>* _last() const {
        _EmListElement<T>* pItem = m_pFirst;
        while (NULL != pItem) {
            if (NULL == pItem->m_pNext) {
                return pItem;
            }
            pItem = pItem->Next();
        }
        return NULL;
    }

    // Removes the element returning the next one in the list
    _EmListElement<T>* _remove(_EmListElement<T>* item, _EmListElement<T>* prev) {
        _EmListElement<T>* pNext = item->Next();
        // Set previous value to point to the removed's next item
        if (NULL != prev) {
            prev->m_pNext = item->m_pNext;
        } else {
            m_pFirst = item->Next();
        }
        // Delete this item
        delete item;
        return pNext;
    }

private:
    _EmListElement<T>* m_pFirst;
    ItemsMatchCb<T> m_ItemsMatch;
};

