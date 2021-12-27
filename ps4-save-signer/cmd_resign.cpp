#include "cmd_resign.hpp"

static void doSaveResign(int, SaveResignPacket *);


void handleSaveResign(int connfd, PacketHeader * pHeader) {
    
    sendStatusCode(connfd, CMD_STATUS_READY);
    
    SaveResignPacket uploadPacket;
    ssize_t readStatus = readFull(connfd, &uploadPacket, sizeof(SaveResignPacket));
    if (readStatus <= 0) {
        sendStatusCode(connfd, readStatus);
        return;
    } else if (readStatus < sizeof(SaveResignPacket)) {
        sendStatusCode(connfd, CMD_PARAMS_INVALID);
        return;
    } else {
        sendStatusCode(connfd, CMD_STATUS_READY);    
    }

    Log("Successfully received command to resign save.");
    Log("zip: %s titleId: %s dirName: %s saveBlocks: %li", uploadPacket.zipname, uploadPacket.titleId, uploadPacket.dirName, uploadPacket.saveBlocks);
    doSaveResign(connfd, &uploadPacket);
}

int on_extract_entry(const char *filename, void *arg) {
    Log("Extracting %s", filename);
    return 0;
}

void onRealMount(int32_t errorCode, OrbisSaveDataMountResult& mountResult, void * args) {
    RealMountArgs realArgs = *(RealMountArgs*)args;

    if (errorCode < 0) {
        Log("Mount failed %08x.", errorCode);
        sendStatusCode(realArgs.connfd, errorCode);
        return;
    } else {
        Log("Mounted real save.");
        sendStatusCode(realArgs.connfd, CMD_STATUS_READY);
    }
    char mountPath[126];
    memset(mountPath, 0, sizeof(mountPath));
    sprintf(mountPath, "/mnt/sandbox/BREW00085_000%s/", mountResult.mountPathName);
    if (!changeSaveAccountId(mountPath, realArgs.targetPsnAccountId)) {
        Log("Failed to change psn id from %016lx to %016lx", realArgs.originalPsnAccountId, realArgs.targetPsnAccountId);
        realArgs.errorCode = -1;
    }
}

void onTempMount(int32_t errorCode, OrbisSaveDataMountResult& mountResult, void * args) {
    int connfd = *(int*)args;
    if (errorCode < 0) {
        Log("Mount failed 0x%08x.", errorCode);
        sendStatusCode(connfd, errorCode);
    } else {
        Log("Mounted temp save.");
        sendStatusCode(connfd, CMD_STATUS_READY);
    }
}

void onUnmount(int32_t errorCode, void * args) {
    int connfd = *(int*)args;
    if (errorCode < 0) {
        Log("Unmount failed %i.", errorCode);
        sendStatusCode(connfd, errorCode);
    } else {
        Log("Unmounted save.");
        sendStatusCode(connfd, CMD_STATUS_READY);
    }
}

static void doSaveResign(int connfd, SaveResignPacket * saveResignPacket) {
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

        tempMount.dirName = saveResignPacket->dirName;
        tempMount.titleId = saveResignPacket->titleId;
        tempMount.blocks = saveResignPacket->saveBlocks;
        tempMount.mountMode =  ORBIS_SAVE_DATA_MOUNT_MODE_DESTRUCT_OFF;
        tempMount.mountMode |= ORBIS_SAVE_DATA_MOUNT_MODE_CREATE;
        tempMount.mountMode |= ORBIS_SAVE_DATA_MOUNT_MODE_RDWR;
        tempMount.userId = getUserId();
        tempMount.fingerprint = fingerprint;

        int32_t mumErroCode = saveMountUnMount(tempMount, &onTempMount, &connfd, &onUnmount, &connfd);
        
        if (mumErroCode < 0) {
            return;
        }

        memset(reusablePath, 0, sizeof(reusablePath));
        char * zipPath = reusablePath;
        strcpy(zipPath, "/data/teamalua/uploads/"); 
        strcat(zipPath, saveResignPacket->zipname);

        do {
            char targetDirectory[48];
            memset(targetDirectory, 0, sizeof(targetDirectory));
            sprintf(targetDirectory,"/user/home/%x/savedata/%s/", getUserId(), saveResignPacket->titleId);
            
            char zipTargetDirectory[80];
            memset(zipTargetDirectory, 0, sizeof(zipTargetDirectory));
            sprintf(zipTargetDirectory, "PS4/SAVEDATA/%016lx/%s/", saveResignPacket->originalPsnAccountId, saveResignPacket->titleId);
            
            std::vector<std::string> zipPaths;
            std::vector<std::string> targetPaths;

            char pfsPath[128];
            memset(pfsPath, 0, sizeof(pfsPath));            
            sprintf(pfsPath, "%s%s.bin", zipTargetDirectory, saveResignPacket->dirName);
            zipPaths.push_back(pfsPath);

            char binPath[128];
            memset(binPath, 0, sizeof(binPath));
            sprintf(binPath, "%s%s", zipTargetDirectory, saveResignPacket->dirName);
            zipPaths.push_back(binPath);


            char pfsOutPath[128];
            memset(pfsOutPath, 0, sizeof(pfsOutPath));            
            sprintf(pfsOutPath, "%s%s.bin", targetDirectory, saveResignPacket->dirName);
            targetPaths.push_back(pfsOutPath);

            char binOutPath[128];
            memset(binOutPath, 0, sizeof(binOutPath));
            sprintf(binOutPath, "%ssdimg_%s", targetDirectory, saveResignPacket->dirName);
            targetPaths.push_back(binOutPath);

            if (zip_partial_extract(zipPath, zipPaths, targetPaths) != 0) {
                sendStatusCode(connfd, errno);
                break;
            } else {
                sendStatusCode(connfd, CMD_STATUS_READY);
            }

            OrbisSaveDataMount mount;
            memset(&mount, 0, sizeof(OrbisSaveDataMount));

            mount.dirName = saveResignPacket->dirName;
            mount.titleId = saveResignPacket->titleId;
            mount.blocks = saveResignPacket->saveBlocks;
            mount.mountMode =  ORBIS_SAVE_DATA_MOUNT_MODE_DESTRUCT_OFF;
            mount.mountMode |= ORBIS_SAVE_DATA_MOUNT_MODE_RDWR;
            mount.userId = getUserId();
            mount.fingerprint = fingerprint;
            RealMountArgs realArgs;
            memset(&realArgs, 0, sizeof(realArgs));
            realArgs.connfd = connfd;
            realArgs.originalPsnAccountId = saveResignPacket->originalPsnAccountId;
            realArgs.targetPsnAccountId = saveResignPacket->targetPsnAccountId;

            int32_t mumErroCode = saveMountUnMount(mount, &onRealMount, &realArgs, &onUnmount, &connfd);

            if (mumErroCode < 0 || realArgs.errorCode < 0) {
                break;
            }

            const char * tmpDirectory = "/data/teamalua/temp/";
            char outZipPath[48];
            memset(outZipPath, 0, sizeof(outZipPath));
            strcpy(outZipPath, tmpDirectory);

            char outZipFileName[14];
            memset(outZipFileName, 0, sizeof(outZipFileName));
            sprintf(outZipFileName, "%s.zip", getRandomFileName(8).c_str());

            strcat(outZipPath, outZipFileName);
            


            if (zip_partial_directory(outZipPath, targetPaths, zipPaths) != 0) {
                sendStatusCode(connfd, errno);
                break;
            } else {
                sendStatusCode(connfd, CMD_STATUS_READY);
            }

            Log("Transferring resigned save back to client.");

            if (transferFile(connfd, outZipPath, outZipFileName) != 0) {
                Log("Transfer failed.");
                remove(outZipPath);
                break;
            }
            remove(outZipPath);

        } while (false);



        // Delete save
        OrbisSaveDataDelete del;
        memset(&del, 0, sizeof(OrbisSaveDataDelete));
        del.userId = getUserId();
        del.dirName = (OrbisSaveDataDirName *) saveResignPacket->dirName;
        del.titleId = (OrbisSaveDataTitleId *) saveResignPacket->titleId;
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
