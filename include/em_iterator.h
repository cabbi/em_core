#ifndef __EM_ITERATOR_H__
#define __EM_ITERATOR_H__

#include <stddef.h>

// The abstract iterator class.
template<class T> 
class EmIterator {
public:
    // Returns true if next item is available or false if iterable is empty or end of iteration is reached
    virtual bool next(T*& pItem) = 0;

    // Resets the iterator by starting from begin
    virtual void reset() = 0;
};


template<class T> 
class EmArrayIterator: public EmIterator<T> {
public:
    EmArrayIterator(T* items[], size_t size)
     : m_items(items), m_size(size) {
        reset();
    }

    // Returns true if next item is available or false if iterable is empty or end of iteration is reached
    virtual bool next(T*& pItem) override {
        if (m_index < static_cast<int>(m_size) - 1) {
            pItem = m_items[++m_index];
            return true;
        }
        pItem = NULL;
        return false;
    }

    // Resets the iterator by starting from begin
    virtual void reset() override {
        m_index = -1;
    }

private:
    T** m_items;
    size_t m_size;
    int m_index;
};

#endif // __EM_ITERATOR_H__
