#ifndef __SYNCVALUE__H_
#define __SYNCVALUE__H_

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>

#include "em_sync_lock.h"


enum EmSyncTarget {
    none     = 0x00,
    toLocal  = 0x01,
    toRemote = 0x02
};

enum EmSyncSource {
    fromRemote = toLocal,
    fromLocal  = toRemote,
    fromProcess = toLocal|toRemote
};

class EmUpdatableValue {
public:
    EmUpdatableValue(bool isHighSampling)
     : m_IsHighSampling(isHighSampling) {}
    
    virtual bool SetToTarget(EmSyncTarget syncTarget)=0;
    virtual bool GetFromSource(EmSyncSource syncSource)=0;
    
    virtual void Update()
        { SetToTarget(toLocal);
          SetToTarget(toRemote);
          GetFromSource(fromLocal);
          GetFromSource(fromRemote); }

    virtual bool IsHighSampling() const { return m_IsHighSampling; }

protected:
    bool m_IsHighSampling;
};

template <class T>
class EmValueSource {
public:
    EmValueSource() {};

    virtual bool GetSourceValue(T& value)=0;
    virtual bool SetSourceValue(const T value)=0;
};

template <class T>
class EmValueSource<T*> {
public:
    EmValueSource(const uint8_t len)
        : m_Len(len) {};

    virtual bool GetSourceValue(T* value)=0;
    virtual bool SetSourceValue(const T* value)=0;

    uint8_t GetLen() const { return m_Len; };

protected:
    const uint8_t m_Len;
};

// https://stackoverflow.com/questions/22847803/c-destructor-with-templates-t-could-be-a-pointer-or-not
class EmSyncValueBase: public EmUpdatableValue {
// This is the base class for handling values coming from different sources (i.e. local, remote, ...)
public:
    EmSyncValueBase(bool isHighSampling)
     : EmUpdatableValue(isHighSampling),
       m_SyncTarget(none),
       m_Semaphore(xSemaphoreCreateBinary())
    {
        xSemaphoreGive(m_Semaphore);
    }

    virtual bool IsSyncTarget(EmSyncTarget syncTarget) 
    {
        EmSyncLock syncLock(m_Semaphore);
        return m_SyncTarget & syncTarget;
    }

    virtual EmSyncTarget GetSyncTarget() 
    {
        EmSyncLock syncLock(m_Semaphore);
        return m_SyncTarget;
    }

    virtual void AddSource(EmSyncSource syncSource)
    { 
        EmSyncLock syncLock(m_Semaphore);
        m_SyncTarget = (EmSyncTarget)((int)m_SyncTarget|(int)syncSource);
    }

    virtual void RemoveTarget(EmSyncTarget syncTarget)
    { 
        EmSyncLock syncLock(m_Semaphore);
        m_SyncTarget = (EmSyncTarget)((int)m_SyncTarget&(int)~syncTarget);
    }

protected:
    EmSyncTarget m_SyncTarget;
    SemaphoreHandle_t m_Semaphore;
};

template <class T>
class EmSyncValue: public EmSyncValueBase {
public:
    EmSyncValue(EmValueSource<T>* localSource=NULL,
                EmValueSource<T>* remoteSource=NULL,
                bool isHighSampling=false)
        : EmSyncValueBase(isHighSampling),
        m_LocalSource(localSource),
        m_RemoteSource(remoteSource)
    {
    }

    virtual bool IsEqual(const T val) const
    {
        EmSyncLock syncLock(this->m_Semaphore);
        return val == m_Value;
    }

    const T PeekValue()
    {
        EmSyncLock syncLock(this->m_Semaphore);
        return m_Value;
    }

    virtual void GetValue(T& value)
    {
        Copy(value, m_Value);
    }

    virtual void SetValue(const T value, EmSyncSource syncSource)
    {
        if (!IsEqual(value))
        {
            Copy(m_Value, value);
            this->AddSource(syncSource);
        }
    }

    virtual bool SetToTarget(EmSyncTarget syncTarget)
    {
        if (syncTarget == toLocal)
        {
            return _set(m_LocalSource, toLocal);
        }
        if (syncTarget == toRemote)
        {
            return _set(m_RemoteSource, toLocal);
        }
        return false;
    }

    virtual bool GetFromSource(EmSyncSource syncSource)
    {
        if (syncSource == fromLocal)
        {
            return _get(m_LocalSource, fromLocal);
        }
        if (syncSource == fromRemote)
        {
            return _get(m_RemoteSource, fromRemote);
        }
        return false;
    }

protected:
    virtual bool _set(EmValueSource<T>* valueSource, EmSyncTarget target)
    {
        // Need to set source?
        if (this->IsSyncTarget(target))
        {
            if (valueSource)
            {
                if (!valueSource->SetSourceValue(m_Value))
                {
                    return false;
                }
                this->RemoveTarget(target);
            }
        }
        return true;
    }

    virtual bool _get(EmValueSource<T>* valueSource, EmSyncSource source)
    {
        // Need to set source?
        if (!this->IsSyncTarget((EmSyncTarget)source))
        {
            if (valueSource)
            {
                // Get value from source
                T value;
                if (!valueSource->GetSourceValue(value))
                {
                    return false;
                }
                // Set new value we got
                this->SetValue(value, source);
            }
        }
        return true;
    }

    virtual void Copy(T& dest, const T source)
    {
        EmSyncLock syncLock(this->m_Semaphore);
        dest = source;
    }

    EmValueSource<T>* m_LocalSource;
    EmValueSource<T>* m_RemoteSource;
    T m_Value;
};

template <class T>
class EmSyncValue<T*>: public EmSyncValueBase {
public:
    EmSyncValue(EmValueSource<T*>* localSource=NULL,
              EmValueSource<T*>* remoteSource=NULL,
              uint8_t len=0,
              bool isHighSampling=false)
     : EmSyncValueBase(isHighSampling),
       m_LocalSource(localSource),
       m_RemoteSource(remoteSource),
       m_Len(len > 0 ? len : (localSource!=NULL ? localSource->GetLen() : (remoteSource!=NULL ? remoteSource->GetLen() : 0)))           
    { 
        m_Value = new T[len+1]; 
    };

    virtual ~EmSyncValue()
    {
        delete[] m_Value;
    } 

    virtual bool IsEqual(const T* val) const
    {
        EmSyncLock syncLock(this->m_Semaphore);
        return (0==memcmp(val, m_Value, m_Len));
    }

    const T* PeekValue()
    {
        EmSyncLock syncLock(this->m_Semaphore);
        return m_Value;
    }

    virtual void GetValue(T* value)
    {
        Copy(value, m_Value);
    }

    virtual void SetValue(const T* value, EmSyncSource syncSource)
    {
        if (!IsEqual(value))
        {
            Copy(m_Value, value);
            this->AddSource(syncSource);
        }
    }

    virtual bool SetToTarget(EmSyncTarget syncTarget)
    {
        if (syncTarget == toLocal)
        {
            return _set(m_LocalSource, toLocal);
        }
        if (syncTarget == toRemote)
        {
            return _set(m_RemoteSource, toLocal);
        }
        return false;
    }

    virtual bool GetFromSource(EmSyncSource syncSource)
    {
        if (syncSource == fromLocal)
        {
            return _get(m_LocalSource, fromLocal);
        }
        if (syncSource == fromRemote)
        {
            return _get(m_RemoteSource, fromRemote);
        }
        return false;
    }

protected:
    virtual bool _set(EmValueSource<T*>* valueSource, EmSyncTarget target)
    {
        // Need to set source?
        if (this->IsSyncTarget(target))
        {
            if (valueSource)
            {
                if (!valueSource->SetSourceValue(m_Value))
                {
                    return false;
                }
                this->RemoveTarget(target);
            }
        }
        return true;
    }

    virtual bool _get(EmValueSource<T*>* valueSource, EmSyncSource source)
    {
        // Need to set source?
        if (!this->IsSyncTarget((EmSyncTarget)source))
        {
            if (valueSource)
            {
                // Get value from source
                T* value = new T[m_Len+1];
                if (!valueSource->GetSourceValue(value))
                {
                    delete[] value;
                    return false;
                }
                // Set new value we got
                SetValue(value, source);
                delete[] value;
            }
        }
        return true;
    }

    virtual void Copy(T* dest, const T* source)
    {
        EmSyncLock syncLock(this->m_Semaphore);
        memcpy(dest, source, m_Len);
    }

    EmValueSource<T*>* m_LocalSource;
    EmValueSource<T*>* m_RemoteSource;
    const uint8_t m_Len;
    T* m_Value;
};

#endif