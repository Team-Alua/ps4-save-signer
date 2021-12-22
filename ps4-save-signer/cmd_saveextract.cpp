#include "cmd_saveextract.hpp"


struct __attribute((packed)) SaveExtractPacket {
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

    doSaveExtract(connfd, &uploadPacket);
}

static void doSaveExtract(int connfd, SaveExtractPacket * saveExtractPacket) {
    do {

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

        char zipPath[256];
        memset(zipPath, 0, sizeof(zipPath));
        strcpy(zipPath, "/data/teamalua/uploads/");
        strcat(zipPath, saveExtractPacket->zipname);


        // Update files
        char targetDirectory[256];

        memset(targetDirectory, 0, sizeof(targetDirectory));
        
        sprintf(targetDirectory,"/user/home/%x/savedata/%s/", getUserId(), saveExtractPacket->titleId);

        int zipExtractStatus = zip_extract(zipPath, targetDirectory, NULL, NULL);
        
        if (zipExtractStatus < 0) {
            sendStatusCode(connfd, zipExtractStatus);
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

        do {
           // zip up all the files 
            std::vector<std::string> folders;
            char mountFolder[256];
            memset(mountFolder, 0, sizeof(mountFolder));
            strcpy(mountFolder, "/mnt/sandbox/BREW00085_000");
            strcat(mountFolder, mountResult.mountPathName);
            strcat(mountFolder, "/");

            int result = recursiveList(mountFolder, "", folders);
            if (result != 0) {
                // report this as an error
                break;
            }

            // TODO: Do not hardcode this. Have it auto generate
            const char * outZipPath = "/tmp/owo.zip";
            struct zip_t *archive = zip_open(outZipPath, ZIP_DEFAULT_COMPRESSION_LEVEL, 'w');

            char fullname[256];
            bool success = true;
            for (std::string file : folders) {
                memset(fullname, 0, sizeof(fullname));
                strncpy(fullname, mountFolder, strlen(mountFolder));
                strncat(fullname, file.c_str(), strlen(file.c_str()));
                zip_entry_open(archive, file.c_str());
				if (zip_entry_fwrite(archive, fullname) != 0) {
                    success = false; 
				}
				zip_entry_close(archive);
                if (!success) {
                    break;
                }
            }

            zip_close(archive);


            if (!success) {
                // delete then send back an error
            }

            std::vector<std::string> files;

            files.push_back("owo.zip");

            if (transferFiles(connfd, "/tmp/", files, files) != 0) {
                remove("/tmp/owo.zip");
                break;
            }
            remove("/tmp/owo.zip");
            // report as success here
        } while(0);

        OrbisSaveDataMountPoint mp;
        memset(&mp, 0, sizeof(OrbisSaveDataMountPoint));
        strcpy(mp.data, tempMountResult.mountPathName);
        

        int32_t umountErrorCode = sceSaveDataUmount(&mp);
        
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
        del.dirName = (OrbisSaveDataDirName *) saveExtractPacket->dirName;
        del.titleId = (OrbisSaveDataTitleId *) saveExtractPacket->titleId;
        int32_t deleteUserSaves = sceSaveDataDelete(&del);
        if (deleteUserSaves < 0) {
            sendStatusCode(connfd, deleteUserSaves);
            break;
        } else {
            sendStatusCode(connfd, CMD_STATUS_READY);
        }
    } while(false);
}