#ifndef FILE_UTILS_H_INCLUDED
#define FILE_UTILS_H_INCLUDED

#include <string>
#include <vector>
#include <optional>
#include "common.h"


using load_result = std::optional<std::vector<u8>>;
using loader = std::function<load_result(const std::string&)>;

loader Loader(const std::string& init_dir);


#endif // FILE_UTILS_H_INCLUDED
