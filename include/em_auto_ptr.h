#ifndef __EM_AUTO_PTR_H__
#define __EM_AUTO_PTR_H__

// A very lightweight alternative to std::auto_ptr for environments
// where the standard library is not available or desired.
//
// NOTE: This is a simplified implementation. It is not copyable or movable
// to prevent accidental multiple ownership and double-free errors.
template<class T>
class EmAutoPtr {
public:
    // Constructor takes ownership of the pointer.
    explicit EmAutoPtr(T* ptr = nullptr) : m_ptr(ptr) {}

    // Destructor deletes the managed object.
    ~EmAutoPtr() {
        delete m_ptr;
    }

    // Forbid copying and assignment to prevent multiple ownership.
    EmAutoPtr(const EmAutoPtr&) = delete;
    EmAutoPtr& operator=(const EmAutoPtr&) = delete;

    // Overload -> to access members of the managed object.
    T* operator->() const { return m_ptr; }

    // Overload * to dereference the pointer.
    T& operator*() const { return *m_ptr; }

    // Get the raw pointer.
    T* get() const { return m_ptr; }

private:
    // The raw pointer to the managed object.
    T* m_ptr;
};

#endif // __EM_AUTO_PTR_H__