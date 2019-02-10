
#include "utils.h"

#include <fstream>
#include <algorithm>


std::vector<char> read_bin_file(const std::string& filename) {
    std::ifstream f(filename, std::ios::binary);
    return std::vector<char>(
        std::istreambuf_iterator<char>(f),
        std::istreambuf_iterator<char>()
    );
}


size_t read_bin_file(const std::string& filename, char* buf) {
    auto bin = read_bin_file(filename);
    std::copy(bin.begin(), bin.end(), buf);
    return bin.size();
}


std::string as_lower(const std::string& src) {
    std::string dst(src);
    std::transform(src.begin(), src.end(), dst.begin(), ::tolower);
    return dst;
}
