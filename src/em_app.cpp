#include "em_app.h"

void delay(uint32_t);

// NOTE: We only handle one application object!
bool running = true;
bool restart = false;
EmAppInterfaces runningInterfaces;      

void EmApp::Run(uint32_t loopDelayMs) {
    // Create a copy of the running interfaces list just in case some interfaces and
    // removed and then app is asked to restart -> need all original interfaces list!
    runningInterfaces.Append(m_Interfaces);

    // The app running loop 
    while (running) {
        // Iteration of each interface
        bool iterResult = runningInterfaces.ForEach([](EmAppInterface& interface) -> EmIterResult {
            EmIntOperationResult res;
            if (interface.IsInitialized()) {
                res = interface.CanCallLoop() ? interface.Loop() : EmIntOperationResult::canContinue;
            } else {
                res = interface.Setup();
                if (res == EmIntOperationResult::canContinue) {
                    interface.SetInitialized(true);
                }
            }
            switch (res) {
                case EmIntOperationResult::removeInterface:
                    return EmIterResult::removeMoveNext;
                case EmIntOperationResult::restartApp:
                    restart = true;
                    return EmIterResult::stopFailed;
                case EmIntOperationResult::exitApp:
                    running = false;
                    return EmIterResult::stopFailed;
                case EmIntOperationResult::canContinue:
                    break; // Just to keep compiler happy
            }
            return EmIterResult::moveNext;
        });  
        // Restart application requested?
        if (restart) {
            // Restore all application interfaces and set them as "uninitialized"
            restart = false;
            runningInterfaces.Clear();
            m_Interfaces.ForEach([](EmAppInterface& interface) -> EmIterResult {
                interface.SetInitialized(false);
                runningInterfaces.Append(interface);
                return EmIterResult::moveNext;
            });
        } 
        // We do wait next iteration in case of iteration success
        else if (loopDelayMs && iterResult) {      
            delay(loopDelayMs);
        }
    }
}
