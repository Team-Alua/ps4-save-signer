#include "cmd_savegen.hpp"

struct __attribute__ ((packed)) SaveGeneratorPacket {
    uint64_t psnAccountId;
    char dirName[0x20];
    char titleId[0x10];
    uint64_t saveBlocks;
    char zipname[0x30];
    OrbisSaveDataParam saveParams;
};

static void doSaveGenerator(int, SaveGeneratorPacket *);


void handleSaveGenerating(int connfd, PacketHeader * pHeader) {
    sendStatusCode(connfd, CMD_STATUS_READY);

    SaveGeneratorPacket uploadPacket;
    ssize_t readStatus = readFull(connfd, &uploadPacket, sizeof(SaveGeneratorPacket));
    
    if (readStatus <= 0) {
        sendStatusCode(connfd, readStatus);
        return;
    } else if (readStatus < sizeof(SaveGeneratorPacket)) {
        sendStatusCode(connfd, CMD_PARAMS_INVALID);
        return;
    } else {
        sendStatusCode(connfd, CMD_STATUS_READY);
    }

    doSaveGenerator(connfd, &uploadPacket);
}

static bool changeSaveAccountId(const char * baseMountDirectory, uint64_t accountId) {
    char paramSfoPath[256];
    memset(paramSfoPath, 0, sizeof(paramSfoPath));
    strcpy(paramSfoPath, baseMountDirectory);
    strcat(paramSfoPath, "sce_sys/param.sfo");
    int fd = open(paramSfoPath, O_RDWR, 0);
    if (fd < 0) {
        NOTIFY(300, "Failed to load %s %d", paramSfoPath, errno);
        return false;
    }
    uint8_t tryCount = 10;
    bool success = false;
    do {
        off_t fileSize = lseek(fd, 0x15C, SEEK_SET);
        if (fileSize == -1) {
            NOTIFY(300, "Failed to seek to 0x15C for %s", paramSfoPath);
            break;
        }
        ssize_t written = write(fd, &accountId, sizeof(accountId));
        if (written == sizeof(accountId)) {
            success = true;
            break;
        }
        tryCount--;
        if (tryCount == 0) {
            NOTIFY(300, "Failed to write 10 times for %s", paramSfoPath);
            break;
        }
    } while (true);
    fsync(fd);
    close(fd);
    return success;
}


static void doSaveGenerator(int connfd, SaveGeneratorPacket * saveGenPacket) {
    do {

        char fingerprint[80];
        memset(fingerprint, 0, sizeof(fingerprint));
        strcpy(fingerprint, "0000000000000000000000000000000000000000000000000000000000000000");

        // Create and mount template save
        OrbisSaveDataMount mount;
        memset(&mount, 0, sizeof(OrbisSaveDataMount));


        mount.dirName = saveGenPacket->dirName;
        mount.titleId = saveGenPacket->titleId;
        mount.blocks = saveGenPacket->saveBlocks;
        mount.mountMode =  ORBIS_SAVE_DATA_MOUNT_MODE_DESTRUCT_OFF;
        mount.mountMode |= ORBIS_SAVE_DATA_MOUNT_MODE_CREATE;
        mount.mountMode |= ORBIS_SAVE_DATA_MOUNT_MODE_RDWR;
        mount.userId = getUserId();
        mount.fingerprint = fingerprint;


        OrbisSaveDataMountResult mountResult;
        memset(&mountResult, 0, sizeof(OrbisSaveDataMountResult));
        

        int32_t mountErrorCode = sceSaveDataMount(&mount, &mountResult);
        
        if (mountErrorCode < 0) {
            sendStatusCode(connfd, mountErrorCode);
            break;
        } else {
            sendStatusCode(connfd, CMD_STATUS_READY);
        }

        // Create mount point structure for later use
        OrbisSaveDataMountPoint mp;
        memset(&mp, 0, sizeof(OrbisSaveDataMountPoint));
        strcpy(mp.data, mountResult.mountPathName);
        

        char saveMountDirectory[256];
        memset(saveMountDirectory, 0, sizeof(saveMountDirectory));
        strcpy(saveMountDirectory, "/mnt/sandbox/BREW00085_000");
        strcat(saveMountDirectory, mountResult.mountPathName);
        strcat(saveMountDirectory, "/");


        bool success = false;
        uint32_t saveModError = CMD_SAVE_GEN_PARMSFO_MOD_FAILED;
        do {
            if (!changeSaveAccountId(saveMountDirectory, saveGenPacket->psnAccountId)) {
                break;
            }

            memset(&saveGenPacket->saveParams.unknown1, 0, sizeof(saveGenPacket->saveParams.unknown1));
            memset(&saveGenPacket->saveParams.unknown2, 0, sizeof(saveGenPacket->saveParams.unknown2));
            saveGenPacket->saveParams.mtime = time(NULL);

            uint32_t type = ORBIS_SAVE_DATA_PARAM_TYPE_ALL;
            int32_t setParamResult = sceSaveDataSetParam(&mp, type, &saveGenPacket->saveParams, sizeof(saveGenPacket->saveParams));
            if (setParamResult < 0) {
                break;
            }
            saveModError = CMD_SAVE_GEN_COPY_FOLDER_FAILED;


            char zipPath[256];
            memset(zipPath, 0, sizeof(zipPath));
            strcpy(zipPath, "/data/teamalua/uploads/");
            strcat(zipPath, saveGenPacket->zipname);
            
            int zipExtractStatus = zip_extract(zipPath, saveMountDirectory, NULL, NULL);
            
            // TODO: Get return code and check if it was removed
            remove(zipPath);

            if (zipExtractStatus < 0) {
                break;
            }

            success = true;
        } while(0);
        

        int32_t umountErrorCode = sceSaveDataUmount(&mp);
        
        if (umountErrorCode < 0) {
            sendStatusCode(connfd, umountErrorCode);
            break;
        } else {
            sendStatusCode(connfd, CMD_STATUS_READY);
        }
        
        if (success) {
            sendStatusCode(connfd, CMD_STATUS_READY);

            // TODO: Maybe only send a zip?
            std::vector<std::string> inFiles;
            std::vector<std::string> outFiles;
            char baseDirectory[256];

            char pfsPath[256];
            char binPath[256];
            char binOutPath[256];
            
            memset(baseDirectory, 0, sizeof(baseDirectory));
            sprintf(baseDirectory,"/user/home/%x/savedata/%s/", getUserId(), saveGenPacket->titleId);


            memset(pfsPath, 0, sizeof(pfsPath));            
            strcpy(pfsPath, saveGenPacket->dirName);
            strcat(pfsPath, ".bin");


            memset(binPath, 0, sizeof(binPath));
            strcpy(binPath, "sdimg_");
            strcat(binPath, saveGenPacket->dirName);

            memset(binOutPath, 0, sizeof(binOutPath));
            strcpy(binOutPath, saveGenPacket->dirName);

            inFiles.push_back(std::string(pfsPath));
            outFiles.push_back(std::string(pfsPath));
            inFiles.push_back(std::string(binPath));
            outFiles.push_back(std::string(binOutPath));

            transferFiles(connfd, baseDirectory, inFiles, outFiles);
        } else {
            sendStatusCode(connfd, saveModError);
        }

        // delete zip file here

        OrbisSaveDataDelete del;
        memset(&del, 0, sizeof(OrbisSaveDataDelete));
        del.userId = getUserId();
        del.dirName = (OrbisSaveDataDirName *) saveGenPacket->dirName;
        del.titleId = (OrbisSaveDataTitleId *) saveGenPacket->titleId;
        int32_t deleteUserSaves = sceSaveDataDelete(&del);
        if (deleteUserSaves < 0) {
            sendStatusCode(connfd, deleteUserSaves);
            break;
        } else {
            sendStatusCode(connfd, CMD_STATUS_READY);
        }
    } while (false);
}