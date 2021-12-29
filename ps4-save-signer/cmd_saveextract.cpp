#include "cmd_saveextract.hpp"


static void doSaveExtract(int, SaveExtractPacket *);

static void onRealMount(int32_t errorCode, OrbisSaveDataMountResult& mountResult, void * args);


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
    Log("Successfully received command to extract save.");
    // TODO: make sure the last character \0
    Log("zip: %s titleId: %s dirName: %s saveBlocks: %l", uploadPacket.zipname, uploadPacket.titleId, uploadPacket.dirName, uploadPacket.saveBlocks);
    doSaveExtract(connfd, &uploadPacket);
}


static void doSaveExtract(int connfd, SaveExtractPacket * saveExtractPacket) {
    do {
        // should be able to use this multiple times
        char reusablePath[256];
        memset(reusablePath, 0, sizeof(reusablePath));

        char fingerprint[80];
        memset(fingerprint, 0, sizeof(fingerprint));
        strcpy(fingerprint, "0000000000000000000000000000000000000000000000000000000000000000");

        // Create the save and then unmount it
        OrbisSaveDataMount tempMount;
        memset(&tempMount, 0, sizeof(OrbisSaveDataMount));

        tempMount.dirName = saveExtractPacket->dirName;
        tempMount.titleId = saveExtractPacket->titleId;
        tempMount.blocks = saveExtractPacket->saveBlocks;
        tempMount.mountMode =  ORBIS_SAVE_DATA_MOUNT_MODE_DESTRUCT_OFF;
        tempMount.mountMode |= ORBIS_SAVE_DATA_MOUNT_MODE_CREATE2;
        tempMount.mountMode |= ORBIS_SAVE_DATA_MOUNT_MODE_RDWR;
        tempMount.userId = getUserId();
        tempMount.fingerprint = fingerprint;

        on_save_mount onTempMount = [](int32_t errorCode, OrbisSaveDataMountResult& mountResult, void * args){
            int connfd = *(int*)args;
            if (errorCode < 0) {
                Log("Mount failed %08x.", errorCode);
                sendStatusCode(connfd, errorCode);
            } else {
                Log("Mounted temp save.");
                sendStatusCode(connfd, CMD_STATUS_READY);
            }
        };

        on_save_unmount onTempUnmount = [](int32_t errorCode, void * args) {
            int connfd = *(int*)args;
            if (errorCode < 0) {
                Log("Unmount failed %i.", errorCode);
                sendStatusCode(connfd, errorCode);
            } else {
                Log("Unmounted save.");
                sendStatusCode(connfd, CMD_STATUS_READY);
            }
        };

        int32_t mumErroCode = saveMountUnMount(tempMount, onTempMount, &connfd, onTempUnmount, &connfd);
        
        if (mumErroCode < 0) {
            return;
        }

        memset(reusablePath, 0, sizeof(reusablePath));
        char * zipPath = reusablePath;
        strcpy(zipPath, "/data/teamalua/uploads/"); 
        strcat(zipPath, saveExtractPacket->zipname);

        char targetDirectory[48];
        memset(targetDirectory, 0, sizeof(targetDirectory));
        sprintf(targetDirectory,"/user/home/%x/savedata/%s/", getUserId(), saveExtractPacket->titleId);
        
        char zipTargetDirectory[80];
        memset(zipTargetDirectory, 0, sizeof(zipTargetDirectory));
        sprintf(zipTargetDirectory, "PS4/SAVEDATA/%016lx/%s/", saveExtractPacket->psnAccountId, saveExtractPacket->titleId);
        
        std::vector<std::string> zipPaths;
        std::vector<std::string> targetPaths;

        char pfsPath[128];
        memset(pfsPath, 0, sizeof(pfsPath));            
        sprintf(pfsPath, "%s%s.bin", zipTargetDirectory, saveExtractPacket->dirName);
        zipPaths.push_back(pfsPath);

        char binPath[128];
        memset(binPath, 0, sizeof(binPath));
        sprintf(binPath, "%s%s", zipTargetDirectory, saveExtractPacket->dirName);
        zipPaths.push_back(binPath);


        char pfsOutPath[128];
        memset(pfsOutPath, 0, sizeof(pfsOutPath));            
        sprintf(pfsOutPath, "%s%s.bin", targetDirectory, saveExtractPacket->dirName);
        targetPaths.push_back(pfsOutPath);

        char binOutPath[128];
        memset(binOutPath, 0, sizeof(binOutPath));
        sprintf(binOutPath, "%ssdimg_%s", targetDirectory, saveExtractPacket->dirName);
        targetPaths.push_back(binOutPath);

        auto zipExtractStatus = zip_partial_extract(zipPath, zipPaths, targetPaths);

        if (zipExtractStatus < 0) {
            // Delete save because extraction failed
            OrbisSaveDataDelete del;
            memset(&del, 0, sizeof(OrbisSaveDataDelete));
            del.userId = getUserId();
            del.dirName = (OrbisSaveDataDirName *) saveExtractPacket->dirName;
            del.titleId = (OrbisSaveDataTitleId *) saveExtractPacket->titleId;
            int32_t deleteUserSaves = sceSaveDataDelete(&del);
            Log("Failed to extract to %s!", targetDirectory);
        }
        
        if (zipExtractStatus < 0) {
            sendStatusCode(connfd, errno);
            break;
        } else {
            sendStatusCode(connfd, CMD_STATUS_READY);
        }

        Log("Extracted to %s !", targetDirectory);

        OrbisSaveDataMount mount;
        memset(&mount, 0, sizeof(OrbisSaveDataMount));

        mount.dirName = saveExtractPacket->dirName;
        mount.titleId = saveExtractPacket->titleId;
        mount.blocks = saveExtractPacket->saveBlocks;
        mount.mountMode =  ORBIS_SAVE_DATA_MOUNT_MODE_DESTRUCT_OFF;
        mount.mountMode |= ORBIS_SAVE_DATA_MOUNT_MODE_RDONLY;
        mount.userId = getUserId();
        mount.fingerprint = fingerprint;

        OrbisSaveDataMountResult mountResult;
        memset(&mountResult, 0, sizeof(OrbisSaveDataMountResult));

        on_save_unmount onRealUnmount = [](int32_t errorCode, void * args) {
            int connfd = *(int*)args;
            if (errorCode < 0) {
                Log("Unmount failed %i.", errorCode);
                sendStatusCode(connfd, errorCode);
            } else {
                Log("Unmounted save.");
                sendStatusCode(connfd, CMD_STATUS_READY);
            }
        };

        int32_t realMumErroCode = saveMountUnMount(tempMount, &onRealMount, &connfd, onRealUnmount, &connfd);
        
        if (realMumErroCode < 0) {
            return;
        }

        // Delete save
        OrbisSaveDataDelete del;
        memset(&del, 0, sizeof(OrbisSaveDataDelete));
        del.userId = getUserId();
        del.dirName = (OrbisSaveDataDirName *) saveExtractPacket->dirName;
        del.titleId = (OrbisSaveDataTitleId *) saveExtractPacket->titleId;
        int32_t deleteUserSaves = sceSaveDataDelete(&del);
        if (deleteUserSaves < 0) {
            sendStatusCode(connfd, deleteUserSaves);
            break;
        } else {
            sendStatusCode(connfd, CMD_STATUS_READY);
        }
        Log("Deleted save.");
    } while(false);
}


static void onRealMount(int32_t errorCode, OrbisSaveDataMountResult& mountResult, void * args)  {
    int connfd = *(int*)args;
    char mountFolder[256];
    memset(mountFolder, 0, sizeof(mountFolder));

    sprintf(mountFolder, "/mnt/sandbox/BREW00085_000%s/", mountResult.mountPathName);


    const char * tmpDirectory = "/data/teamalua/temp/";
    
    mkdir(tmpDirectory, 0777);

    char outZipFileName[14];
    memset(outZipFileName, 0, sizeof(outZipFileName));
    sprintf(outZipFileName, "%s.zip", getRandomFileName(8).c_str());
    
    char outZipPath[48];
    memset(outZipPath, 0, sizeof(outZipPath));
    sprintf(outZipPath, "%s%s", tmpDirectory, outZipFileName);

    if (zip_directory(outZipPath, mountFolder) != 0) {
        // It failed for a reason we do not care about 
        // Delete it
        sendStatusCode(connfd, errno);
        remove(outZipPath);
        return;
    } else {
        Log("Zipped file save.");
        sendStatusCode(connfd, CMD_STATUS_READY);
    }

    Log("Transferring dump back to client.");

    if (transferFile(connfd, outZipPath, outZipFileName) != 0) {
        Log("Transfer failed.");
        remove(outZipPath);
        return;
    }
    Log("Transfer successful.");
    remove(outZipPath);
}
