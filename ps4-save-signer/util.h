// Original: https://github.com/0x199/ps4-ipi/blob/main/Internal%20PKG%20Installer/util.h
#pragma once

enum OrbisSystemNotificationType
{
	NotificationRequest = 0,
	SystemNotification = 1,
	SystemNotificationWithUserId = 2,
	SystemNotificationWithDeviceId = 3,
	SystemNotificationWithDeviceIdRelatedToUser = 4,
	SystemNotificationWithText = 5,
	SystemNotificationWithTextRelatedToUser = 6,
	SystemNotificationWithErrorCode = 7,
	SystemNotificationWithAppId = 8,
	SystemNotificationWithAppName = 9,
	SystemNotificationWithAppInfo = 9,
	SystemNotificationWithAppNameRelatedToUser = 10,
	SystemNotificationWithParams = 11,
	SendSystemNotificationWithUserName = 12,
	SystemNotificationWithUserNameInfo = 13,
	SendAddressingSystemNotification = 14,
	AddressingSystemNotificationWithDeviceId = 15,
	AddressingSystemNotificationWithUserName = 16,
	AddressingSystemNotificationWithUserId = 17,

	UNK_1 = 100,
	TrcCheckNotificationRequest = 101,
	NpDebugNotificationRequest = 102,
	UNK_2 = 102,
};

struct OrbisSystemNotificationBuffer
{
	OrbisSystemNotificationType Type;
	int ReqId;
	int Priority;
	int MsgId;
	int TargetId;
	int UserId;
	int unk1;
	int unk2;
	int AppId;
	int ErrorNum;
	int unk3;
	char UseIconImageUri;
	char Message[1024];
	char Uri[1024];
	char unkstr[1024];
};
// update sceKernelSendNotificationRequest to int64_t (*sceKernelSendNotificationRequest)(int64_t unk1, char* Buffer, size_t size, int64_t unk2);
// in <orbis/libkernel.h>
int system_notification(const char* text, const char* iconName = "icon_system");


#define NOTIFY_CONST(msg) {\
        system_notification(msg, "icon_system");\
    }

#define NOTIFY(SIZE, ...) {\
        char error[SIZE];\
        sprintf(error, __VA_ARGS__);\
        system_notification(error, "icon_system");\
    }

