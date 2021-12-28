#include "log.hpp"

#ifdef __DEBUG_LOG_FILE
std::mutex fileMtx;
#endif



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

#ifdef __DEBUG_LOG_FILE
    fileMtx.lock();
    do {
        FILE * logFile = fopen("/data/teamalua/log.txt", "a");
        size_t stringLen = strlen(outMsg);
        if (fwrite(outMsg, sizeof(char), stringLen, logFile) == -1) {
            // an error occured but we can't handle it.
        }
        fclose(logFile);
    } while(0);
    fileMtx.unlock();
#endif

#ifdef __DEBUG_LOG_KERNEL
    sceKernelDebugOutText(0, outMsg);
#endif

#endif
}
