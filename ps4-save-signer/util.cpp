// Original: https://github.com/0x199/ps4-ipi/blob/main/Internal%20PKG%20Installer/util.cpp
#include "util.h"
#include "define_headers.h"

#include "string.h"
#include "stdio.h"
#include <orbis/libkernel.h>
#include <orbis/UserService.h>



int system_notification(const char* text, const char* iconName) {
	OrbisSystemNotificationBuffer NotificationBuffer;
	
	NotificationBuffer.Type = OrbisSystemNotificationType::NotificationRequest;
	NotificationBuffer.unk3 = 0; 
	NotificationBuffer.UseIconImageUri = 1;
	NotificationBuffer.TargetId = -1;
	
	snprintf(NotificationBuffer.Uri, sizeof(NotificationBuffer.Uri), "cxml://psnotification/tex_%s", iconName);
	strncpy(NotificationBuffer.Message, text, sizeof(NotificationBuffer.Message));
	
	sceKernelSendNotificationRequest(0, (char*)&NotificationBuffer, 3120, 0);
	
	return 0;
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


int32_t getUserId() {
    int32_t outUserId;
    sceUserServiceGetInitialUser(&outUserId);
    return outUserId;
}

bool (*jailbreak)();

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

