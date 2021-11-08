#include <cinttypes>
#pragma once

typedef struct __attribute__((packed)) OrbisSaveDataMount {
	int32_t userId;
	int32_t unknown1;
	const char *titleId;
	const char *dirName;
	const char *fingerprint;
	uint64_t blocks;
	uint32_t mountMode;
	uint8_t reserved[32];
} OrbisSaveDataMount;



typedef struct __attribute__((packed)) OrbisSaveDataMountResult {
	char mountPathName[0x10];
	uint64_t requiredBlocks;
	uint32_t progress;
	uint8_t reserved[32];
} OrbisSaveDataMountResult;


struct OrbisSaveDataUMount 
{
    char mountPathName[0x10];
};

struct OrbisSaveDataDelete {
	int32_t userId;
	int32_t unknown1; 
	const char *titleId; 
	const char *dirName; 
	uint8_t reserved[32];
	int32_t unknown2;
};

extern "C" {
    int32_t sceSaveDataInitialize3(int32_t initParams);
    int32_t sceSaveDataMount(OrbisSaveDataMount*, OrbisSaveDataMountResult*);
    int32_t sceSaveDataUmount(OrbisSaveDataUMount *);
	int32_t sceSaveDataDelete(OrbisSaveDataDelete *del);
	int32_t sceSaveDataSetParam(char * mountPoint, uint32_t type, void * buffer, size_t bufferSize);
}
