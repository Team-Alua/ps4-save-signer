#include "zip.h"

#include <vector>
#include <string>

#include "log.hpp"
#include "cmd_utils.hpp"

#pragma once


/**
 * @brief 
 * 
 * @param inputZipPath 
 * @param inPaths 
 * @param outPaths 
 * @return int 
 */
int zip_partial_extract(char * inputZipPath, std::vector<std::string>& inPaths, std::vector<std::string>& outPaths);

/**
 * @brief 
 * 
 * @param outZipPath a
 * @param absoluteFilePaths b 
 * @param outPaths c
 * @return int 
 */
int zip_partial_directory(char * outZipPath, std::vector<std::string>& inPaths, std::vector<std::string>& outPaths);


int zip_directory(char * outZipPath, const char * directory);