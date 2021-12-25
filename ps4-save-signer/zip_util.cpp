#include "zip_util.hpp"


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
        if (zip_entry_open(archive, filename) != 0) {
            Log("Failed to open %s - %ld.", filename, errno);
            statusCode = errno;
            break;
        }

        if (zip_entry_fwrite(archive, fullname) != 0) {
            Log("Failed to write %s - %ld", fullname, errno);
            statusCode = errno;
        }

        if (zip_entry_close(archive) != 0) {
            Log("Failed to close archive - %ld", errno);
            statusCode = errno;
        }

        if (statusCode != 0) {
            Log("Failed for some reason - %ld", statusCode);
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