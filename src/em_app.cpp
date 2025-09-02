#include "em_app.h"


void EmApp::setup_() {
    m_runningInterfaces.set(m_appInterfaces, false);
    beforeInterfacesSetup();
    loop();  // This will call the 'setup' method of each interface
    afterInterfacesSetup();
}

void EmApp::loop_() {
    struct LoopContext {
        EmApp* self;
        EmIntOperationResult res;
    };
    LoopContext context = { this, EmIntOperationResult::canContinue };

    m_runningInterfaces.forEach<LoopContext>([](EmAppInterface& interface, bool, bool, LoopContext* pCtx) -> EmIterResult {
            if (!interface.isInitialized()) {
                pCtx->res = interface.setup();
                if (pCtx->res == EmIntOperationResult::canContinue) {
                    interface.setInitialized(true);
                }
            } else {
                if (interface.canCallLoop()) {
                    pCtx->res = interface.loop();
                } else {
                    pCtx->res = EmIntOperationResult::canContinue;
                }
            }
            switch (pCtx->res) {
                case EmIntOperationResult::stopInterface:
                    return EmIterResult::removeMoveNext;
                case EmIntOperationResult::restartApp:
                    return EmIterResult::stopFailed;
                case EmIntOperationResult::stopApp:
                    return EmIterResult::stopFailed;
                case EmIntOperationResult::canContinue:
                    break; // Just to keep compiler happy
            }
            // You can now access the EmApp instance via pCtx->self
            return EmIterResult::moveNext;
        }, &context);

    if (context.res == EmIntOperationResult::restartApp ||
        context.res == EmIntOperationResult::stopApp) {
        stop_(context.res);
    }
}

void EmApp::stop_(EmIntOperationResult reason) {
    struct LoopContext {
        EmApp* self;
        EmIntOperationResult reason;
    };
    LoopContext context = { this, reason };

    // No more running interfaces
    m_runningInterfaces.clear();
    // Notify all interfaces about stop event
    m_appInterfaces.forEach<LoopContext>(
        [](EmAppInterface& interface, bool, bool, LoopContext* pCtx) -> EmIterResult {
            interface.onStop(pCtx->reason);
            interface.setInitialized(false);
            if (pCtx->reason == EmIntOperationResult::stopApp) {
                return EmIterResult::removeMoveNext;
            } else if (pCtx->reason == EmIntOperationResult::restartApp) {
                pCtx->self->m_runningInterfaces.appendUnowned(interface);
                return EmIterResult::moveNext;
            }
            return EmIterResult::moveNext;
        }, &context);
    // On stop event
    onStop(reason);
}
