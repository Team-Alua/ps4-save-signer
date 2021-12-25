#include "cmd_uploadfile.hpp"

void handleUploadFile(int connfd, PacketHeader * pHeader) {
    // size will be string size
    if (pHeader->size > MAX_FILENAME_SIZE - 1) {
        sendStatusCode(connfd, CMD_PARAMS_INVALID);
        return;
    } else {
        sendStatusCode(connfd, CMD_STATUS_READY);
    }
    Log("Received upload request");

    char filename[MAX_FILENAME_SIZE]; 
    memset(&filename, 0, MAX_FILENAME_SIZE);
    ssize_t readStatus = readFull(connfd, &filename, pHeader->size);
    // It currently fails silently should add status check here
    if (readStatus <= 0) {
        Log("Failed to read filename for upload request: %l", readStatus);
        sendStatusCode(connfd, readStatus);
        return;
    } else {
        sendStatusCode(connfd, CMD_STATUS_READY);
    }
    Log("Successfully got file name.");
    
    uint32_t filesize;
    readStatus = readFull(connfd, &filesize, sizeof(uint32_t));

    if (readStatus <= 0) {
        Log("Failed to read filesize for upload request: %l", readStatus);
        sendStatusCode(connfd, readStatus);
        return;
    } else {
        sendStatusCode(connfd, CMD_STATUS_READY);
    }
    
    Log("Successfully got file size.");


    // TODO: log stuff in here
    // TODO: ensure filename does not contain any dots
    downloadFileTo(connfd, "/data/teamalua/uploads/", filename, filesize);
}
