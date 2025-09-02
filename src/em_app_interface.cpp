#include <string.h>
#include "em_app_interface.h"

void EmAppInterface::setWarning(bool value, const char* msg)
{
    // Any change? (avoid log same stuff many times)
    if (value == getStatusFlag_(EmInterfaceStatusFlag::hasWarning) &&
        ((msg == NULL && m_warningMsg[0]==0) ||
          0==strcmp(msg, m_warningMsg)))
        return;
    // Set new status
    setStatusFlag_(EmInterfaceStatusFlag::hasWarning, value); 
    memset(m_warningMsg, 0, sizeof(m_warningMsg));
    if (msg) 
    {
        strncpy(m_warningMsg, msg, MAX_INTERFACE_MSG_LEN); 
    }
}
    
void EmAppInterface::setError(bool value, const char* msg) 
{
    // Any change? (avoid log same stuff many times)
    if (value == getStatusFlag_(EmInterfaceStatusFlag::hasError) &&
        ((msg == NULL && m_errorMsg[0]==0) ||
          0==strcmp(msg, m_errorMsg)))
        return;
    // Set new status
    setStatusFlag_(EmInterfaceStatusFlag::hasError, value); 
    memset(m_errorMsg, 0, sizeof(m_errorMsg));
    if (msg) 
    {
        strncpy(m_errorMsg, msg, MAX_INTERFACE_MSG_LEN); 
    }
}
