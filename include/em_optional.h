#ifndef __EM_OPTIONAL_H__
#define __EM_OPTIONAL_H__

// A very lightweight alternative to std::optional since std library is not part of AVR development

// The undefined optional value (i.e. no value)
class EmUndefined{};
constexpr EmUndefined emUndefined = EmUndefined();

// The optional class allowing uninitialized state
template<class T>
class EmOptional {
public:
    EmOptional() : m_hasValue(false) {}
    EmOptional(const EmUndefined&) : m_hasValue(false) {}
    EmOptional(const T& value) : m_hasValue(true), m_value(value) {}

    bool hasValue() const { return m_hasValue; }
    bool hasNoValue() const { return !hasValue(); }
    
    T& value() { return m_value; }
    const T& value() const { return m_value; }

    EmOptional& operator=(const T& newValue) {
        m_hasValue = true;
        m_value = newValue;
        return *this;
    }

    EmOptional& operator=(const EmOptional& other) {
        m_hasValue = other.hasValue;
        m_value = other.value;
        return *this;
    }

    EmOptional<T>& operator=(const EmUndefined&) {
        m_hasValue = false;
        return *this;
    }

    bool operator==(const T& value) const {
        return m_hasValue && m_value == value;
    }

    bool operator==(const EmOptional& other) const {
        return m_hasValue == other.m_hasValue && m_value == other.m_value;
    }

    EmOptional& operator==(EmUndefined&) const {
        return !m_hasValue;
    }

private:
    bool m_hasValue;
    T m_value;
};

#endif // __EM_OPTIONAL_H__