
#include "file_utils.h"
#include <regex>
#include <iostream>
#include "utils.h"


std::string to_host(const std::string& from_client) {
    std::string s(from_client);
    for (std::string::size_type c = 0; c < s.length(); ++c) {
        if (s[c] == char(0xa4)) s[c] = 0x5f; // '_'
    }
    return s;
}

std::string to_client(const std::string& from_host) {
    std::string s(from_host);
    for (std::string::size_type c = 0; c < s.length(); ++c) {
        if (s[c] == char(0x5c)) s[c] = 0x2f; // '\' --> '/'
        else if (s[c] == char(0x5f)) s[c] = 0xa4; // '_'
    }
    return s;
}


std::optional<std::vector<u8>> Loader::operator()(const std::string& name_) {
    namespace fs = std::filesystem;

    // TODO: filtered listing, e.g. 'LOAD"$:BA*"'
    if (name_ == "$") return dir_basic_listing(cur_dir);

    std::string name = to_host(name_);

    auto path = fs::path(name);
    if (!path.is_absolute()) path = cur_dir / path;
    if (fs::is_directory(path)) return load_dir(path);
    if (fs::is_regular_file(path)) return read_file(path.string());

    // matching done only against names in current dir
    if ((name.find("*") != std::string::npos) || (name.find("?") != std::string::npos)) {
        // TODO: special chars (e.g. "+" --> "\+")
        std::string rs = replace(name, "*", ".*");
        rs = replace(rs, "?", ".");
        std::regex re(rs);

        auto [dirs, files] = list_dir(cur_dir.string());
        for (const auto& dn : dirs) {
            if (std::regex_match(dn, re)) {
                path = cur_dir / fs::path(dn);
                return load_dir(path);
            }
        }
        for (const auto& fn : files) {
            if (std::regex_match(fn, re)) {
                path = cur_dir / fs::path(fn);
                return read_file(path.string());
            }
        }
    }

    return {};
}


// generates a basic program listing of the dir listing
std::vector<u8> Loader::dir_basic_listing(const std::filesystem::path& dir) {
    struct Basic_line { u16 num; std::string text; };
    using Basic_listing = std::vector<Basic_line>;

    // treats text as... text.. (that is, no tokenization)
    auto to_bin = [](const Basic_listing& bl) -> std::vector<u8> {
        std::vector<u8> bin;

        u16 addr = 0x0801;
        bin.push_back(addr); // load addr. (lo)
        bin.push_back(addr >> 8); // load addr. (hi)

        for(auto& line : bl) {
            u16 line_len = 2 + 2 + line.text.length() + 1;
            addr += line_len;
            bin.push_back(addr); // lo
            bin.push_back(addr >> 8); // hi
            bin.push_back(line.num); // lo
            bin.push_back(line.num >> 8); // hi
            for (auto c : line.text) bin.push_back(c); // text
            bin.push_back(0x00); // end of line
        }

        bin.push_back(0x00); // end of program (null pointer)
        bin.push_back(0x00);

        return bin;
    };

    static constexpr u8 first_line[] = { // 0 PRINT"<clr/home>":LIST1-
        0x99, 0x22, 0x93, 0x22, 0x3a,  // 'print' token, '"', '<clr/home>', '"', ':'
        0x9b, 0x31, 0xab, 0x00 // 'list' token, '1', '-', eol
    };
    auto second_line = (char)0x12 + std::string("\"") + as_upper(to_client(dir.string()))
                            + std::string("\""); // <rvs-on>"<dir_path>"

    Basic_listing bl{
        { 0x00, std::string((char*)first_line) },
        { 0x01, second_line }, // 1 <header>
    };

    if (dir.parent_path() != dir) bl.push_back({ 0x02, std::string("/: \"..\"") });

    auto [dirs, files] = list_dir(dir.string());
    for (const auto& name : dirs) {
        bl.push_back({ 0x02, std::string("/: \"") + to_client(name) + "\"" }); // 2 /: "<name>"
    }
    for (const auto& name : files) {
        bl.push_back({ 0x03, std::string(" : \"") + to_client(name) + '"' }); // 3  : "<name>"
    }

    return to_bin(bl);
}


std::vector<u8> Loader::load_dir(const path& dir_path) {
    cur_dir = std::filesystem::canonical(dir_path.lexically_normal());
    return dir_basic_listing(cur_dir);
}
