
#include "file_utils.h"
#include <filesystem>
#include <regex>
#include <iostream>
#include <algorithm>
#include <numeric>
#include <iterator>
#include "utils.h"



namespace fs = std::filesystem;


namespace petscii { // TOREDO...
    static constexpr char rvs_on     = 0x12;
    static constexpr char slash      = 0x2f;
    static constexpr char arrow_up   = 0x5e;
    static constexpr char arrow_left = 0x5f;
    static constexpr char nbsp       = (char)0xa0;
    static constexpr char underscore = (char)0xa4;
}


static const char* win_drive_list_entry = "?:/";
static const char* eject_entry = "EJECT:";
static const char* eject_entry_short = ":";

static const int MAX_HD_FILE_SIZE = 0xfffff; // TODO: check
static const int MAX_C64_BIN_SIZE = 0xffff; // TODO: check
static const int D64_SIZE = 174848;
static const int D64_WITH_ERROR_INFO_SIZE = 174848 + 683;
static const int T64_MIN_SIZE = 0x60;


static const auto& NOT_FOUND = std::nullopt;


static constexpr u8 dir_list_first_line[] = { // 0 PRINT"<clr>":LIST1-
    0x99, 0x22, 0x93, 0x22, 0x3a,  // 'print' token, '"', '<clr>', '"', ':'
    0x9b, 0x31, 0xab, 0x00 // 'list' token, '1', '-', eol
};

static constexpr u8 t64_signatures[][16] = {
    { 0x43, 0x36, 0x34, 0x53, 0x20, 0x74, 0x61, 0x70, 0x65, 0x20, 0x69, 0x6D, 0x61, 0x67, 0x65, 0x20 },
    { 0x43, 0x36, 0x34, 0x20, 0x74, 0x61, 0x70, 0x65, 0x20, 0x69, 0x6D, 0x61, 0x67, 0x65, 0x20, 0x66 },
    { 0x43, 0x36, 0x34, 0x53, 0x20, 0x74, 0x61, 0x70, 0x65, 0x20, 0x66, 0x69, 0x6c, 0x65, 0x0d, 0x0a },
};


using dir_listing = std::pair<std::vector<std::string>, std::vector<std::string>>;
using dir_filter = std::optional<std::function<bool(const std::string&)>>;


struct U16 {
    u8 b0; u8 b1;
    operator u16() const { return (b1 << 8) | b0; }
};

struct U32 {
    u8 b0; u8 b1; u8 b2; u8 b3;
    operator u32() const { return (b3 << 24) | (b2 << 16) | (b1 << 8) | b0; }
};


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


bool is_t64(const std::vector<u8>& file) {
    if (std::size(file) >= T64_MIN_SIZE) {
        for (const auto& sig : t64_signatures) {
            if (std::equal(std::begin(sig), std::end(sig), std::begin(file)))
                return true;
        }
    }
    return false;
}


class Basic_listing {
public:
    struct Line { u16 num; std::string text; };

    Basic_listing(std::initializer_list<Line> lines) : listing(lines) {}

    void append(Line line) { listing.push_back(line); }

    std::vector<u8> to_bin() const {
        std::vector<u8> bin;

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
std::vector<u8> hd_dir_basic_listing(
    const std::string& dir,
    const std::vector<std::string>& sub_dirs,
    const std::vector<std::string>& files,
    bool drives_entry = false, bool root_entry = false, bool parent_entry = false)
{
    auto header = petscii::rvs_on + std::string(" $ ") + quoted(to_client(dir));

    Basic_listing bl{
        { 0x00, std::string((char*)dir_list_first_line) },
        { 0x01, header },
    };

    if (drives_entry) {
        static auto de = std::string(" ") + petscii::arrow_up + " "
                            + quoted(win_drive_list_entry);
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
std::vector<u8> load_win_drives_list() {
    std::vector<std::string> drives;
    std::string d = "?:/";
    for (char c : "ABCDEFGHIJKLMNOPQRSTUWVXYZ") { // good ol' trial & error...
        d[0] = c;
        if (fs::is_directory(d)) drives.push_back(d);
    }

    return hd_dir_basic_listing(win_drive_list_entry, drives, std::vector<std::string>());
}
#endif


class _Loader {
public:

    _Loader(const std::string& init_dir) : context{init_dir} {}

    load_result operator()(const std::string& what) { return load(what); }

private:
    fs::path context;

    load_result load(const std::string& what) {
        if (fs::is_directory(context)) return load_hd(what);
        if (fs::is_regular_file(context)) return load_hd_file(context, what);
        context = "/";
        return load_hd("");
    }

    std::vector<u8> load_hd_dir(const fs::path& new_dir) {
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

    load_result load_hd_file(const fs::path& path, const std::string& what) {
        if (fs::file_size(path) > MAX_HD_FILE_SIZE) return NOT_FOUND;

        if (auto file = read_file(path.string())) {
            if (is_t64(*file)) {
                // context = path;
                return load_t64(*file, what);
            }

            auto sz = std::size(*file);
            if (sz == D64_SIZE || sz == D64_WITH_ERROR_INFO_SIZE) {
                context = path;
                return load_d64(*file, what);
            }

            if (sz <= MAX_C64_BIN_SIZE) return file;
        }

        return NOT_FOUND;
    }

    load_result load_hd(const std::string& what);
    load_result load_d64(std::vector<u8>& d64_data, const std::string& what);
    load_result load_t64(std::vector<u8>& t64_data, const std::string& what);

    load_result eject() {
        context = context.parent_path();
        return load("");
    }
};


load_result _Loader::load_hd(const std::string& what) {
    const auto& cur_dir = context;

    try {
        if (what.length() == 0 || what == "$") return load_hd_dir(cur_dir);

        #ifdef _WIN32
        if (what == win_drive_list_entry) return load_win_drives_list();
        #endif

        // if file/dir --> load it
        std::string name = to_host(what);
        auto path = fs::path(name);
        if (!path.is_absolute()) path = cur_dir / path;
        if (fs::is_directory(path)) return load_hd_dir(path);
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
        std::cout << e.what() << std::endl;
    }

    return NOT_FOUND;
}


struct D64 {
    const std::vector<u8> data;

    static constexpr u8 dir_track = 18;

    struct TS { u8 track; u8 sector; };

    static constexpr TS bam_sector{dir_track, 0};
    static constexpr TS dir_start{dir_track, 1};

    struct Sector_chain {
        const D64& d64;
        const u8* data;
        Sector_chain(const D64& d64_, const TS& start) : d64(d64_) { data = d64.sector(start); }
        bool next() { if (data) data = d64.sector(data[0], data[1]); return data != nullptr; }
        bool last() const { return data[0] == 0x00; }
    };

    struct BAM {
        struct TS_alloc {
            u8 free_count;
            u8 free1;
            u8 free2;
            u8 free3;
        };

        TS dir_ts;
        u8 dos_version;
        u8 pad1;
        TS_alloc ts_alloc[35];
        u8 disk_name[16];
        u8 pad2[2];
        u8 disk_id[2];
        u8 pad3;
        u8 dos_type[2];
        u8 pad4[89];

        u16 blocks_free() const {
            u16 total = std::accumulate(
                std::begin(ts_alloc), std::end(ts_alloc), 0,
                    [](u16 a, const TS_alloc& x) { return a + x.free_count; });
            return total - ts_alloc[dir_track - 1].free_count; // disregard bam/dir track
        }
    };

    struct Dir_entry {
        struct File_type {
            u8 file_type;
            enum Type   { del=0, seq, prg, usr, rel, bad, mask=0xf };
            enum Status { lock = 0x40, close = 0x80 };
            Type type() const {
                const Type t = (Type)(file_type & Type::mask);
                return t < Type::bad ? t : Type::bad;
            }
            bool locked() const { return file_type & Status::lock; }
            bool closed() const { return file_type & Status::close; }
        };

        TS next_dir_sector;
        File_type file_type;
        TS file_start;
        u8 filename[16];
        TS first_side_sector_block;
        u8 record_length;
        u8 unused[6];
        U16 size;

        bool file_exists() const { return file_type.file_type != 0x00; };
    };

    using dir_list = std::vector<const Dir_entry*>;

    const u8* sector(int abs_n) const {
        return (abs_n < 0 || abs_n > 682) ? nullptr : &(data.data()[abs_n * 0x100]);
    }

    const u8* sector(int track_n, int sector_n) const {
        enum { first_sector = 0, first_track = 1, last_track = 35 };
        static constexpr int sector_count[36] = {
            0,  21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
            21, 21, 19, 19, 19, 19, 19, 19, 19, 18, 18, 18, 18, 18, 18, 17,
            17, 17, 17, 17
        };
        if (track_n < first_track || track_n > last_track) return nullptr;
        if (sector_n < first_sector || sector_n >= sector_count[track_n]) return nullptr;
        for (int t = 1; t < track_n; ++t) sector_n += sector_count[t];
        return sector(sector_n);
    }
    const u8* sector(const TS& ts) const { return sector(ts.track, ts.sector); }

    Sector_chain sector_chain(const TS& start) const { return Sector_chain(*this, start); }

    const BAM& bam() const { return *((BAM*)sector(bam_sector)); }

    dir_list dir() const {
        dir_list d;
        for (auto dsc = sector_chain(dir_start); dsc.data; dsc.next()) {
            Dir_entry* de = (Dir_entry*)dsc.data;
            for (int dei = 0; dei < 8; ++dei, ++de) { // 8 entries/sector
                if (de->file_exists()) d.push_back(de);
            }
        }
        return d;
    }

    std::vector<u8> read_file(const TS& start) {
        std::vector<u8> data;
        for (auto fsc = sector_chain(start); true; fsc.next()) {
            if (fsc.last()) {
                auto end = &fsc.data[fsc.data[1] + 1];
                std::copy(&fsc.data[2], end, std::back_inserter(data));
                return data;
            }
            std::copy(&fsc.data[2], &fsc.data[256], std::back_inserter(data));
        }
    }
};


load_result d64_dir_basic_listing(const D64& d64) {
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

    auto header = petscii::rvs_on + std::string(" $ ")
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

    bl.append({ 0x09, std::string(" ^ ") + quoted(to_client(eject_entry)) });
    bl.append({ d64.bam().blocks_free(), "BLOCKS FREE."});

    return bl.to_bin();
}


load_result _Loader::load_d64(std::vector<u8>& d64_data, const std::string& what) {
    if (what == eject_entry || what == eject_entry_short) return eject();

    D64 d64{std::move(d64_data)};

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


struct T64 {
    const std::vector<u8> data;

    struct Dir_entry {
        u8 _todo[2]; // types
        U16 start_address;
        U16 end_address;
        u8 pad1[2];
        U32 file_start;
        u8 pad2[4];
        u8 __todo[16]; // name
    };

    std::vector<u8> MUCH_TODO__so_for_now__just_try_to_read_the_first_file() const {
        const auto& d = data.data();
        const auto& e = *((Dir_entry*)&d[0x40]);

        auto sz = e.end_address - e.start_address + 1;
        auto so = e.file_start;
        auto eo = e.file_start + sz;

        // try to deal with falty images (probably fails on a multifile image)
        eo = eo > std::size(data) ? std::size(data) : eo;

        std::vector<u8> file({e.start_address.b0, e.start_address.b1});
        std::copy(&d[so], &d[eo], std::back_inserter<std::vector<u8>>(file));

        return file;
    }
};


load_result _Loader::load_t64(std::vector<u8>& t64_data, const std::string& what) { UNUSED(what);
    T64 t64{std::move(t64_data)};
    auto file = t64.MUCH_TODO__so_for_now__just_try_to_read_the_first_file();
    if (std::size(file) > 2 && std::size(file) <= MAX_C64_BIN_SIZE)
        return file;
    else
        return NOT_FOUND;
}


loader Loader(const std::string& init_dir) {
    return _Loader(init_dir);
}
