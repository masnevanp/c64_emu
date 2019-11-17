#ifndef FILE_UTILS_H_INCLUDED
#define FILE_UTILS_H_INCLUDED

#include <string>
#include <vector>
#include <filesystem>
#include <optional>
#include "common.h"



class Loader {
public:
    using path = std::filesystem::path;

    Loader(const path& init_dir) { load_dir(init_dir); }

    std::optional<std::vector<u8>> operator()(const std::string& name);

    static std::vector<u8> dir_basic_listing(const path& dir);

private:
    path cur_dir;

    std::vector<u8> load_dir(const path& dir_path);

};


#endif // FILE_UTILS_H_INCLUDED
