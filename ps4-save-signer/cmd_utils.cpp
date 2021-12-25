#include "cmd_utils.hpp"

ssize_t readFull(int connfd, void * buffer, size_t size) {
    size_t offset = 0;
    while(size > 0) {
        char * data = (char *)buffer + offset;
        size_t readSize = 1024;
        if (size < readSize) {
            readSize = size;
        }

        ssize_t bytesRead = read(connfd, data, readSize);
        if (bytesRead == 0) {
            return bytesRead;
        } else if (bytesRead == -1) {
            return errno;
        }
        
        offset += bytesRead;
        size -= bytesRead;
    }
    return offset;
}

int getPacketHeader(int connfd, PacketHeader * packetHeader) {
    ssize_t resultCode = readFull(connfd, packetHeader, sizeof(PacketHeader));
    if (resultCode < 0) {
        return UNEXPECTED_ERROR;
    }

    if (resultCode < sizeof(PacketHeader)) {
        return UNEXPECTED_ERROR;
    }
    
    if (packetHeader->magic != PACKET_MAGIC) {
        return CMD_MAGIC_MISMATCH;
    }

    return CMD_STATUS_READY;
 }


ssize_t sendStatusCode(int connfd, uint32_t command) {
    return write(connfd, &command, sizeof(uint32_t));
}


int recursiveDelete(const char * sourceDirectoryPath) {
    DIR * sourceDirectory = opendir(sourceDirectoryPath);

    // do file copy
    std::vector<std::string> folders;
    struct dirent *file;
    int errorCode = 0;
    do {
        file = readdir(sourceDirectory);
        if (file == NULL) {
            break;
        }

        if (file->d_name[0] == '.') {
            continue;
        }

        if (file->d_type == DT_DIR) {
            folders.push_back(std::string(file->d_name));
        } else if (file->d_type == DT_REG) {
            char sourcePath[256];            
            memset(sourcePath, 0, sizeof(sourcePath));
            strcpy(sourcePath, sourceDirectoryPath);
            strcat(sourcePath, file->d_name);
            int fileRemoveResult = remove(sourcePath);
            if (fileRemoveResult < 0) {
                errorCode = fileRemoveResult;
                break;
            }
        }
    } while (true);

    closedir(sourceDirectory);

    if (errorCode != 0) {
        return errorCode;
    }
    
    char newSourceDirectory[256];
    
    for (std::string folderName : folders) {
        memset(newSourceDirectory, 0, sizeof(newSourceDirectory));
        strcpy(newSourceDirectory, sourceDirectoryPath);
        strcat(newSourceDirectory, folderName.c_str());
        strcat(newSourceDirectory, "/");
        int deleteResult = recursiveDelete(newSourceDirectory);
        if (deleteResult != 0) {
            return deleteResult;
        }
    }
    return rmdir(sourceDirectoryPath);
}

int recursiveList(const char * sourceDirectoryPath, const char * baseDirectory, std::vector<std::string> & files) {
    DIR * sourceDirectory = opendir(sourceDirectoryPath);

    std::vector<std::string> folders;

    struct dirent *file;
    do {
        file = readdir(sourceDirectory);
        if (file == NULL) {
            break;
        }

        if (file->d_name[0] == '.') {
            continue;
        }

        if (file->d_type == DT_DIR) {
            folders.push_back(std::string(file->d_name));
        } else if (file->d_type == DT_REG) {
            char sourcePath[256];
                   
            memset(sourcePath, 0, sizeof(sourcePath));
            strcpy(sourcePath, baseDirectory);
            strcat(sourcePath, file->d_name);

            files.push_back(std::string(sourcePath));
        }
    } while (true);

    closedir(sourceDirectory);
    
    char newSourceDirectory[256];
    
    for (std::string folderName : folders) {
        memset(newSourceDirectory, 0, sizeof(newSourceDirectory));
        strcpy(newSourceDirectory, sourceDirectoryPath);
        strcat(newSourceDirectory, folderName.c_str());
        strcat(newSourceDirectory, "/");
    
        int result = recursiveList(newSourceDirectory, (folderName + "/").c_str(), files);
        if (result != 0) {
            return result;
        }
    }
    return 0;
}



int recursiveCopy(const char * sourceDirectoryPath, const char * targetDirectoryPath) {
    DIR * sourceDirectory = opendir(sourceDirectoryPath);

    _mkdir(targetDirectoryPath);
    
    DIR * targetDirectory = opendir(targetDirectoryPath);
    // do file copy
    std::vector<std::string> folders;
    struct dirent *file;
    bool success = true;
    do {
        file = readdir(sourceDirectory);
        if (file == NULL) {
            break;
        }

        if (file->d_name[0] == '.') {
            continue;
        }

        if (file->d_type == DT_DIR) {
            folders.push_back(std::string(file->d_name));
        } else if (file->d_type == DT_REG) {
            char sourcePath[256];            
            memset(sourcePath, 0, sizeof(sourcePath));
            strcpy(sourcePath, sourceDirectoryPath);
            strcat(sourcePath, file->d_name);

            char targetPath[256];
            memset(targetPath, 0, sizeof(targetPath));
            strcpy(targetPath, targetDirectoryPath);
            strcat(targetPath, file->d_name);

            int sourceFd = open(sourcePath, O_RDONLY, 0);

            if (sourceFd < 0) {
                success = false;
                break;
            }

            int targetFd = open(targetPath, O_CREAT | O_TRUNC | O_WRONLY, 0777);

            if (targetFd < 0) {
                close(sourceFd);
                success = false;
                break;
            }

            char fileBuffer[1024];
            
            
            while (true) {

                ssize_t n1 = read(sourceFd, fileBuffer, 1024);
                if (n1 == -1) {
                    success = false;
                    break;
                }
                if (n1 == 0) {
                    break;
                }

                bool writeSuccess = true;
                size_t totalWritten = 0;
                while (totalWritten < n1) {
                    ssize_t written = write(targetFd, &fileBuffer[totalWritten], n1 - totalWritten);
                    if (written == -1) {
                        writeSuccess = false;
                        break;
                    }
                    totalWritten += written;
                }
                if (!writeSuccess) {
                    success = false;
                    break;
                }
            }
            
            fsync(targetFd);
            close(sourceFd);
            close(targetFd);
            if (!success) {
                NOTIFY(300, "Failed: src:%s dest:%s", sourcePath, targetPath);
                break;
            }        
        }
    } while (true);

    closedir(sourceDirectory);
    closedir(targetDirectory);
    
    if (!success) {
        return -1;
    }
    
    char newSourceDirectory[256];
    char newTargetDirectory[256];
    
    for (std::string folderName : folders) {
        memset(newSourceDirectory, 0, sizeof(newSourceDirectory));
        strcpy(newSourceDirectory, sourceDirectoryPath);
        strcat(newSourceDirectory, folderName.c_str());
        strcat(newSourceDirectory, "/");

        memset(newTargetDirectory, 0, sizeof(newTargetDirectory));
        strcpy(newTargetDirectory, targetDirectoryPath);
        strcat(newTargetDirectory, folderName.c_str());
        strcat(newTargetDirectory, "/");
        int copyResult = recursiveCopy(newSourceDirectory, newTargetDirectory);
        if (copyResult != 0) {
            return copyResult;
        }
    }
    return 0;
}


bool directoryExists(const char * directoryName) {
    DIR * dir = opendir(directoryName);
    if (dir) {
        closedir(dir);
        return true;
    } else if (ENOENT == errno) {
        return false;
    }
    return false;
}



// http://nion.modprobe.de/blog/archives/357-Recursive-directory-creation.html
void _mkdir(const char *dir) {
    char tmp[256];
    char *p = NULL;

    snprintf(tmp, sizeof(tmp), "%s", dir);
    size_t len = strlen(tmp);

    if(tmp[len - 1] == '/') {
        tmp[len - 1] = 0;
    }
    for(p = tmp + 1; *p; p++) {
        if(*p == '/') {
            *p = 0;
            mkdir(tmp, 0777);
            *p = '/';
        }
    }
    mkdir(tmp, 0777);
}



long getFileSize(const char *filename)
{
    FILE *fp = fopen(filename, "r");
    long off = -1;

    if (fp == NULL)
    {
        return -1;
    }

    do {

        if (fseek(fp, 0, SEEK_END) == -1) break;

        off = ftell(fp);
        if (off == -1) break;
    
        if (fseek(fp, 0, SEEK_SET) == -1) break;

    } while (false);


    if (fclose(fp) != 0)
    {
        return -1;
    }

    return off;
}

int transferFile(int connfd, const char * filePath, std::string fileName) {
    uint8_t fileCount = 1;
    write(connfd, &fileCount, sizeof(uint8_t));
   
    int fd = open(filePath, O_RDONLY, 0);
    Log("Sending... %s", filePath);
    if (fd < 0) {
        sendStatusCode(connfd, errno);
        return errno;
    } else {
        sendStatusCode(connfd, CMD_STATUS_READY);
    }

    off_t fileSize = lseek(fd, 0, SEEK_END);
    if (fileSize == -1) {
        Log("Failed to seek to end", filePath);
        sendStatusCode(connfd, errno);
        close(fd);
        return errno;
    } else {
        sendStatusCode(connfd, CMD_STATUS_READY);
    }

    if (lseek(fd, 0, SEEK_SET) == -1) {
        Log("Failed to seek to beginning", filePath);
        sendStatusCode(connfd, errno);
        close(fd);
        return errno;
    } else {
        sendStatusCode(connfd, CMD_STATUS_READY);
    }
    Log("Got necessary info for %s", filePath);
        
    // size of relative file path (8 byte)
    size_t filePathLength = fileName.size();

    write(connfd, &filePathLength, sizeof(filePathLength));

    write(connfd, &fileSize, sizeof(fileSize));

    write(connfd, fileName.c_str(), fileName.size());

    
    _transferFile(connfd, fd , fileSize);
    close(fd);
    return 0;
}



int transferFiles(int connfd, const char * baseDirectory, std::vector<std::string> relFilePaths, std::vector<std::string> outPaths) {
    uint8_t fileCount = (uint8_t)relFilePaths.size();
    // send file count
    write(connfd, &fileCount, sizeof(uint8_t));
    for(uint32_t i = 0; i < fileCount; i++) {
        std::string & inPath = relFilePaths[i];

        char fullFilePath[256];
        sprintf(fullFilePath, "%s%s", baseDirectory, inPath.c_str());

        int fd = open(fullFilePath, O_RDONLY, 0);
        if (fd < 0) {
            sendStatusCode(connfd, errno);
            continue;
        } else {
            sendStatusCode(connfd, CMD_STATUS_READY);
        }

        off_t fileSize = lseek(fd, 0, SEEK_END);
        if (fileSize == -1) {
            sendStatusCode(connfd, errno);
            close(fd);
            continue;
        } else {
            sendStatusCode(connfd, CMD_STATUS_READY);
        }

        if (lseek(fd, 0, SEEK_SET) == -1) {
            sendStatusCode(connfd, errno);
            close(fd);
            continue;
        } else {
            sendStatusCode(connfd, CMD_STATUS_READY);
        }

        std::string & outPath = outPaths[i];
        
        // size of relative file path (8 byte)
        size_t filePathLength = outPath.size();


        write(connfd, &filePathLength, sizeof(filePathLength));

        write(connfd, &fileSize, sizeof(fileSize));

        write(connfd, outPath.c_str(), outPath.size());
        
        _transferFile(connfd, fd , fileSize);

        close(fd);
    }
    return 0;
}

#include <math.h>

static int _transferFile(int connfd, int fd, size_t size) {
    unsigned char buffer[4096];
    int error = 0;

    while(size > 0) {
        size_t bufferRead = std::min(4096ul, size);

        ssize_t bytesRead = read(fd, buffer, bufferRead);
        if (bytesRead > 0) {
            size -= bytesRead;
            error = 0;
            size_t totalBytesWritten = 0;
            while (totalBytesWritten < bytesRead) {
                size_t bytesWritten = write(connfd, &buffer[totalBytesWritten], bytesRead - totalBytesWritten);
                if (bytesWritten < 0) {
                    return -1;
                }
                if (bytesWritten == 0) {
                    break;
                }
                totalBytesWritten += bytesWritten;
            }
        } else if (bytesRead == 0) {
            break;
        } else {
            if (error == 0) {
                error = 1;
            }
        }

    }
    return 0;
}

void downloadFileTo(int connfd, const char * basePath, const char * filename, uint32_t filesize) {
    char filepath[256];
    memset(filepath, 0, 256);

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

    int fd = open(filepath, O_CREAT | O_WRONLY, 0777);

    if (fd == -1) {
        sendStatusCode(connfd, errno);
        return; 
    } else {
        sendStatusCode(connfd, CMD_STATUS_READY);
    }

    uint8_t buffer[8192];

    uint32_t bytesRemaining = filesize;
    uint32_t statusCode = CMD_STATUS_READY;

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
            statusCode = received;
            NOTIFY(50, "read socket error: %d", errno);
            break;
        }
        size_t fileOffset = filesize - bytesRemaining;
        ssize_t writeError = pwrite(fd, buffer, received, fileOffset);
        if (writeError == -1) {
            statusCode = errno;
            // NOTIFY(50, "write file error: %d %d", errno, fd);
            break;
        } else if (writeError < received) {
            // There is a mismatch in bytes read and written
            statusCode = -1;
            // NOTIFY(50, "write file error: %d %d", errno, fd);
            break;
        }
        bytesRemaining -= received;
    } while(true);
    


    close(fd);
    
    if (statusCode != CMD_STATUS_READY) {
        unlink(filepath);
    }
    sendStatusCode(connfd, statusCode);
}


std::string getRandomFileName(uint8_t size) {
    char filename[26];
    if (size > 25) {
        // adjust size to max
        size = 25;
    }

    memset(filename, 0, sizeof(filename));
    for(int i = 0; i < size; i++) {
        int number = rand() % 16;
        if (number > 9) {
            filename[i] = number + 'A' - 10;  
        } else {
            filename[i] = number + '0';
        }
    }

    return std::string(filename);
}
