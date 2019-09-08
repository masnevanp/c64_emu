#ifndef FILE_UTILS_H_INCLUDED
#define FILE_UTILS_H_INCLUDED

#include <string>
#include <vector>
#include "common.h"


std::vector<u8> dir_basic_listing(const std::string& dir);

std::vector<u8> read_file(const std::string& dir, const std::string& name_pattern);


#endif // FILE_UTILS_H_INCLUDED
