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

#pragma once

void handleSaveExtract(int connfd, PacketHeader * pHeader);
