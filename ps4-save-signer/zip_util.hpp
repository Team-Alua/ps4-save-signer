#include "zip.h"

#include <vector>
#include <string>

#include "log.hpp"


#pragma once

/**
 * @brief 
 * 
 * @param outZipPath a
 * @param absoluteFilePaths b 
 * @param outPaths c
 * @return int 
 */
int zip_partial_directory(char * outZipPath, std::vector<std::string>& inPaths, std::vector<std::string>& outPaths);