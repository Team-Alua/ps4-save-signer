#include <fcntl.h>
#include <sys/stat.h>
#include <sys/sendfile.h>

#include <vector>
#include <string>

#include <orbis/libkernel.h>

#include "zip.h"

#include "cmd_utils.hpp"
#include "cmd_constants.hpp"
#include "util.hpp"

#include "errcodes.hpp"

#pragma once

void handleSaveGenerating(int connfd, PacketHeader * pHeader);
