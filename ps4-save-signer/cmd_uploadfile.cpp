#include "cmd_uploadfile.hpp"
#include <errno.h>

#define MAX_FILENAME_SIZE 64


static void downloadFileTo(int connfd, const char * filename, uint32_t filesize);

void handleUploadFile(int connfd, PacketHeader * pHeader) {
    // size will be string size
    if (pHeader->size > MAX_FILENAME_SIZE - 1) {
        sendStatusCode(connfd, CMD_PARAMS_INVALID);
        return;
    } else {
        sendStatusCode(connfd, CMD_STATUS_READY);
    }

    char filename[MAX_FILENAME_SIZE]; 
    memset(&filename, 0, MAX_FILENAME_SIZE);
    ssize_t readStatus = readFull(connfd, &filename, pHeader->size);
    if (readStatus <= 0) {
        return;
    }
    uint32_t filesize;
    readStatus = readFull(connfd, &filesize, sizeof(uint32_t));

    if (readStatus <= 0) {
        return;
    }

    downloadFileTo(connfd, "/data/teamalua/uploads/", filename, filesize);
}
