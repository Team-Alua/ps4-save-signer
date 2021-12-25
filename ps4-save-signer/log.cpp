#include "log.hpp"

void Log(const char * msg, ...) {
#ifdef _DEBUG
    char formatMsg[500];
    memset(&formatMsg, 0, sizeof(formatMsg));
    strncpy(formatMsg, "[SAVE SIGNER] ", sizeof("[SAVE SIGNER] "));
    strncat(formatMsg, msg, strlen(msg));
    strcat(formatMsg, "\n");
    char outMsg[500];
    va_list argptr;
    va_start(argptr, msg);
    vsnprintf(outMsg, sizeof(outMsg), formatMsg, argptr);
    va_end(argptr);
    sceKernelDebugOutText(0, outMsg);
#endif
}
