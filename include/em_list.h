#pragma once

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
    _EmListElement(EmList<T>& list, T* pItem, bool shouldBeDeleted)
        : EmListIterator<T>(list), 
          m_shouldBeDeleted(shouldBeDeleted) {
        this->m_pItem = pItem;
        this->m_pNext = nullptr;
    }
    
    // NOTE: keep destructor and class without virtual functions to limit RAM footprint
    ~_EmListElement() {
        if (this->m_pItem != nullptr && m_shouldBeDeleted) {
            delete this->m_pItem;
        }
    }

    _EmListElement<T>* next() const {
        return static_cast<_EmListElement<T>*>(this->m_pNext);
    }

    bool m_shouldBeDeleted;
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

    // NOTE: keep destructor and class without virtual functions to limit RAM footprint
    ~EmList() { clear(); }

    // Append an element at the end of the list.
    void append(T& item) { _append(item, false); }

    // Append an element pointer at the end of the list.
    void append(T* item, bool shouldBeDeleted) {
        if (item != nullptr) {
            _append(*item, shouldBeDeleted);
        }
    }

    // Append 'list' elements at the end of this list.
    void append(EmList<T>& list) {
        list.forEach<EmList<T>>([](T& item, bool, bool, EmList<T>* pThis) -> EmIterResult {
            pThis->_append(item, false);
            return EmIterResult::moveNext;
        }, this);
    }

    // Remove an element from list.
    // Returns true if element has been found and removed.
    bool remove(T& item) {
        _EmListElement<T>* pPrev = nullptr;
        _EmListElement<T>* elem = m_pFirst;
        while (elem != nullptr) {
            if (m_itemsMatch(*(elem->m_pItem), item)) {
                // Found!
                _remove(elem, pPrev);
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
    // Returns false if at least one element in the removal list was not found
    bool remove(const EmList<T>& list) {
        bool res = true;
        list.forEach<bool>([this](T& item, bool, bool, bool* pRes) -> EmIterResult {
            if (!remove(item)) *pRes = false;
            return EmIterResult::moveNext;
        }, &res);
        return res;
    }

    // Find the same element of the list. T should have right equality operator.
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

    bool forEach(IterationCb<T> iter) {
        return _forEach<void>((void*)iter, false, nullptr);
    }

    template<class V = void>
    bool forEach(IterationExCb<T, V> iter, V* pUserData = nullptr) {
        return _forEach<V>((void*)iter, true, pUserData);
    }

    T* first() const { return m_pFirst ? m_pFirst->m_pItem : nullptr; }
    T* last() const {
        _EmListElement<T>* last = _last();
        return last ? last->m_pItem : nullptr;
    }

protected:
    void _append(T& item, bool shouldBeDeleted) {
        _EmListElement<T>* last = _last();
        if (last) {
            last->m_pNext = new _EmListElement<T>(*this, &item, shouldBeDeleted);
        } else {
            m_pFirst = new _EmListElement<T>(*this, &item, shouldBeDeleted);
        }
    }

    template<class V>
    bool _forEach(void* iter, bool isExtendedCb, V* pUserData) {
        _EmListElement<T>* pPrev = nullptr;
        _EmListElement<T>* pItem = m_pFirst;
        while (pItem != nullptr) {
            EmIterResult res = isExtendedCb ?
                ((IterationExCb<T, V>)iter)(*pItem->m_pItem, pItem == m_pFirst, pItem->m_pNext == nullptr, pUserData) :
                ((IterationCb<T>)iter)(*pItem->m_pItem);
            switch (res) {
                case EmIterResult::stopSucceed: return true;
                case EmIterResult::stopFailed: return false;
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
                    pItem = pItem->next();
            }
        }
        return true;
    }

    _EmListElement<T>* _last() const {
        _EmListElement<T>* elem = m_pFirst;
        while (elem && elem->m_pNext) {
            elem = elem->next();
        }
        return elem;
    }

    _EmListElement<T>* _remove(_EmListElement<T>* item, _EmListElement<T>* prev) {
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

