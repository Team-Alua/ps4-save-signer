#include "zip_util.hpp"

typedef size_t (*on_extract)(void *arg, uint64_t offset, const void *data, size_t size);

int zip_partial_extract(char * inputZipPath, std::vector<std::string>& inPaths, std::vector<std::string>& outPaths) {

    struct zip_t *archive = zip_open(inputZipPath, ZIP_DEFAULT_COMPRESSION_LEVEL, 'r');
    if (archive == NULL) {
        Log("There was an issue opening %s", inputZipPath);
        return -1;
    }
    Log("Opening zip at %s", inputZipPath);
    int32_t statusCode = 0;
    for(uint32_t i = 0; i < inPaths.size(); i++) {
        std::string & inPath = inPaths[i];
        std::string & outPath = outPaths[i];
        
        const char * zipFilePath = inPath.c_str();
        const char * targetFilePath = outPath.c_str();

        Log("%ld - Extracting  %s => %s .", archive, zipFilePath, targetFilePath);
        const int entryOpenErrorCode = zip_entry_open(archive, zipFilePath);
        if (entryOpenErrorCode < 0) {
            Log("Failed to open %s - %s.", zipFilePath, zip_strerror(entryOpenErrorCode));
            statusCode = entryOpenErrorCode;
            break;
        }

        const int entryReadErrorCode = zip_entry_fread(archive, targetFilePath);
        
        if (entryReadErrorCode < 0) {
            Log("Failed to extract %s - %s", zipFilePath, zip_strerror(entryReadErrorCode));
            statusCode = entryReadErrorCode;
        }
        const int entryCloseErrorCode = zip_entry_close(archive);

        if (entryCloseErrorCode < 0) {
            Log("Failed to close %s in archive - %s", zipFilePath, zip_strerror(entryCloseErrorCode));
            statusCode = entryCloseErrorCode;
        }

        if (statusCode != 0) {
            break;
        }
    }
    zip_close(archive);
    if (statusCode != 0) {
        return statusCode;
    }
    return 0;
}

int zip_partial_directory(char * outZipPath, std::vector<std::string>& inPaths, std::vector<std::string>& outPaths) {

    struct zip_t *archive = zip_open(outZipPath, ZIP_DEFAULT_COMPRESSION_LEVEL, 'w');
    if (archive == NULL) {
        Log("There was an issue opening %s %ld", outZipPath, errno);
        return -1;
    }
    Log("Opening zip at %s", outZipPath);
    int32_t statusCode = 0;
    for(uint32_t i = 0; i < inPaths.size(); i++) {
        std::string & inPath = inPaths[i];
        std::string & outPath = outPaths[i];
        
        const char * fullname = inPath.c_str();
        const char * filename = outPath.c_str();

        Log("%ld - Zipping  %s => %s .", archive, fullname, filename);
        const int entryOpenErrorCode = zip_entry_open(archive, filename);
        if (entryOpenErrorCode < 0) {
            Log("Failed to open %s - %s.", filename, zip_strerror(entryOpenErrorCode));
            statusCode = entryOpenErrorCode;
            break;
        }

        const int entryWriteErrorCode = zip_entry_fwrite(archive, fullname);
        
        if (entryWriteErrorCode < 0) {
            Log("Failed to write %s - %s", fullname, zip_strerror(entryWriteErrorCode));
            statusCode = entryWriteErrorCode;
        }

        const int entryCloseErrorCode = zip_entry_close(archive);

        if (entryCloseErrorCode < 0) {
            Log("Failed to close %s in archive - %s", filename, zip_strerror(entryCloseErrorCode));
            statusCode = entryCloseErrorCode;
        }

        if (statusCode != 0) {
            break;
        }
    }

    zip_close(archive);
    if (statusCode != 0) {
        errno = statusCode;
        return -1;
    }
    return 0;
}

int zip_directory(char * outZipPath, const char * targetDirectory) {
    std::vector<std::string> zipFileNames;
    int result = recursiveList(targetDirectory, "", zipFileNames);
    if (result != 0) {
        return -1;
    }

    Log("There are %i files in %s .", zipFileNames.size(), targetDirectory);
    std::vector<std::string> absoluteFilePaths;
    char saveFilePath[128];
    for (int i = 0; i < zipFileNames.size(); i++) {
        memset(saveFilePath, 0, sizeof(saveFilePath));
        sprintf(saveFilePath, "%s%s", targetDirectory, zipFileNames[i].c_str());
        absoluteFilePaths.push_back(std::string(saveFilePath));
    }

    return zip_partial_directory(outZipPath, absoluteFilePaths, zipFileNames);
}