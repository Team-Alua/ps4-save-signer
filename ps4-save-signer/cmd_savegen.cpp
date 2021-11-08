#include "cmd_savegen.hpp"
#include "cmd_utils.hpp"
#include <vector>
#include <string>

struct __attribute((packed)) SaveGeneratorPacket {
    char dirName[0x20];
    char titleId[0x10];
    char copyDirectory[0x30];
};

static void doSaveGenerator(int, SaveGeneratorPacket *);




void handleSaveGenerating(int connfd, PacketHeader * pHeader) {
    sendStatusCode(connfd, CMD_STATUS_READY);

    SaveGeneratorPacket uploadPacket;
    ssize_t readStatus = readFull(connfd, &uploadPacket, sizeof(SaveGeneratorPacket));
    
    if (readStatus < sizeof(SaveGeneratorPacket)) {
        sendStatusCode(connfd, CMD_PARAMS_INVALID);
        return;
    }

    doSaveGenerator(connfd, &uploadPacket);
}


static void doSaveGenerator(int connfd, SaveGeneratorPacket * saveGenPacket) {
    do {
        char templateFolder[256];
        memset(templateFolder, 0, sizeof(templateFolder));
        strcpy(templateFolder, "/data/teamalua/templates/");
        strcat(templateFolder, saveGenPacket->titleId);
        strcat(templateFolder, "/");
        if (!directoryExists(templateFolder)) {
            sendStatusCode(connfd, CMD_SAVE_GEN_TITLE_ID_UNSUPPORTED);
            return;
        }

        char copyFolder[256];
        memset(copyFolder, 0, sizeof(copyFolder));
        strcpy(copyFolder, "/data/teamalua/uploads/");
        strcat(copyFolder, saveGenPacket->copyDirectory);
        strcat(copyFolder, "/");

        if (!directoryExists(copyFolder)) {
            sendStatusCode(connfd, CMD_SAVE_GEN_COPY_FOLDER_NOT_FOUND);
            return;
        }
        sendStatusCode(connfd, CMD_STATUS_READY);


        OrbisSaveDataMount mount;
        memset(&mount, 0, sizeof(OrbisSaveDataMount));

        char fingerprint[80];
        memset(fingerprint, 0, sizeof(fingerprint));
        strcpy(fingerprint, "0000000000000000000000000000000000000000000000000000000000000000");

        mount.userId = getUserId();
        mount.dirName = saveGenPacket->dirName;
        mount.fingerprint = fingerprint;
        mount.titleId = saveGenPacket->titleId;
        mount.blocks = 114;
        mount.mountMode = 8 | 4 | 2;
        
        OrbisSaveDataMountResult mountResult;
        memset(&mountResult, 0, sizeof(OrbisSaveDataMountResult));

        int32_t mountErrorCode = sceSaveDataMount(&mount, &mountResult);
        if (mountErrorCode < 0) {
            sendStatusCode(connfd, CMD_SAVE_GEN_MOUNT_ERROR);
            return;
        }
 
        char targetDirectory[256];
        memset(targetDirectory, 0, sizeof(targetDirectory));
        strcpy(targetDirectory, "/mnt/sandbox/BREW00085_000");
        strcat(targetDirectory, mountResult.mountPathName);
        strcat(targetDirectory, "/");

        recursiveDelete(targetDirectory);

        bool success = false;
        do {
            if (recursiveCopy(templateFolder, targetDirectory) != 0) {
                break;
            }
            if (recursiveCopy(copyFolder, targetDirectory) != 0) {
                break;
            }
            success = true;
        } while(0);

        OrbisSaveDataUMount umount;
        memset(&umount, 0, sizeof(OrbisSaveDataUMount));

        memcpy(umount.mountPathName, mountResult.mountPathName, sizeof(mountResult.mountPathName));
        int32_t umountErrorCode = sceSaveDataUmount(&umount);
        
        if (umountErrorCode < 0) {
            sendStatusCode(connfd, CMD_SAVE_GEN_UMOUNT_ERROR);
            return;
        }
        
        if (success) {
            sendStatusCode(connfd, CMD_STATUS_READY);
            recursiveDelete(copyFolder);

            std::vector<std::string> files;
            char baseDirectory[256];

            char pfsPath[256];
            char binPath[256];
            
            memset(baseDirectory, 0, sizeof(baseDirectory));
            memset(pfsPath, 0, sizeof(pfsPath));
            memset(binPath, 0, sizeof(binPath));

            sprintf(baseDirectory,"/user/home/%x/savedata/", getUserId());
            
            strcpy(pfsPath, saveGenPacket->titleId);
            strcat(pfsPath, "/");
            strcat(pfsPath, saveGenPacket->dirName);
            strcat(pfsPath, ".bin");

            strcpy(binPath, saveGenPacket->titleId);
            strcat(binPath, "/sdimg_");
            strcat(binPath, saveGenPacket->dirName);

            files.push_back(std::string(pfsPath));
            files.push_back(std::string(binPath));
            transferFiles(connfd, baseDirectory, files);
        } else {
            sendStatusCode(connfd, CMD_SAVE_GEN_COPY_FOLDER_FAILED);
        }


        OrbisSaveDataDelete del;
        memset(&del, 0, sizeof(OrbisSaveDataDelete));
        del.userId = getUserId();
        del.dirName = saveGenPacket->dirName;
        del.titleId = saveGenPacket->titleId;
        int32_t deleteUserSaves = sceSaveDataDelete(&del);
        if (deleteUserSaves < 0) {
            sendStatusCode(connfd, CMD_SAVE_GEN_DELETE_MOUNT_ERROR);
            return;
        }

        sendStatusCode(connfd, CMD_STATUS_READY);
    } while (false);
}