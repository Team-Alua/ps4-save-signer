
#include "cmd_utils.hpp"
#include "zip_util.hpp"


#pragma once

struct __attribute__ ((packed)) SaveResignPacket {
    uint64_t originalPsnAccountId;
    uint64_t targetPsnAccountId;
    char dirName[0x20];
    char titleId[0x10];
    char zipname[0x80];
    uint64_t saveBlocks;
};


struct RealMountArgs {
    int connfd;
    SaveResignPacket * resignPacket;
    uint32_t errorCode;
};

void handleSaveResign(int connfd, PacketHeader * pHeader);

