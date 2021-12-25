#include "cmd_constants.hpp"
#include "cmd_utils.hpp"
#include <string>

struct __attribute((packed)) SaveGeneratorPacket {
    char dirName[0x20];
};

#define MAX_FILENAME_SIZE 64


void handleDirectoryDelete(int connfd, PacketHeader * pHeader) {
    // size will be string size
    if (pHeader->size > MAX_FILENAME_SIZE - 1) {
        sendStatusCode(connfd, CMD_PARAMS_INVALID);
        return;
    } else {
        sendStatusCode(connfd, CMD_STATUS_READY);
    }
    
    char folderPath[MAX_FILENAME_SIZE]; 
    memset(&folderPath, 0, MAX_FILENAME_SIZE);

    ssize_t readStatus = readFull(connfd, &folderPath, pHeader->size);

    if (readStatus <= 0) {
        sendStatusCode(connfd, UNEXPECTED_ERROR);
        return;
    }

    // TODO: add check for /../ or ../ within path

    char targetFolder[256];
    memset(&targetFolder, 0, 256);
    strcpy(targetFolder, "/data/teamalua/uploads/");
    strcat(targetFolder, folderPath);

    int deleteResult = recursiveDelete(targetFolder);

    if (deleteResult < 0) {
        sendStatusCode(connfd, UNEXPECTED_ERROR);
        return;
    }

    sendStatusCode(connfd, CMD_STATUS_READY);
}