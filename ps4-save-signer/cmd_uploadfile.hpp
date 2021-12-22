#include <fcntl.h>
#include <sys/stat.h>


#include "server.hpp"

#include "cmd_utils.hpp"
#include "cmd_constants.hpp"



#pragma once

void handleUploadFile(int connfd, PacketHeader * pHeader);
