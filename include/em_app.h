#ifndef __EM_APP__H_
#define __EM_APP__H_

#include "em_defs.h"
#include "em_log.h"
#include "em_app_interface.h"

// This is the application class you can run withing your code.
//
// By using an EmApp object you can manage multiple application interfaces. 
// Each interface will setup and run the main loop. Interfaces can drive the application
// workflow by removing themselves and stopping or restarting the application.
//
// Setup of each assigned interface is called until it return without failure.
// Once setup of interface is successful, its loop function is called.
//
// Use 'EmAppTaskInterface' or 'EmAppTaskInterfaces' to have interfaces running in a 
// separate task (i.e. not the application main loop.)
class EmApp: public EmAppInterfaces, public EmLog
{
public:
    EmApp(const char* logContext = "App", 
          EmLogLevel logLevel = EmLogLevel::global) : 
        EmAppInterfaces(), 
        EmLog(logContext, logLevel) {};
    
    virtual ~EmApp() {
        m_runningInterfaces.clear();
    }

    // Adds an interface object to the application.
    // NOTE that the object will NOT be owned by the application 
    // so it must outlive the application.
    virtual void addInterface(EmAppInterface& interface) {
        // TODO: add this to 'm_runningInterfaces' if this app is already "running" 
        //       (i.e. setup already called!) in  thread safe mode!
        appendUnowned(interface);
    }

    // Add multiple interfaces to the application using a variable argument list. 
    // NOTE: last argument MUST be a nullptr.
    void addInterfaces(EmAppInterface* interface, ...) {
        // TODO: add this to 'm_runningInterfaces' if this app is already "running" 
        //       (i.e. setup already called!) in  thread safe mode!
        va_list args;
        va_start(args, interface);
        extend(false, interface, args);
        va_end(args);
    }

    virtual void setup() { setup_(); }
    virtual void loop() { loop_(); }

    virtual void beforeInterfacesSetup() {
        // Do some preparation if needed
    }

    virtual void afterInterfacesSetup() {
        // Do some preparation if needed
    }

    virtual void beforeInterfacesLoop() {
        // Do something before each loop if needed
    }

    virtual void afterInterfacesLoop() {
        // Do something after each loop if needed
    }


    // Called before application restarts or stops due to an interface requesting
    // 'EmIntOperationResult::restartApp' or 'EmIntOperationResult::stopApp'
    virtual void onStop(EmIntOperationResult /*reason*/) { 
        // Do some cleanup if needed
    }

    bool isRunning() const {
        return !m_runningInterfaces.isEmpty();
    }

protected:
    virtual void setup_();
    virtual void loop_();
    virtual void stop_(EmIntOperationResult reason);

    EmAppInterfaces m_runningInterfaces;
};

#endif