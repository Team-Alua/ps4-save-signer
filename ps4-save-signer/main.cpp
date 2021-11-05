
#include <condition_variable>
#include <thread>
#include "define_headers.h"
#include "server.h"
#include "util.h"
#include "errcodes.hpp"

#include <orbis/libkernel.h>
#include <orbis/UserService.h>
#include <orbis/SaveData.h>

// Use this for jailbreaking: https://github.com/0x199/ps4-ipi/blob/main/Internal%20PKG%20Installer/modules.cpp

// Logging
std::stringstream debugLogStream;


int32_t getUserId() {
    int32_t outUserId;
    sceUserServiceGetInitialUser(&outUserId);
    return outUserId;
}

int initializeModules() {
    OrbisUserServiceInitializeParams params;

	memset(&params, 0, sizeof(params));
	params.priority = 700;

    int32_t userServiceResult = sceUserServiceInitialize(&params);

    if (userServiceResult != 0) {
        printf("Failed to initialize user service %d", userServiceResult);
        return -1;
    }
    
    int32_t saveDataInitializeResult = sceSaveDataInitialize3(0);

    if (saveDataInitializeResult != 0) {
        printf("Failed to initialize save data library %d", saveDataInitializeResult);
        return -1;
    }
    return 0;
}


bool(*jailbreak)();

int resolveDynamicLinks() {
	// https://github.com/sleirsgoevy/ps4-libjbc
	int libjbc = sceKernelLoadStartModule("/app0/sce_module/libjbc.sprx", 0, NULL, 0, NULL, NULL);
	if (libjbc == 0) {
		printf("sceKernelLoadStartModule() failed to load module %s\n", "libjbc.sprx");
		return -1;
	}
	
    sceKernelDlsym(libjbc, "Jailbreak", (void**)&jailbreak);
	if(jailbreak == nullptr) {
		printf("Failed to resolve symbol: %s\n", "Jailbreak");
		return -1;
	}
    return 0;
}

void thread1() {
    if (initializeModules() != 0) {
        return;
    }

    if (resolveDynamicLinks() != 0) {
        return;
    }
    jailbreak();
    do {
        sceKernelSleep(3);
        OrbisSaveDataMount mount;
        memset(&mount, 0, sizeof(OrbisSaveDataMount));
        
        char dirName[32];
        memset(dirName, 0, sizeof(dirName));
        strcpy(dirName, "data0000");

        char fingerprint[80];
        memset(fingerprint, 0, sizeof(fingerprint));
        strcpy(fingerprint, "0000000000000000000000000000000000000000000000000000000000000000");

        char titleId[16];
        memset(titleId, 0, sizeof(titleId));
        strcpy(titleId, "BREW00085");

        mount.userId = getUserId();
        mount.dirName = dirName;
        mount.fingerprint = fingerprint;
        mount.titleId = titleId;
        mount.blocks = 114;
        mount.mountMode = 8 | 2;
        
        OrbisSaveDataMountResult mountResult;
        memset(&mountResult, 0, sizeof(OrbisSaveDataMountResult));

        int32_t mountErrorCode = sceSaveDataMount(&mount, &mountResult);
        if (mountErrorCode != 0) {
            char msg[100];
            memset(msg, 0, 100);
            snprintf(msg, 100, "sceSaveDataMount ret=%s\n", errorCodeToString(mountErrorCode));
            NOTIFY_CONST(msg);
            continue;
        }

        OrbisSaveDataUMount umount;
        memset(&umount, 0, sizeof(OrbisSaveDataUMount));

        memcpy(umount.mountPathName, mountResult.mountPathName, sizeof(mountResult.mountPathName));
        int32_t umountErrorCode = sceSaveDataUmount(&umount);
        
        if (umountErrorCode != 0) {
            char msg[100];
            memset(msg, 0, 100);
            snprintf(msg, 100, "sceSaveDataUmount ret=%s\n", errorCodeToString(umountErrorCode));
            NOTIFY_CONST(msg);
            continue;
        }
        // sceKernelSleep(1);
        
        // int32_t deleteUserSaves = sceSaveDataDelelte(mount);
        // if (deleteUserSaves < 0) {
        //     char msg[100];
        //     memset(msg, 0, 100);
        //     snprintf(msg, 100, "sceSaveDataDeleteUser ret=%s\n", errorCodeToString(deleteUserSaves));
        //     NOTIFY_CONST(msg);
        //     continue;
        // }
    } while (true);
}

int main(void)
{
    // No buffering
    setvbuf(stdout, NULL, _IONBF, 0);

    std::thread t1(thread1);
    
    for(;;); 
}
