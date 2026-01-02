
#include "files.h"
#include <regex>
#include <filesystem>
#include "utils.h"



namespace fs = std::filesystem;


namespace Files {


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

static const int SYS_SNAP_SIZE = sizeof(System_snapshot);
static const int HD_FILE_SIZE_MAX = SYS_SNAP_SIZE;
static const int C64_BIN_SIZE_MIN = 0x0003; // TODO: check
static const int C64_BIN_SIZE_MAX = 0xffff; // TODO: check
static const int T64_SIZE_MIN = 0x60;
static const int G64_SIZE_MIN = 0xffff; // TODO: a more realistic minimum (this is far below...)
static const int CRT_SIZE_MIN = 0x0050;


static const File NO_FILE{File::Type::none, "", {}};


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



File::Type file_type(const Bytes& file) {
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
        return std::size(file) == D64::size
                    || std::size(file) == D64::size_with_error_info;
    };

    auto is_c64_bin = [&]() {
        return (std::size(file) >= C64_BIN_SIZE_MIN) && std::size(file) <= C64_BIN_SIZE_MAX;
    };

    auto is_sys_snap = [&]() {
        const auto& sign = System_snapshot::signature;
        return std::size(file) == SYS_SNAP_SIZE
            && std::equal(std::begin(sign), std::end(sign), std::begin(file));
    };

    using Type = File::Type;

    if (is_crt()) return Type::crt;
    if (is_t64()) return Type::t64;
    if (is_g64()) return Type::g64;
    if (is_d64()) return Type::d64;
    if (is_c64_bin()) return Type::c64_bin;
    if (is_sys_snap()) return Type::sys_snap;

    return Type::unknown;
}


File read(const std::string& path) {
    const auto fs_path = fs::path(path);
    const auto name = fs_path.filename().string();

    try {
        if (fs::file_size(fs_path) <= HD_FILE_SIZE_MAX) {
            if (auto data = read_file(path); data) {
                return File{file_type(*data), name, *data};
            }
        } else {
            Log::error("File oversized");
        }
    } catch (const fs::filesystem_error& e) {
        Log::error("%s", e.what());
    }

    return NO_FILE;
}


File generate_basic_info_list(const File& file) {
    using Type = File::Type;

    switch (file.type) {
        case Type::d64:
            return load_from_d64(D64{file.data}, "$");
        default:
            return NO_FILE;
    }
}


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

    Bytes to_bin() const {
        Bytes bin;

        u16 addr = 0x0801;
        bin.push_back(addr); // load addr. (lo)
        bin.push_back(addr >> 8); // load addr. (hi)

        for(auto& line : listing) {
            const u16 line_len = 2 + 2 + line.text.length() + 1;
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
File hd_dir_basic_listing(
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
        const auto de = std::string(" ") + petscii::arrow_up + " "
                            + quoted(win_drive_list_filename);
        bl.append({ 0x02,  de });
    }
    if (root_entry) {
        const auto re = std::string(" ") + petscii::arrow_up + " \"/\"";
        bl.append({ 0x02, re });
    }
    if (parent_entry) {
        const auto pe = std::string(" ") + petscii::arrow_left + " " + quoted("..");
        bl.append({ 0x02, pe });
    }

    for (const auto& d : sub_dirs) bl.append({ 0x03, " / " + quoted(to_client(d)) });
    for (const auto& f : files)    bl.append({ 0x04, " : " + quoted(to_client(f)) });

    return File{File::Type::c64_bin, "", bl.to_bin()};
}


#ifdef _WIN32
File load_win_drives_list() {
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
    _Loader(const std::string& init_path) {
        auto ip = fs::path(init_path);

        if (ip.is_relative()) ip = fs::current_path() / ip;

        context = fs::is_directory(ip) ? fs::canonical(ip.lexically_normal()) : "/";

        Log::info("Loader context: '%s'", context.string().c_str());
    }

    File operator()(const std::string& what) { return load(what); }

private:
    fs::path context;

    File load(const std::string& what);
    File load_hd(const std::string& what);
    File load_hd_file(const fs::path& path, const std::string& what);
    File load_hd_dir(const fs::path& new_dir);
};


File _Loader::load(const std::string& what) {
    if (fs::is_directory(context)) return load_hd(what);
    if (fs::is_regular_file(context)) return load_hd_file(context, what);
    context = "/";
    return load_hd("");
}


File _Loader::load_hd(const std::string& what) {
    const auto& cur_dir = context;

    try {
        if (what.length() == 0 || what == "$") return load_hd_dir(cur_dir);

        #ifdef _WIN32
        if (what == win_drive_list_filename) return load_win_drives_list();
        #endif

        // if file/dir --> load it
        const std::string name = to_host(what);
        auto path = fs::path(name);
        if (!path.is_absolute()) path = cur_dir / path;
        if (fs::is_directory(path)) return load_hd_dir(path);
        if (fs::is_regular_file(path)) return load_hd_file(path, "");

        // no luck, try to filter something (TODO: special chars (e.g. "+" --> "\+")?)
        std::function<bool(const std::string&)> dir_filter;
        try {
            const auto re = regex(name);
            dir_filter = [re = std::move(re)](const std::string& entry) -> bool {
                return std::regex_match(as_lower(entry), re);
            };
        } catch (const std::regex_error& e) {
            dir_filter = [](const std::string& _) -> bool { UNUSED(_); return true; };
        }

        const auto [dirs, files] = list_dir(cur_dir.string(), dir_filter);

        const auto n = std::size(dirs) + std::size(files);
        if (n == 1) { // if only 1 --> just load it (dir/file)
            if (std::size(dirs) == 1) {
                path = cur_dir / dirs[0];
                return load_hd_dir(path);
            } else {
                path = cur_dir / files[0];
                return load_hd_file(path, "");
            }
        } else if (n > 1) { // if many --> load listing
            auto d = cur_dir.string() + '/' + what;
            return hd_dir_basic_listing(d, dirs, files);
        }
    } catch (const fs::filesystem_error& e) {
        Log::error("%s", e.what());
    }

    return NO_FILE;
}


File _Loader::load_hd_file(const fs::path& path, const std::string& what) {
    using Type = File::Type;

    switch (auto file = read(path.string()); file.type) {
        case Type::t64: return load_from_t64(T64{file.data}, what);
        case Type::d64:
            if (bool is_already_mounted = (context == path); is_already_mounted) {
                if (what == unmount_filename) {
                    context = context.parent_path(); // 'unmount'
                    return load("");
                }
                else return load_from_d64(D64{file.data}, what);
            } else {
                context = path; // 'mount'
                return file;
            }
        default:
            return file;
    }
}


File _Loader::load_hd_dir(const fs::path& new_dir) {
    const auto cd = fs::canonical(new_dir.lexically_normal());
    const auto [dirs, files] = list_dir(cd.string());

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


std::vector<const D64::Dir_entry*> d64_dir_entries(const D64& d64) {
    std::vector<const D64::Dir_entry*> d;
    for (auto bc = d64.block_chain(D64::dir_start); bc.ok(); bc.next()) {
        const D64::Dir_entry* de = (D64::Dir_entry*)bc.data;
        for (int dei = 0; dei < 8; ++dei, ++de) { // 8 entries/sector
            if (de->file_exists()) d.push_back(de);
        }
    }
    return d;
}


Bytes d64_read_file(const D64& d64, const D64::BL& start) {
    Bytes file_data;
    for (auto bc = d64.block_chain(start); bc.ok(); bc.next()) {
        if (bc.last()) {
            auto end = &bc.data[bc.data[1] + 1];
            std::copy(&bc.data[2], end, std::back_inserter(file_data));
            return file_data;
        }
        std::copy(&bc.data[2], &bc.data[256], std::back_inserter(file_data));
        if (file_data.size() >= d64.data.size()) break; // some kind of safe guard...
    }
    return {};
}


File load_from_t64(const T64& t64, const std::string& what) { UNUSED(what);
    auto file = t64.first_file();
    if (std::size(file) > 2 && std::size(file) <= C64_BIN_SIZE_MAX)
        return File{File::Type::c64_bin, "", file};
    else
        return NO_FILE;
}


File d64_dir_basic_listing(const D64& d64) {
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

    const auto header = ctrl_ch::rvs_on + std::string(" $ ")
        + quoted(extract_string(d64.bam().disk_name, 0x00)) + "  "
        + extract_string(d64.bam().disk_id, petscii::nbsp, 2) + " "
        + extract_string(d64.bam().dos_type, petscii::nbsp, 2) + "    ";

    Basic_listing bl{
        { 0x00, std::string((char*)dir_list_first_line) },
        { 0x01, header },
    };

    for (const auto entry : d64_dir_entries(d64)) {
        std::string text(" : \"N                :T    : S  ");
        int c = 0;
        for (; c < 16; ++c) { // TODO: simplify...?
            const char ch = entry->filename[c];
            if (ch == petscii::nbsp) break;
            text[c + 4] = ch;
        }
        text[c + 4] = '"';

        text.replace(22, 5, type_str(entry->file_type));
        text.replace(29, 3, std::to_string(entry->size));

        bl.append({ 0x05, text });
    }

    bl.append({ 0x00, " : ----------------- : FRE : " + std::to_string(d64.bam().blocks_free())});
    bl.append({ 0x09, std::string(" : \"") + unmount_filename + "\"               : *UNMOUNT*" });

    return File{File::Type::c64_bin, "", bl.to_bin()};
}


File load_from_d64(const D64& d64, const std::string& what) {
    if (what == "" || what == "$") return d64_dir_basic_listing(d64);

    std::function<bool(const std::string&)> match;
    try {
        const auto re = regex(what);
        match = [&what, re = std::move(re)](const std::string& name) -> bool {
            return name == what || std::regex_match(as_lower(name), re);
        };
    } catch (const std::regex_error& e) {
        match = [&](const std::string& name) -> bool { return name == what; };
    }

    for (const auto entry : d64_dir_entries(d64)) {
        const auto filename = extract_string(entry->filename);
        if (match(filename)) {
            auto file = d64_read_file(d64, entry->file_start);
            if (file.size() > 0) return File{File::Type::c64_bin, filename, file};
            break;
        }
    }

    return NO_FILE;
}


// TODO: maybe.. does this make any sense?
/*
File load_from_g64(const Bytes& g64_data, const std::string& what) {
    UNUSED(g64_data); UNUSED(what);
    // TODO(?): g64->d64
    return NOT_FOUND;
}
*/


Loader init_loader(const std::string& init_dir) {
    return _Loader(init_dir);
}


} // namespace Files