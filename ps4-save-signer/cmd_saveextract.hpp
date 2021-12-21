#include <fcntl.h>
#include <sys/stat.h>


#include "server.h"

#include "cmd_utils.hpp"
#include "cmd_constants.hpp"

#include "define_headers.h"
#include "errcodes.hpp"

#include <orbis/libkernel.h>
#include <orbis/SaveData.h>

#include <sys/sendfile.h>


#pragma once

void handleSaveExtract(int connfd, PacketHeader * pHeader);
