#include "cmd_deleteupload.hpp"

void handleUploadDelete(int connfd, PacketHeader * pHeader) {
    // size will be string size
    if (pHeader->size > MAX_FILENAME_SIZE - 1) {
        sendStatusCode(connfd, CMD_PARAMS_INVALID);
        return;
    } else {
        sendStatusCode(connfd, CMD_STATUS_READY);
    }
    
    char fileName[MAX_FILENAME_SIZE]; 
    memset(&fileName, 0, MAX_FILENAME_SIZE);

    ssize_t readStatus = readFull(connfd, &fileName, pHeader->size);

    if (readStatus <= 0) {
        sendStatusCode(connfd, UNEXPECTED_ERROR);
        return;
    } else {
        sendStatusCode(connfd, CMD_STATUS_READY);
    }

    // TODO: add check for /../ or ../ within path

    char targetFile[256];
    memset(&targetFile, 0, 256);
    sprintf(targetFile, "/data/teamalua/uploads/%s", fileName);

    int deleteResult = remove(targetFile);

    if (deleteResult < 0) {
        sendStatusCode(connfd, errno);
    } else {
        sendStatusCode(connfd, CMD_STATUS_READY);
    }
}