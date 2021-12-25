#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

#include "server.hpp"

#include "cmd_utils.hpp"
#include "cmd_constants.hpp"
#include "log.hpp"

#pragma once


#define MAX_FILENAME_SIZE 64

void handleUploadFile(int connfd, PacketHeader * pHeader);
