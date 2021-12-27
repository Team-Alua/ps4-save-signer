#include "cmd_savegen.hpp"
#include <inttypes.h>

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
        mount.mountMode |= ORBIS_SAVE_DATA_MOUNT_MODE_CREATE2;
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
                saveModError = setParamResult;
                break;
            }

            if (strlen(saveGenPacket->zipname) > 0) {
                char zipPath[256];
                memset(zipPath, 0, sizeof(zipPath));
                strcpy(zipPath, "/data/teamalua/uploads/");
                strcat(zipPath, saveGenPacket->zipname);
                
                int zipExtractStatus = zip_extract(zipPath, saveMountDirectory, NULL, NULL);
                // TODO: Get return code and check if it was removed
                if (zipExtractStatus < 0) {
                    saveModError = errno;
                    break;
                }
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
        } else {
            sendStatusCode(connfd, saveModError);
        }

        if (success) {
            std::vector<std::string> relFiles;
            std::vector<std::string> outFiles;


            const char * tmpDirectory = "/data/teamalua/temp/";
            char outZipPath[48];
            memset(outZipPath, 0, sizeof(outZipPath));
            strcpy(outZipPath, tmpDirectory);
            mkdir(outZipPath, 0777);

            char outZipFileName[48];
            memset(outZipFileName, 0, sizeof(outZipFileName));
            sprintf(outZipFileName, "%s.zip", getRandomFileName(8).c_str());

            strcat(outZipPath, outZipFileName);

            // TODO: zip this up on the PS4 and send
            char baseDirectory[128];
            memset(baseDirectory, 0, sizeof(baseDirectory));
            sprintf(baseDirectory,"/user/home/%x/savedata/%s", getUserId(), saveGenPacket->titleId);

            char baseExportDirectory[128];
            memset(baseExportDirectory, 0, sizeof(baseExportDirectory));
            sprintf(baseExportDirectory, "PS4/SAVEDATA/%016lx/%s", saveGenPacket->psnAccountId, saveGenPacket->titleId);


            char pfsPath[128];
            memset(pfsPath, 0, sizeof(pfsPath));            
            sprintf(pfsPath, "%s/%s.bin", baseDirectory, saveGenPacket->dirName);
            relFiles.push_back(pfsPath);

            char binPath[128];
            memset(binPath, 0, sizeof(binPath));
            sprintf(binPath, "%s/sdimg_%s", baseDirectory, saveGenPacket->dirName);
            relFiles.push_back(binPath);

            char pfsOutPath[128];
            memset(pfsOutPath, 0, sizeof(pfsPath));            
            sprintf(pfsOutPath, "%s/%s.bin", baseExportDirectory, saveGenPacket->dirName);
            outFiles.push_back(pfsOutPath);

            char binOutPath[128];
            memset(binOutPath, 0, sizeof(binOutPath));
            sprintf(binOutPath, "%s/%s", baseExportDirectory, saveGenPacket->dirName);
            outFiles.push_back(binOutPath);
            // TODO: If this fails then it will not delete the mounted save 
            if (zip_partial_directory(outZipPath, relFiles, outFiles) != 0) {
                sendStatusCode(connfd, errno);
                break;
            } else {
                sendStatusCode(connfd, CMD_STATUS_READY);
            }

            if (transferFile(connfd, outZipPath, outZipFileName) != 0) {
                break;
            }
        }

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