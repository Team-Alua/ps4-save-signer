#include <sys/sendfile.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <vector>
#include <string>

#include <orbis/libkernel.h>
#include <orbis/SaveData.h>
#include <orbis/UserService.h>


#include "zip.h"

#include "cmd_constants.hpp"
#include "cmd_utils.hpp"
#include "errcodes.hpp"

#include "util.hpp"
#include "log.hpp"
#include "zip_util.hpp"

#pragma once

struct __attribute__ ((packed)) SaveExtractPacket {
    uint64_t psnAccountId;
    char dirName[0x20];
    char titleId[0x10];
    char zipname[0x30];
    uint64_t saveBlocks;
};


void handleSaveExtract(int connfd, PacketHeader * pHeader);
