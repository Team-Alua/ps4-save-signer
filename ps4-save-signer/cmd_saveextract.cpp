#include "cmd_saveextract.hpp"


struct __attribute__ ((packed)) SaveExtractPacket {
    char dirName[0x20];
    char titleId[0x10];
    char zipname[0x30];
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
    Log("Successfully received command to extract save.");
    Log("zip: %s titleId: %s dirName: %s saveBlocks: %l", uploadPacket.zipname, uploadPacket.titleId, uploadPacket.dirName, uploadPacket.saveBlocks);
    doSaveExtract(connfd, &uploadPacket);
}
int on_extract_entry(const char *filename, void *arg) {
    Log("Extracting %s", filename);
    return 0;
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
        tempMount.mountMode |= ORBIS_SAVE_DATA_MOUNT_MODE_CREATE;
        tempMount.mountMode |= ORBIS_SAVE_DATA_MOUNT_MODE_RDWR;
        tempMount.userId = getUserId();
        tempMount.fingerprint = fingerprint;

        OrbisSaveDataMountResult tempMountResult;
        memset(&tempMountResult, 0, sizeof(OrbisSaveDataMountResult));

        int32_t tempMountErrorCode = sceSaveDataMount(&tempMount, &tempMountResult);

        if (tempMountErrorCode < 0) {
            sendStatusCode(connfd, tempMountErrorCode);
            break;
        } else {
            sendStatusCode(connfd, CMD_STATUS_READY);
        }
        Log("Successfully temp save.");
    

        OrbisSaveDataMountPoint tempMp;
        memset(&tempMp, 0, sizeof(OrbisSaveDataMountPoint));
        strcpy(tempMp.data, tempMountResult.mountPathName);
        
        int32_t tempUmountErrorCode = sceSaveDataUmount(&tempMp);
        
        if (tempUmountErrorCode < 0) {
            sendStatusCode(connfd, tempUmountErrorCode);
            break;
        } else {
            sendStatusCode(connfd, CMD_STATUS_READY);
        }
        Log("Successfully unmounted temp save.");

        memset(reusablePath, 0, sizeof(reusablePath));
        char * zipPath = reusablePath;
        strcpy(zipPath, "/data/teamalua/uploads/"); 
        strcat(zipPath, saveExtractPacket->zipname);


        // Update files
        char targetDirectory[48];

        memset(targetDirectory, 0, sizeof(targetDirectory));
        
        // 48
        sprintf(targetDirectory,"/user/home/%x/savedata/%s/", getUserId(), saveExtractPacket->titleId);
        Log("Extracting %s to %s", zipPath, targetDirectory);
        
        // __asm__ __volatile("int3");

        int zipExtractStatus = zip_extract(zipPath, targetDirectory, on_extract_entry, NULL);
        if (zipExtractStatus < 0) {
            Log("Failed to extract to %s with error %ld !", targetDirectory, errno);
            sendStatusCode(connfd, errno);
            break;
        } else {
            sendStatusCode(connfd, CMD_STATUS_READY);
        }
        Log("Extracted to %s !", targetDirectory);


        OrbisSaveDataMount mount;
        // Send files back
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
        // /user/home/1388b6a0/savedata/CUSA05571
        int32_t mountErrorCode = sceSaveDataMount(&mount, &mountResult);

        if (mountErrorCode < 0) {
            sendStatusCode(connfd, mountErrorCode);
            break;
        } else {
            sendStatusCode(connfd, CMD_STATUS_READY);
        }
        Log("Success! Mount can be found at %s .", mountResult.mountPathName);


        do {
            // zip up all the files 
            std::vector<std::string> zipFileNames;
            memset(reusablePath, 0, sizeof(reusablePath));
            char * mountFolder = reusablePath;
            sprintf(mountFolder, "/mnt/sandbox/BREW00085_000%s/", mountResult.mountPathName);

            int result = recursiveList(mountFolder, "", zipFileNames);
            if (result != 0) {
                // report this as an error
                break;
            }
            Log("There are %i files in %s .", zipFileNames.size(), mountFolder);
            std::vector<std::string> absoluteFilePaths;
            char saveFilePath[128];
            for (int i = 0; i < zipFileNames.size(); i++) {
                memset(saveFilePath, 0, sizeof(saveFilePath));
                sprintf(saveFilePath, "%s%s", mountFolder, zipFileNames[i].c_str());
                absoluteFilePaths.push_back(std::string(saveFilePath));
            }


            const char * tmpDirectory = "/data/teamalua/temp/";
            char outZipPath[48];
            memset(outZipPath, 0, sizeof(outZipPath));
            mkdir(outZipPath, 0777);

            char outZipFileName[14];
            memset(outZipFileName, 0, sizeof(outZipFileName));
            sprintf(outZipFileName, "%s.zip", getRandomFileName(8).c_str());

            sprintf(outZipPath, "%s%s", tmpDirectory, outZipFileName);

            if (zip_partial_directory(outZipPath, absoluteFilePaths, zipFileNames) != 0) {
                break;
            }

            Log("Transferring dump back to client.");

            if (transferFile(connfd, outZipPath, outZipFileName) != 0) {
                Log("Transfer failed.");
                remove(outZipPath);
                break;
            }
            remove(outZipPath);

        } while(0);

        OrbisSaveDataMountPoint mp;
        memset(&mp, 0, sizeof(OrbisSaveDataMountPoint));
        strcpy(mp.data, mountResult.mountPathName);

        int32_t umountErrorCode = sceSaveDataUmount(&mp);
        
        if (umountErrorCode < 0) {
            sendStatusCode(connfd, umountErrorCode);
            break;
        } else {
            sendStatusCode(connfd, CMD_STATUS_READY);
        }
        Log("Unmounted save.");


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