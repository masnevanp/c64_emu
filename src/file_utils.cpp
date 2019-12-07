
#include "file_utils.h"
#include <filesystem>
#include <regex>
#include <iostream>
#include "utils.h"



namespace fs = std::filesystem;


namespace petscii {
    static constexpr char rvs_on     = 0x12;
    static constexpr char slash      = 0x2f;
    static constexpr char arrow_up   = 0x5e;
    static constexpr char arrow_left = 0x5f;
    static constexpr char underscore = (char)0xa4;
};


const char* win_drive_list_entry = "?:/";


using dir_listing = std::pair<std::vector<std::string>, std::vector<std::string>>;
using dir_filter = std::optional<std::function<bool(const std::string&)>>;

dir_listing list_dir(const std::string& dir, const dir_filter& df = {}) {
    std::vector<std::string> dirs;
    std::vector<std::string> files;

    for (const auto& entry : fs::directory_iterator(dir)) {
        std::string name = entry.path().filename().string();
        if (!df || (*df)(name)) {
            if (entry.is_directory()) dirs.push_back(name);
            else if (entry.is_regular_file()) files.push_back(name);
        }
    }

    std::sort(dirs.begin(), dirs.end());
    std::sort(files.begin(), files.end());

    return {dirs, files};
}


std::string _to_client(const std::string& from_host) {
    std::string s = as_upper(from_host);
    for (std::string::size_type c = 0; c < s.length(); ++c) {
        if (s[c] == '\\') s[c] = petscii::slash;
        else if (s[c] == '_') s[c] = petscii::underscore;
    }
    return s;
}

// generates a basic program listing of the dir listing
std::vector<u8> dir_basic_listing(
    const std::string& dir,
    const std::vector<std::string>& sub_dirs,
    const std::vector<std::string>& files,
    bool drives_entry = false, bool root_entry = false, bool parent_entry = false)
{
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

    static constexpr u8 first_line[] = { // 0 PRINT"<clr>":LIST1-
        0x99, 0x22, 0x93, 0x22, 0x3a,  // 'print' token, '"', '<clr>', '"', ':'
        0x9b, 0x31, 0xab, 0x00 // 'list' token, '1', '-', eol
    };
    auto second_line = petscii::rvs_on + std::string("\"")
                            + _to_client(dir) + std::string("\"");

    Basic_listing bl{
        { 0x00, std::string((char*)first_line) },
        { 0x01, second_line }, // 1 <header>
    };

    if (drives_entry) {
        static auto de = petscii::arrow_up + std::string(": \"")
                            + win_drive_list_entry + std::string("\"");
        bl.push_back({ 0x02,  de });
    }
    if (root_entry) {
        static auto re = petscii::arrow_up + std::string(": \"/\"");
        bl.push_back({ 0x02, re });
    }
    if (parent_entry) {
        static auto pe = petscii::arrow_left + std::string(": \"..\"");
        bl.push_back({ 0x02, pe });
    }

    for (const auto& d : sub_dirs) {
        bl.push_back({ 0x03, std::string("/: \"") + _to_client(d) + "\"" });
    }
    for (const auto& f : files) {
        bl.push_back({ 0x04, std::string(" : \"") + _to_client(f) + '"' });
    }

    return to_bin(bl);
}


#ifdef _WIN32
std::vector<u8> load_win_drives_list() {
    std::vector<std::string> drives;
    std::string d = "?:/";
    for (char c : "ABCDEFGHIJKLMNOPQRSTUWVXYZ") { // good ol' trial & error...
        d[0] = c;
        if (fs::is_directory(d)) drives.push_back(d);
    }

    return dir_basic_listing(win_drive_list_entry, drives, std::vector<std::string>());
}
#endif


std::optional<std::vector<u8>> load_file(const fs::path& path) {
    if (fs::file_size(path) > 0xffff) {
        std::cout << path << " too big" << std::endl;
        return {};
    }

    return read_file(path.string());
}


class _Loader {
public:
    _Loader(const std::string& init_dir) { this->operator()(init_dir); }

    std::optional<std::vector<u8>> operator()(const std::string& name);

private:
    std::filesystem::path cur_dir;

    std::vector<u8> load_dir(const fs::path& new_dir) {
        auto cd = fs::canonical(new_dir.lexically_normal());
        auto [dirs, files] = list_dir(cd.string());

        bool drives_entry = false;
        #ifdef _WIN32
        drives_entry = true;
        #endif
        bool root_entry = cd.parent_path() != cd.root_path();
        bool parent_entry = cd.parent_path() != cd;

        cur_dir = cd;

        return dir_basic_listing(cur_dir.string(), dirs, files,
                                        drives_entry, root_entry, parent_entry);
    }
};


std::string _to_host(const std::string& from_client) {
    std::string s(from_client);
    for (std::string::size_type c = 0; c < s.length(); ++c) {
        if (s[c] == petscii::underscore) s[c] = '_';
    }
    return s;
}


std::optional<std::vector<u8>> _Loader::operator()(const std::string& what) {
    try {
        if (what.length() == 0 || what == "$") return load_dir(cur_dir);

        #ifdef _WIN32
        if (what == win_drive_list_entry) return load_win_drives_list();
        #endif

        // if file/dir --> load it
        std::string name = _to_host(what);
        auto path = fs::path(name);
        if (!path.is_absolute()) path = cur_dir / path;
        if (fs::is_directory(path)) return load_dir(path);
        if (fs::is_regular_file(path)) return load_file(path);

        // no luck, try to filter something (TODO: special chars (e.g. "+" --> "\+")?)
        std::string rs = replace(name, "*", ".*");
        rs = replace(rs, "?", ".");
        std::regex re(as_lower(rs));

        auto dir_filter = [&re](const std::string& entry) -> bool {
            return std::regex_match(as_lower(entry), re);
        };
        auto [dirs, files] = list_dir(cur_dir.string(), dir_filter);

        auto n = std::size(dirs) + std::size(files);
        if (n == 1) { // if only 1 --> just load it (dir/file)
            if (std::size(dirs) == 1) {
                path = cur_dir / dirs[0];
                return load_dir(path);
            } else {
                path = cur_dir / files[0];
                return load_file(path);
            }
        } else if (n > 1) { // if many --> load listing
            auto d = cur_dir.string() + '/' + what;
            return dir_basic_listing(d, dirs, files);
        }
    } catch (const fs::filesystem_error& e) {
        std::cout << e.what() << std::endl;
    }

    return {}; // 'file not found'
}


loader Loader(const std::string& init_dir) {
    return _Loader(init_dir);
}
