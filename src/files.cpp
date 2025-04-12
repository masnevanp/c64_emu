
#include "files.h"
#include <regex>
#include <iostream>
#include <algorithm>
#include <iterator>
#include <filesystem>
#include "utils.h"



namespace fs = std::filesystem;


using D64 = Files::D64;
using Load_result = Files::Load_result;


namespace petscii { // TOREDO...
    static constexpr char slash      = 0x2f;
    static constexpr char arrow_up   = 0x5e;
    static constexpr char arrow_left = 0x5f;
    static constexpr char nbsp       = (char)0xa0;
    static constexpr char underscore = (char)0xa4;
}

namespace ctrl_ch { // TOREDO...
    static constexpr char rvs_on     = 0x12;
    //static constexpr char del        = 0x14;
}


static const char* win_drive_list_filename = "?:/";
static const char* unmount_filename = ":";

static const int HD_FILE_SIZE_MAX = 0x200000; // TODO: check
static const int C64_BIN_SIZE_MIN = 0x0003; // TODO: check
static const int C64_BIN_SIZE_MAX = 0xffff; // TODO: check
static const int T64_SIZE_MIN = 0x60;
static const int G64_SIZE_MIN = 0xffff; // TODO: a more realistic minimum (this is far below...)
static const int CRT_SIZE_MIN = 0x0050;


static constexpr auto& NOT_FOUND = std::nullopt;
static constexpr auto& NONE = std::nullopt;


static constexpr u8 dir_list_first_line[] = { // 0 PRINT"<clr>":LIST1-:REM<del><del>...<del>
    0x99, 0x22, 0x93, 0x22, 0x3a,  // 'print' token, '"', '<clr>', '"', ':'
    0x9b, 0x31, 0xab, 0x3a, 0x8f, // 'list' token, '1', '-', ':', 'rem' token
    0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, // <del>, <del>,...
    0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, // ...<del>, <del>
    0x14, 0x00 // <del>, eol
};


static constexpr u8 t64_signatures[][16] = {
    { 0x43, 0x36, 0x34, 0x53, 0x20, 0x74, 0x61, 0x70, 0x65, 0x20, 0x69, 0x6D, 0x61, 0x67, 0x65, 0x20 },
    { 0x43, 0x36, 0x34, 0x20, 0x74, 0x61, 0x70, 0x65, 0x20, 0x69, 0x6D, 0x61, 0x67, 0x65, 0x20, 0x66 },
    { 0x43, 0x36, 0x34, 0x53, 0x20, 0x74, 0x61, 0x70, 0x65, 0x20, 0x66, 0x69, 0x6c, 0x65, 0x0d, 0x0a },
};

static constexpr u8 g64_signature[] = { 0x47, 0x43, 0x52, 0x2D, 0x31, 0x35, 0x34, 0x31 };

static constexpr u8 crt_signature[] = {
    0x43, 0x36, 0x34, 0x20, 0x43, 0x41, 0x52, 0x54,
    0x52, 0x49, 0x44, 0x47, 0x45, 0x20, 0x20, 0x20
};



Files::Img::Type file_type(const Bin& file) {
    auto is_crt = [&]() {
        return std::size(file) >= CRT_SIZE_MIN
                    && std::equal(std::begin(crt_signature), std::end(crt_signature),
                                        std::begin(file));
    };

    auto is_t64 = [&]() {
        if (std::size(file) >= T64_SIZE_MIN) {
            for (const auto& sig : t64_signatures) {
                if (std::equal(std::begin(sig), std::end(sig), std::begin(file)))
                    return true;
            }
        }
        return false;
    };

    auto is_g64 = [&]() {
        return std::size(file) >= G64_SIZE_MIN
                    && std::equal(std::begin(g64_signature), std::end(g64_signature),
                                        std::begin(file));
    };

    auto is_d64 = [&]() {
        return std::size(file) == Files::D64::size
                    || std::size(file) == Files::D64::size_with_error_info;
    };

    auto is_raw = [&]() {
        return std::size(file) > C64_BIN_SIZE_MIN && std::size(file) <= C64_BIN_SIZE_MAX;
    };

    using Type = Files::Img::Type;

    if (is_crt()) return Type::crt;
    if (is_t64()) return Type::t64;
    if (is_g64()) return Type::g64;
    if (is_d64()) return Type::d64;
    if (is_raw()) return Type::raw;

    return Type::unknown;
}


Files::Img Files::read_img_file(const std::string& path) {
    const auto fs_path = fs::path(path);
    const auto name = fs_path.filename().string();

    if (fs::file_size(fs_path) <= HD_FILE_SIZE_MAX) {
        if (auto data = read_file(path)) {
            return Img{
                file_type(*data),
                name,
                *data
            };
        }
    }

    return Img{
        Img::Type::oversized,
        name,
        Bin(0)
    };
}


using dir_listing = std::pair<std::vector<std::string>, std::vector<std::string>>;
using dir_filter = std::optional<std::function<bool(const std::string&)>>;


std::string extract_string(const u8* from, char terminator = petscii::nbsp, int max_len = 16) {
    int n = 0;
    while ((char)from[n] != terminator && ++n < max_len);
    return std::string(&from[0], &from[n]);
}


std::string quoted(const std::string& s) {
    return '"' + s + '"';
}


std::regex regex(const std::string& pattern) {
    std::string rs = replace(pattern, "*", ".*");
    rs = replace(rs, "?", ".");
    return std::regex(as_lower(rs));
}


std::string to_client(const std::string& from_host) {
    std::string s = as_upper(from_host);
    for (std::string::size_type c = 0; c < s.length(); ++c) {
        if (s[c] == '\\') s[c] = petscii::slash;
        else if (s[c] == '_') s[c] = petscii::underscore;
    }
    return s;
}


std::string to_host(const std::string& from_client) {
    std::string s(from_client);
    for (std::string::size_type c = 0; c < s.length(); ++c) {
        if (s[c] == petscii::underscore) s[c] = '_';
    }
    return s;
}


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

    auto cmp = [](const auto& a, const auto& b) { return as_lower(a) < as_lower(b); };

    std::sort(dirs.begin(), dirs.end(), cmp);
    std::sort(files.begin(), files.end(), cmp);

    return {dirs, files};
}



class Basic_listing {
public:
    struct Line { u16 num; std::string text; };

    Basic_listing(std::initializer_list<Line> lines) : listing(lines) {}

    void append(Line line) { listing.push_back(line); }

    Bin to_bin() const {
        Bin bin;

        u16 addr = 0x0801;
        bin.push_back(addr); // load addr. (lo)
        bin.push_back(addr >> 8); // load addr. (hi)

        for(auto& line : listing) {
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
    }

private:
    std::vector<Line> listing;

};


// generates a basic program listing of the dir listing
Bin hd_dir_basic_listing(
    const std::string& dir,
    const std::vector<std::string>& sub_dirs,
    const std::vector<std::string>& files,
    bool drives_entry = false, bool root_entry = false, bool parent_entry = false)
{
    auto header = ctrl_ch::rvs_on + std::string(" $ ") + quoted(to_client(dir));

    Basic_listing bl{
        { 0x00, std::string((char*)dir_list_first_line) },
        { 0x01, header },
    };

    if (drives_entry) {
        static auto de = std::string(" ") + petscii::arrow_up + " "
                            + quoted(win_drive_list_filename);
        bl.append({ 0x02,  de });
    }
    if (root_entry) {
        static auto re = std::string(" ") + petscii::arrow_up + " \"/\"";
        bl.append({ 0x02, re });
    }
    if (parent_entry) {
        static auto pe = std::string(" ") + petscii::arrow_left + " " + quoted("..");
        bl.append({ 0x02, pe });
    }

    for (const auto& d : sub_dirs) bl.append({ 0x03, " / " + quoted(to_client(d)) });
    for (const auto& f : files)    bl.append({ 0x04, " : " + quoted(to_client(f)) });

    return bl.to_bin();
}


#ifdef _WIN32
Bin load_win_drives_list() {
    std::vector<std::string> drives;
    std::string d = "?:/";
    for (char c : "ABCDEFGHIJKLMNOPQRSTUWVXYZ") { // good ol' trial & error...
        d[0] = c;
        if (fs::is_directory(d)) drives.push_back(d);
    }

    return hd_dir_basic_listing(win_drive_list_filename, drives, std::vector<std::string>());
}
#endif


class _Loader {
public:
    _Loader(const std::string& init_dir) : context{init_dir} {}

    Load_result operator()(const std::string& what) { return load(what); }

private:
    fs::path context;

    Load_result load(const std::string& what) {
        if (fs::is_directory(context)) return load_hd(what);
        if (fs::is_regular_file(context)) return load_hd_file(context, what);
        context = "/";
        return load_hd("");
    }

    Bin load_hd_dir(const fs::path& new_dir) {
        auto cd = fs::canonical(new_dir.lexically_normal());
        auto [dirs, files] = list_dir(cd.string());

        bool drives_entry = false;
        #ifdef _WIN32
        drives_entry = cd == cd.root_path();
        #endif
        bool root_entry = cd.parent_path() != cd.root_path();
        bool parent_entry = cd.parent_path() != cd;

        context = cd;

        return hd_dir_basic_listing(context.string(), dirs, files,
                                        drives_entry, root_entry, parent_entry);
    }

    Load_result load_hd_file(const fs::path& path, const std::string& what) {
        using Type = Files::Img::Type;

        auto is_already_mounted = [&]() { return context == path; };
        auto mount = [&]() { context = path; };

        auto img = Files::read_img_file(path.string());

        switch (img.type) {
            case Type::crt: return {img, NONE};
            case Type::t64: return {NONE, load_from_t64(img.data, what)};
            case Type::d64:
                if (is_already_mounted()) {
                    if (what == unmount_filename) return unmount();
                    else return {NONE, load_from_d64(img.data, what)};
                } else {
                    mount();
                    return {img, load_from_d64(img.data, "$")};
                }
            case Type::g64: return {img, NONE}; // TODO: handle same as a d64?
            case Type::raw: return {NONE, std::move(img.data)};
            default:
                return {NOT_FOUND, NOT_FOUND};
        }
    }

    Load_result load_hd(const std::string& what);

    Maybe<Bin> load_from_d64(const Bin& d64_data, const std::string& what);
    Maybe<Bin> load_from_g64(const Bin& g64_data, const std::string& what);
    Maybe<Bin> load_from_t64(const Bin& t64_data, const std::string& what);

    Load_result unmount() {
        context = context.parent_path();
        return load("");
    }
};


Load_result _Loader::load_hd(const std::string& what) {
    const auto& cur_dir = context;

    try {
        if (what.length() == 0 || what == "$") return {NONE, load_hd_dir(cur_dir)};

        #ifdef _WIN32
        if (what == win_drive_list_filename) return {NONE, load_win_drives_list()};
        #endif

        // if file/dir --> load it
        std::string name = to_host(what);
        auto path = fs::path(name);
        if (!path.is_absolute()) path = cur_dir / path;
        if (fs::is_directory(path)) return {NONE, load_hd_dir(path)};
        if (fs::is_regular_file(path)) return load_hd_file(path, "");

        // no luck, try to filter something (TODO: special chars (e.g. "+" --> "\+")?)
        std::function<bool(const std::string&)> dir_filter;
        try {
            auto re = regex(name);
            dir_filter = [re = std::move(re)](const std::string& entry) -> bool {
                return std::regex_match(as_lower(entry), re);
            };
        } catch (const std::regex_error& e) {
            dir_filter = [](const std::string& _) -> bool { UNUSED(_); return true; };
        }

        auto [dirs, files] = list_dir(cur_dir.string(), dir_filter);

        auto n = std::size(dirs) + std::size(files);
        if (n == 1) { // if only 1 --> just load it (dir/file)
            if (std::size(dirs) == 1) {
                path = cur_dir / dirs[0];
                return {NONE, load_hd_dir(path)};
            } else {
                path = cur_dir / files[0];
                return load_hd_file(path, "");
            }
        } else if (n > 1) { // if many --> load listing
            auto d = cur_dir.string() + '/' + what;
            return {NONE, hd_dir_basic_listing(d, dirs, files)};
        }
    } catch (const fs::filesystem_error& e) {
        std::cout << e.what() << std::endl;
    }

    return {NOT_FOUND, NOT_FOUND};
}


Bin d64_dir_basic_listing(const D64& d64) {
    using File_type = D64::Dir_entry::File_type;

    auto type_str = [](const File_type& t){
        static const constexpr char* const ts[] =  {
            " DEL ", " SEQ ", " PRG ", " USR ", " REL ", " ??? "
        };
        auto s = (t.type() != File_type::Type::bad) ? std::string(ts[t.type()])
                                        : std::string(ts[File_type::Type::bad]);
        if (t.locked()) s[4] = '<';
        if (!t.closed()) s[0] = '*';
        return s;
    };

    auto header = ctrl_ch::rvs_on + std::string(" $ ")
        + quoted(extract_string(d64.bam().disk_name, 0x00)) + "  "
        + extract_string(d64.bam().disk_id, petscii::nbsp, 2) + " "
        + extract_string(d64.bam().dos_type, petscii::nbsp, 2) + "    ";

    Basic_listing bl{
        { 0x00, std::string((char*)dir_list_first_line) },
        { 0x01, header },
    };

    for (const auto entry : d64.dir()) {
        std::string text(" : \"N                :T    : S  ");
        int c = 0;
        for (; c < 16; ++c) { // TODO: simplify...?
            char ch = entry->filename[c];
            if (ch == petscii::nbsp) break;
            text[c + 4] = ch;
        }
        text[c + 4] = '"';

        text.replace(22, 5, type_str(entry->file_type));
        text.replace(29, 3, std::to_string(entry->size));

        bl.append({ 0x05, text });
    }

    bl.append({ 0x00, " : \"=================: NON : " + std::to_string(d64.bam().blocks_free())});
    bl.append({ 0x00, " :" });
    bl.append({ 0x09, std::string(" : \"") + unmount_filename + "\"               : *UNMOUNT*" });

    return bl.to_bin();
}


Maybe<Bin> _Loader::load_from_d64(const Bin& d64_data, const std::string& what) {
    D64 d64{d64_data};

    if (what.length() == 0 || what == "$") return d64_dir_basic_listing(d64);

    std::function<bool(const std::string&)> match;
    try {
        auto re = regex(what);
        match = [&what, re = std::move(re)](const std::string& name) -> bool {
            return name == what || std::regex_match(as_lower(name), re);
        };
    } catch (const std::regex_error& e) {
        match = [&](const std::string& name) -> bool { return name == what; };
    }

    for (const auto entry : d64.dir()) {
        if (match(extract_string(entry->filename))) {
            auto file = d64.read_file(entry->file_start);
            if (file.size() > 0) return file;
            break;
        }
    }

    return NOT_FOUND;
}


// TODO: maybe.. does this make any sense?
Maybe<Bin> _Loader::load_from_g64(const Bin& g64_data, const std::string& what) {
    UNUSED(g64_data); UNUSED(what);
    // TODO(?): g64->d64
    return NOT_FOUND;
}


Maybe<Bin> _Loader::load_from_t64(const Bin& t64_data, const std::string& what) { UNUSED(what);
    Files::T64 t64{t64_data};
    auto file = t64.first_file();
    if (std::size(file) > 2 && std::size(file) <= C64_BIN_SIZE_MAX)
        return file;
    else
        return NOT_FOUND;
}


Files::Loader Files::init_loader(const std::string& init_dir) {
    return _Loader(init_dir);
}
