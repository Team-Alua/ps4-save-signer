// Original: https://github.com/0x199/ps4-ipi/blob/main/Internal%20PKG%20Installer/util.h

#include <stdint.h>
#include "string.h"
#include "stdio.h"

#include <orbis/libkernel.h>
#include <orbis/UserService.h>
#include <orbis/SaveData.h>

#include "cmd_constants.hpp"
#include <vector>
#include <string>

#pragma once


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

int resolveDynamicLinks();
int initializeModules();

int32_t getUserId();

extern bool (*jailbreak)();
