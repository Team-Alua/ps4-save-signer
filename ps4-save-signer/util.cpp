// Original: https://github.com/0x199/ps4-ipi/blob/main/Internal%20PKG%20Installer/util.cpp
#include "util.h"

#include "string.h"
#include "stdio.h"
#include <orbis/libkernel.h>


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


