#pragma once

#include <ftw.h>

#include <string.h>
#include <sys/stat.h>

#include <dirent.h>
#include <errno.h>

#include <cstdint>

#include <stdint.h>

#include <sys/types.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <unistd.h>

#include <vector>
#include <string>

#include "util.hpp"
#include "log.hpp"
#include "cmd_constants.hpp"

struct __attribute((packed)) PacketHeader {
    uint32_t magic;
    uint32_t cmd;
    uint32_t size;
};

std::string getRandomFileName(uint8_t size);

int getPacketHeader(int connfd, PacketHeader * packetHeader);

ssize_t readFull(int connfd, void * buffer, size_t size);

ssize_t sendStatusCode(int connfd, uint32_t command);

void _mkdir(const char *dir);

bool directoryExists(const char * directoryName);

int recursiveCopy(const char * sourceDirectoryPath, const char * targetDirectoryPath);

int recursiveDelete(const char * sourceDirectoryPath);

int recursiveList(const char * sourceDirectoryPath, const char * baseDirectory, std::vector<std::string> & files);

static int _transferFile(int connfd, int fd, size_t size);

int transferFile(int connfd, const char * filePath, std::string fileName);

int transferFiles(int connfd, const char * baseDirectory, std::vector<std::string> relFilePaths, std::vector<std::string> outPaths);

long getFileSize(const char *filename);

void downloadFileTo(int connfd, const char * basePath, const char * filename, uint32_t filesize);