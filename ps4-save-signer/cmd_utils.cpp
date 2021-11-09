#include "cmd_utils.hpp"
#include "cmd_constants.hpp"
#include "util.h"
#include <vector>
#include <string>

using namespace std::__1::__fs::filesystem;


size_t readFull(int connfd, void * buffer, size_t size) {
    size_t offset = 0;
    while(offset < size) {
        char * data = (char *)buffer + offset;
        size_t readSize = 1024;
        if (size < readSize) {
            readSize = size;
        }

        ssize_t newOffset = read(connfd, data, readSize);
        if (newOffset <= 0) {
            return newOffset;
        }
        offset += newOffset;
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
            remove(sourcePath);
        }
    } while (true);

    closedir(sourceDirectory);

    if (!success) {
        return -1;
    }
    
    char newSourceDirectory[256];
    
    for (std::string folderName : folders) {
        memset(newSourceDirectory, 0, sizeof(newSourceDirectory));
        strcpy(newSourceDirectory, sourceDirectoryPath);
        strcat(newSourceDirectory, folderName.c_str());
        strcat(newSourceDirectory, "/");
        int copyResult = recursiveDelete(newSourceDirectory);
        if (copyResult != 0) {
            return copyResult;
        }
    }
    remove(sourceDirectoryPath);
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


int transferFiles(int connfd, const char * baseDirectory, std::vector<std::string> relFilePaths, std::vector<std::string> outPaths) {
    uint8_t fileCount = (uint8_t)relFilePaths.size();
    // send file count
    write(connfd, &fileCount, sizeof(uint8_t));
    for(uint32_t i = 0; i < fileCount; i++) {
        std::string & inPath = relFilePaths[i];

        char fullFilePath[256];
        memset(fullFilePath, 0, sizeof(fullFilePath));
        strcpy(fullFilePath, baseDirectory);
        strcat(fullFilePath, inPath.c_str());



        int fd = open(fullFilePath, O_RDONLY, 0);
        if (fd < 0) {
            NOTIFY(300, "open error=%d", errno);
            sendStatusCode(connfd, CMD_SAVE_GEN_COPY_SKIP_FILE);
            continue;
        }

        off_t fileSize = lseek(fd, 0, SEEK_END);
        if (fileSize == -1) {
            sendStatusCode(connfd, CMD_SAVE_GEN_COPY_SKIP_FILE);
            close(fd);
            continue;
        }

        if (lseek(fd, 0, SEEK_SET) == -1) {
            sendStatusCode(connfd, CMD_SAVE_GEN_COPY_SKIP_FILE);
            close(fd);
            continue;
        }
        
        std::string & outPath = outPaths[i];
        // size of relative file path (8 byte)
        size_t filePathLength = outPath.size();

        sendStatusCode(connfd, CMD_STATUS_READY);

        write(connfd, &filePathLength, sizeof(filePathLength));

        write(connfd, &fileSize, sizeof(fileSize));

        write(connfd, outPath.c_str(), outPath.size());
        
        transferFile(connfd, fd , fileSize);

        close(fd);
    }
    return 0;
}

#include <math.h>

int transferFile(int connfd, int fd, size_t size) {
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
                NOTIFY(300, "read error %d", errno);
            }
        }

    }
    return 0;
}