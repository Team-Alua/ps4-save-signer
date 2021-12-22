
#include <condition_variable>
#include <ftw.h>
#include <thread>
#include "util.hpp"
#include "server.hpp"
// Use this for jailbreaking: https://github.com/0x199/ps4-ipi/blob/main/Internal%20PKG%20Installer/modules.cpp

// Logging
std::stringstream debugLogStream;


int main(void)
{  
    
    // No buffering
    setvbuf(stdout, NULL, _IONBF, 0);
    do {

        if (initializeModules() != 0) {
            break;
        }

        if (resolveDynamicLinks() != 0) {
            break;
        }
        sceKernelDebugOutText(0, "[SAVE SIGNER] Loaded successfully!");
        jailbreak();
        std::thread t1(serverThread);
        t1.detach();
    } while(0);

    
    for(;;); 
}
