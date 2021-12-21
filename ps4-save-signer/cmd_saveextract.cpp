#include "cmd_saveextract.hpp"
#include "cmd_utils.hpp"
#include <vector>
#include <string>


struct __attribute((packed)) SaveExtractPacket {
    char dirName[0x20];
    char titleId[0x10];
    char copyDirectory[0x30];
    uint64_t saveBlocks;
};

static void doSaveExtract(int, SaveExtractPacket *);


void handleSaveExtract(int connfd, PacketHeader * pHeader) {
    
    sendStatusCode(connfd, CMD_STATUS_READY);
    SaveExtractPacket uploadPacket;
    ssize_t readStatus = readFull(connfd, &uploadPacket, sizeof(SaveExtractPacket));
    if (readStatus <= 0) {
        sendStatusCode(connfd, readStatus);
        return;
    } else if (readStatus < sizeof(SaveExtractPacket)) {
        sendStatusCode(connfd, CMD_PARAMS_INVALID);
        return;
    } else {
        sendStatusCode(connfd, CMD_STATUS_READY);    
    }

    doSaveExtract(connfd, &uploadPacket);
}

static void doSaveExtract(int connfd, SaveExtractPacket * saveExtractPacket) {
    do {
        // Create the save and then unmount it
        OrbisSaveDataMount tempMount;
        memset(&tempMount, 0, sizeof(OrbisSaveDataMount));

        tempMount.dirName = saveExtractPacket->dirName;
        tempMount.titleId = saveExtractPacket->titleId;
        tempMount.blocks = saveExtractPacket->saveBlocks;
        tempMount.mountMode = 8 | 4 | 2;

        OrbisSaveDataMountResult tempMountResult;
        memset(&tempMountResult, 0, sizeof(OrbisSaveDataMountResult));
        int32_t tempMountErrorCode = createSave(&tempMount, &tempMountResult);
        
        if (tempMountErrorCode < 0) {
            sendStatusCode(connfd, tempMountErrorCode);
            break;
        } else {
            sendStatusCode(connfd, CMD_STATUS_READY);
        }

        OrbisSaveDataUMount tempUmount;
        memset(&tempUmount, 0, sizeof(OrbisSaveDataUMount));

        memcpy(tempUmount.mountPathName, tempMountResult.mountPathName, sizeof(tempMountResult.mountPathName));
        int32_t tempUmountErrorCode = sceSaveDataUmount(&tempUmount);
        
        if (tempUmountErrorCode < 0) {
            sendStatusCode(connfd, tempUmountErrorCode);
            break;
        } else {
            sendStatusCode(connfd, CMD_STATUS_READY);
        }

        char copyFolder[256];
        memset(copyFolder, 0, sizeof(copyFolder));
        strcpy(copyFolder, "/data/teamalua/uploads/");
        strcat(copyFolder, saveExtractPacket->copyDirectory);
        strcat(copyFolder, "/");

        if (!directoryExists(copyFolder)) {
            sendStatusCode(connfd, CMD_SAVE_GEN_COPY_FOLDER_NOT_FOUND);
            break;
        } else {
            sendStatusCode(connfd, CMD_STATUS_READY);
        }

        // Update files
        char targetDirectory[256];

        memset(targetDirectory, 0, sizeof(targetDirectory));
        
        sprintf(targetDirectory,"/user/home/%x/savedata/%s/", getUserId(), saveExtractPacket->titleId);
    
        int recCopyResult = recursiveCopy(copyFolder, targetDirectory);
        
        if (recCopyResult != 0) {
            sendStatusCode(connfd, recCopyResult);
            break;
        } else {
            sendStatusCode(connfd, CMD_STATUS_READY);
        }

        OrbisSaveDataMount mount;
        // Send files back
        memset(&mount, 0, sizeof(OrbisSaveDataMount));

        mount.dirName = saveExtractPacket->dirName;
        mount.titleId = saveExtractPacket->titleId;
        mount.blocks = saveExtractPacket->saveBlocks;
        mount.mountMode = 8 | 1;

        OrbisSaveDataMountResult mountResult;
        memset(&mountResult, 0, sizeof(OrbisSaveDataMountResult));
        int32_t mountErrorCode = createSave(&mount, &mountResult);
        
        if (mountErrorCode < 0) {
            sendStatusCode(connfd, mountErrorCode);
            break;
        } else {
            sendStatusCode(connfd, CMD_STATUS_READY);
        }

        // send back all files found
        do {
            std::vector<std::string> folders;
            char baseFolder[256];
            memset(baseFolder, 0, sizeof(baseFolder));
            strcpy(baseFolder, "/mnt/sandbox/BREW00085_000");
            strcat(baseFolder, mountResult.mountPathName);
            strcat(baseFolder, "/");
            int result = recursiveList(baseFolder, "", folders);
            if (result != 0) {
                // report this as an error
                break;
            }

            if (transferFiles(connfd, baseFolder, folders, folders) != 0) {
                // report this as an error
                break;
            }

            // report as success here
        } while(0);

        OrbisSaveDataUMount umount;
        memset(&umount, 0, sizeof(OrbisSaveDataUMount));

        memcpy(umount.mountPathName, mountResult.mountPathName, sizeof(mountResult.mountPathName));
        int32_t umountErrorCode = sceSaveDataUmount(&umount);
        
        if (umountErrorCode < 0) {
            sendStatusCode(connfd, umountErrorCode);
            break;
        } else {
            sendStatusCode(connfd, CMD_STATUS_READY);
        }


        // Delete save
        OrbisSaveDataDelete del;
        memset(&del, 0, sizeof(OrbisSaveDataDelete));
        del.userId = getUserId();
        del.dirName = saveExtractPacket->dirName;
        del.titleId = saveExtractPacket->titleId;
        int32_t deleteUserSaves = sceSaveDataDelete(&del);
        if (deleteUserSaves < 0) {
            sendStatusCode(connfd, deleteUserSaves);
            break;
        } else {
            sendStatusCode(connfd, CMD_STATUS_READY);
        }

    } while(false);
}