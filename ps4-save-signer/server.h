#pragma once
#include <stdio.h>
#include <string.h>
#include <thread>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <orbis/libkernel.h>

#include "../../_common/log.h"


#include "util.h"



void serverThread();