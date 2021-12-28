#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#ifdef __DEBUG_LOG_FILE
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <mutex> 

#endif

#ifdef __DEBUG_LOG_KERNEL
#include <orbis/libkernel.h>
#endif


#pragma once

void Log(const char * msg, ...);