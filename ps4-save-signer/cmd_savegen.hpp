#include <fcntl.h>
#include <sys/stat.h>
#include <sys/sendfile.h>

#include <vector>
#include <string>

#include <orbis/libkernel.h>

#include "cmd_utils.hpp"
#include "cmd_constants.hpp"
#include "util.hpp"
#include "log.hpp"
#include "zip_util.hpp"

#include "errcodes.hpp"

#pragma once

struct __attribute__ ((packed)) SaveGeneratorPacket {
    uint64_t psnAccountId;
    char dirName[0x20];
    char titleId[0x10];
    uint64_t saveBlocks;
    char zipname[0x80];
    OrbisSaveDataParam saveParams;
};


void handleSaveGenerating(int connfd, PacketHeader * pHeader);
