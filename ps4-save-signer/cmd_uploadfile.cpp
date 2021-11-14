#include "cmd_uploadfile.hpp"
#include <errno.h>

#define MAX_FILENAME_SIZE 64


static void copyFileTo(int connfd, const char * filename, uint32_t filesize);

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

    copyFileTo(connfd, filename, filesize);
}


static void copyFileTo(int connfd, const char * filename, uint32_t filesize) {
    char filepath[256];
    memset(filepath, 0, 256);
    const char * basePath = "/data/teamalua/uploads/";
    strcpy(filepath, basePath);
    strcat(filepath, filename);

    const int fileLength = strlen(filepath);

    char fileParentPath[256];
    memset(fileParentPath, 0, sizeof(fileParentPath));
    
    int slashIndex = fileLength - 1;

    for (;slashIndex >= 0; slashIndex--) {
        if (filepath[slashIndex] == '/') {
            break;
        }
    }
    strncpy(fileParentPath, filepath, slashIndex + 1);
    _mkdir(fileParentPath);

    int fd = open(filepath, O_CREAT | O_EXCL | O_WRONLY, 0777);

    if (fd < 0) {
        sendStatusCode(connfd, CMD_UPLOAD_FILE_OPEN_FAILED);
        NOTIFY(50, "open error: %d %d", errno, fd);
        return; 
    }

    uint8_t buffer[8192];

    uint32_t bytesRemaining = filesize;
    bool success = true;

    do {
        if (bytesRemaining == 0) {
            break;
        }
        size_t fileBufSize = 8192;
        
        if (bytesRemaining < fileBufSize) {
            fileBufSize = bytesRemaining;
        }
        
        ssize_t received = readFull(connfd, buffer, fileBufSize);
        if (received <= 0) {
            success = false;
            NOTIFY(50, "read socket error: %d", errno);
            break;
        }
        size_t fileOffset = filesize - bytesRemaining;
        ssize_t writeError = pwrite(fd, buffer, received, fileOffset);
        if (writeError <= 0) {
            success = false;
            // NOTIFY(50, "write file error: %d %d", errno, fd);
            break;
        }
        bytesRemaining -= received;
    } while(true);
    


    close(fd);
    
    if (!success) {
        unlink(filepath);
    }
    sendStatusCode(connfd, CMD_STATUS_READY);
}