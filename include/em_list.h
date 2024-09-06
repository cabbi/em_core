#pragma once

#include <stdint.h>

enum class EmIterResult {
    moveNext = 0, // Default is continue iteration by moving to next list value
    stopSucceed,
    stopFailed,
    removeMoveNext,
    removeStopSucceed,
    removeStopFailed,
};

// Forward declarations
template<class T> class EmList;
template<class T> class _EmListItem {
    friend class EmList<T>;
private:    
    _EmListItem(T* pItem, bool shouldBeDeleted)
    : m_ShouldBeDeleted(shouldBeDeleted), 
      m_pItem(pItem), 
      m_pNext(NULL) {}

    virtual ~_EmListItem() {
        if (NULL != m_pItem && m_ShouldBeDeleted) {
            delete m_pItem;
        }
    }

    bool m_ShouldBeDeleted;
    T* m_pItem;
    _EmListItem<T>* m_pNext;
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

    virtual ~EmList() {
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
        _EmListItem<T>* pPrev = NULL;
        _EmListItem<T>* elem = m_pFirst;
        while (NULL != elem) {
            if (m_ItemsMatch(*(elem->m_pItem), item)) {
                // Found!
                _remove(elem, pPrev);
                return true;
            }
            pPrev = elem;
            elem = elem->m_pNext;
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

    // Find the same element of the list. T should have right equalty operator.
    // Return NULL if element is not found.
    T* Find(const T& item) {
        _EmListItem<T>* elem = m_pFirst;
        while (NULL != elem) {
            if (m_ItemsMatch(*elem->m_pItem, item)) {
                return elem->m_pItem;
            }
            elem = elem->m_pNext;
        }
        return NULL;
    }

    T* Find(const T* item) {
        if (NULL == item) {
            return NULL;
        }
        return Find(*item);
    }

    // Return the number of elements in the list
    uint16_t Count() const {
        uint16_t count = 0;
        _EmListItem<T>* last = m_pFirst;
        while (NULL != last) {
            count++;
            last = last->m_pNext;
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
        _EmListItem<T>* next = NULL;
        _EmListItem<T>* item = m_pFirst;
        while (NULL != item) {
            next = item->m_pNext;
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
        _EmListItem<T>* last = _last();
        return NULL == last ? NULL : last->m_pItem;
    }

protected:
    void _append(T& item, bool shouldBeDeleted) {
        _EmListItem<T>* last = _last();
        if (NULL != last) {
            last->m_pNext = new _EmListItem<T>(&item, shouldBeDeleted);
        } else {
            m_pFirst = new _EmListItem<T>(&item, shouldBeDeleted);
        }
    }

    template<class V> bool _forEach(void* iter, bool isExtendedCb, V* pUserData) {
        _EmListItem<T>* pPrev = NULL;
        _EmListItem<T>* pItem = _first();
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
                    pItem = pItem->m_pNext;
            }
        }
        return true;
    }

    _EmListItem<T>* _first() const {
        return m_pFirst;
    }

    _EmListItem<T>* _last() const {
        _EmListItem<T>* pItem = m_pFirst;
        while (NULL != pItem) {
            if (NULL == pItem->m_pNext) {
                return pItem;
            }
            pItem = pItem->m_pNext;
        }
        return NULL;
    }

    // Removes the element returning the next one in the list
    _EmListItem<T>* _remove(_EmListItem<T>* item, _EmListItem<T>* prev) {
        _EmListItem<T>* pNext = item->m_pNext;
        // Set previous value to point to the removed's next item
        if (NULL != prev) {
            prev->m_pNext = item->m_pNext;
        } else {
            m_pFirst = item->m_pNext;
        }
        // Delete this item
        delete item;
        return pNext;
    }

private:
    _EmListItem<T>* m_pFirst;
    ItemsMatchCb<T> m_ItemsMatch;
};

