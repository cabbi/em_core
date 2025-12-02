#ifndef __COM_DEVICE__H_
#define __COM_DEVICE__H_

#include "Stream.h"
//#include "Wire.h"

class EmComDevice {
public:
    EmComDevice()
     : m_IsAvailable(true)
    {}

    virtual bool isAvailable() const { return m_IsAvailable; }
    virtual void keepDevice() { m_IsAvailable = true; }
    virtual void releaseDevice() { m_IsAvailable = false; }

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

class EmComSerial: public EmComDevice, public EmSerialStream
{
public:
    EmComSerial(EmSerialStream& serial)
     : m_serial(serial)
    {}

	virtual void begin(unsigned long baud) {
        m_serial.begin(baud);
        m_IsAvailable = true;
    }

	virtual void begin(unsigned long baud, 
                       uint32_t config, 
                       int8_t rxPin=-1, 
                       int8_t txPin=-1, 
                       bool invert=false, 
                       unsigned long timeout_ms = 20000UL, 
                       uint8_t rxfifo_full_thrhd = 112) {
        m_serial.begin(baud, 
                        config, 
                        rxPin, 
                        txPin, 
                        invert, 
                        timeout_ms, 
                        rxfifo_full_thrhd);
        m_IsAvailable = true;
    }
	
    virtual void end() {
        m_serial.end();
        m_IsAvailable = false;
    }

    int read() {
        return m_serial.read();
    }

    int available() {
        return m_serial.available();
    }

    int peek() {
        return m_serial.peek();
    }

    int write(uint8_t b) {
        return m_serial.write(b);
    }

    int write(const char* bytes) {
        return m_serial.write(bytes);
    }

    void flush() {
        m_serial.flush();
    }

private:
    EmSerialStream& m_serial;
};

/* TODO
class EmComI2C: public TwoWire, public EmComDevice
{
public:
    EmComI2C(uint8_t bus_num)
    : TwoWire(bus_num)
    {}        
};
*/

#endif