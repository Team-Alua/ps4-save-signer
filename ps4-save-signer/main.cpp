
#include <condition_variable>
#include <thread>
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


int deleteAllSavesForUser() {

    int32_t result = sceSaveDataDeleteAllUser();
    printf("sceSaveDataDeleteAllUser ret=%d\n", result);
    return 0;
}

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
        

        OrbisSaveDataMount * a = new OrbisSaveDataMount;
        memset(a, 0, sizeof(OrbisSaveDataMount));
        a->userId = getUserId();
        a->dirName = (char *) "data0000";
        a->fingerprint = (char *) "0000000000000000000000000000000000000000000000000000000000000000";
        a->titleId = (char *) "BREW00085";
        a->blocks = 32768;
        a->mountMode = 1;
        

        OrbisSaveDataMountResult * b = new OrbisSaveDataMountResult;
        memset(b, 0, sizeof(OrbisSaveDataMountResult));

        int32_t mountErrorCode = sceSaveDataMount(a, b);
        if (mountErrorCode != 0) {
            delete a;
            delete b;
            char msg[100];
            memset(msg, 0, 100);
            snprintf(msg, 100, "sceSaveDataMount ret=%s\n", errorCodeToString(mountErrorCode));
            NOTIFY_CONST(msg);
            break;
        }
        OrbisSaveDataUMount * c = new OrbisSaveDataUMount;
        memset(c, 0, sizeof(OrbisSaveDataUMount));

        memcpy(c->mountPathName, b->mountPathName, sizeof(b->mountPathName));
        int32_t umountErrorCode = sceSaveDataUmount(c);
        if (umountErrorCode != 0) {
            delete c;
            char msg[100];
            memset(msg, 0, 100);
            snprintf(msg, 100, "sceSaveDataUmount ret=%s\n", errorCodeToString(umountErrorCode));
            NOTIFY_CONST(msg);
            break;
        }

        delete a;
        delete b;
        delete c;

    } while (false);

    // sceSaveDataUmount();
    
    for(;;); 
}
