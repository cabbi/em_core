#ifndef __COM_DEVICE__H_
#define __COM_DEVICE__H_

#include "Arduino.h"
#include "Stream.h"
//#include "Wire.h"

class EmComDevice {
public:
    EmComDevice()
     : m_IsAvailable(true)
    {}

    virtual bool IsAvailable() const { return m_IsAvailable; }
    virtual void KeepDevice() { m_IsAvailable = false; }
    virtual void ReleaseDevice() { m_IsAvailable = true; }

protected: 
    bool m_IsAvailable;
};

typedef bool (*DeviceIterFunc)(EmComDevice* device, void* arg);  // Function should return 'false' to stop iteration

/*
class ComPool {
public:
    ComPool()
    {}

    void AddDevice(ComDevice* device) 
    { m_ComDevices.push_back(device); }

    void IterDevices(DeviceIterFunc iterFunc, void* arg=NULL, bool availableOnly=false)
    {
        for (std::vector<ComDevice*>::iterator it = m_ComDevices.begin(); it != m_ComDevices.end(); ++it)
        {
            if (!availableOnly || (*it)->IsAvailable())
            {
                if (!iterFunc(*it, arg))
                {
                    return;
                }
            }
        }
    }
    
protected:
    std::vector<ComDevice*> m_ComDevices;
};
*/

class EmComSerial: public EmComDevice
{
public:
    EmComSerial(Stream& serial)
     : m_serial(serial)
    {}

    int read() {
        return m_serial.read();
    }

    int available() {
        return m_serial.available();
    }

    int peek() {
        return m_serial.peek();
    }

    int write(byte b) {
        return m_serial.write(b);
    }

    int write(const char* bytes) {
        return m_serial.write(bytes);
    }

    void flush() {
        m_serial.flush();
    }

private:
    Stream& m_serial;
};

/*
class EmComI2C: public TwoWire, public EmComDevice
{
public:
    EmComI2C(uint8_t bus_num)
    : TwoWire(bus_num)
    {}        
};
*/

#endif