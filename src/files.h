#ifndef FILES_H_INCLUDED
#define FILES_H_INCLUDED

#include <string>
#include <vector>
#include <numeric>
#include "common.h"
#include "state.h"
#include "sid.h"


namespace Files {


struct File {
    enum class Type { none = 0, crt, t64, d64, g64, c64_bin, sys_snap, unknown };

    const Type type;
    const std::string name;
    Bytes data;

    operator bool() const { return type != Type::none; }
    bool identified() const { return (type != Type::none) && (type != Type::unknown); }
};

File read(const std::string& path);

File generate_basic_info_list(const File& file);

using Loader = std::function<File (const std::string&)>;

// TODO: --> 'loader.h' ?
Loader init_loader(const std::string& init_path);


struct System_snapshot {
    static constexpr char signature[16] = {
        'c', '6', '4', '_', 'e', 'm', 'u', '_', 's', 'y', 's', '_', 's', 'n', 'a', 'p'
    };
    static constexpr int sign_len = sizeof(signature) / sizeof(signature[0]);

private:
    char sign[sign_len];
public:
    System_snapshot() {
        for (int c = 0; c < sign_len; ++c) sign[c] = signature[c];
    }

    State::System sys_state;

    reSID_Wrapper::Core::State sid;
};


struct T64 {
    const Bytes& data;

    struct Dir_entry {
        u8 _todo[2]; // types
        U16l start_address;
        U16l end_address;
        u8 pad1[2];
        U32l file_start;
        u8 pad2[4];
        u8 __todo[16]; // name
    };

    Bytes first_file() const { // TODO: handle multifile archives?
        const auto& d = data.data();
        const auto& e = *((Dir_entry*)&d[0x40]);

        auto sz = e.end_address - e.start_address + 1;
        auto so = e.file_start;
        auto eo = e.file_start + sz;

        // try to deal with faulty images (probably fails on a multifile image)
        eo = eo > std::size(data) ? std::size(data) : eo;

        Bytes file({e.start_address.b0, e.start_address.b1});
        std::copy(&d[so], &d[eo], std::back_inserter<Bytes>(file));

        return file;
    }
};


struct D64 {
    const Bytes& data;

    // standard 35-track disk
    static constexpr int track_count = 35;
    static constexpr int block_count = 683;

    static constexpr int block_size = 2 + 254;
    static constexpr int size = block_count * block_size;
    static constexpr int size_with_error_info = size + block_count;

    struct BL { u8 track; u8 sector; }; // block location

    struct Block_chain {
        const D64& d64;
        const u8* data;
        Block_chain(const D64& d64_, const BL& start) : d64(d64_), data(d64.block(start)) {}
        bool next() { if (data) data = d64.block({ data[0], data[1] }); return ok(); }
        bool ok() const { return data != nullptr; }
        bool last() const { return data[0] == 0x00; }
    };

    struct BAM {
        struct Block_alloc {
            u8 free_count;
            u8 free1;
            u8 free2;
            u8 free3;
        };

        BL dir_bl;
        u8 dos_version;
        u8 pad1;
        Block_alloc block_alloc[35];
        u8 disk_name[16];
        u8 pad2[2];
        u8 disk_id[2];
        u8 pad3;
        u8 dos_type[2];
        u8 pad4[89];

        u16 blocks_free() const {
            u16 total = std::accumulate(
                std::begin(block_alloc), std::end(block_alloc), 0,
                    [](u16 a, const Block_alloc& x) { return a + x.free_count; });
            return total - block_alloc[C1541::dir_track - 1].free_count; // disregard bam/dir track
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

        BL next_dir_block;
        File_type file_type;
        BL file_start;
        u8 filename[16];
        BL first_side_sector_block;
        u8 record_length;
        u8 unused[6];
        U16l size;

        bool file_exists() const { return file_type.file_type != 0x00; };
    };

    static constexpr BL bam_block{C1541::dir_track, 0};
    static constexpr BL dir_start{C1541::dir_track, 1};

    static int block_abs_n(u8 track_n, u8 sector_n) {
        if (track_n < C1541::first_track || track_n > C1541::last_track) return -1;
        if (sector_n < C1541::first_sector || sector_n >= C1541::sector_count(track_n)) return -1;

        int block_n = sector_n;
        for (int t = 1; t < track_n; ++t) block_n += C1541::sector_count(t);
        return block_n;
    }

    const u8* block(int abs_n) const {
        return (abs_n < 0 || abs_n >= block_count) ? nullptr : &(data.data()[abs_n * 0x100]);
    }
    const u8* block(const BL& bl) const {
        return block(block_abs_n(bl.track, bl.sector));
    }

    Block_chain block_chain(const BL& start) const { return Block_chain(*this, start); }

    const BAM& bam() const { return *((BAM*)block(bam_block)); }
};


struct G64 {
    const Bytes data;

    struct Header {
        const u8 signature[8];
        const u8 version;
        const u8 track_count; // 'half track' count
        const U16l max_track_length;
    };

    std::pair<std::size_t, const u8*> track(u8 half_track_num) const {
        if (const u32 tdo = track_data_offset(half_track_num)) {
            u16 len = *(U16l*)(&data[tdo]);
            return {len, &data[tdo + 2]};
        }
        return {0, nullptr};
    }

    const Header& header() const { return *((Header*)&data[0]); }

    u32 track_data_offset(u8 track_num) const {
        if (track_num >= header().track_count) return 0;

        const U32l* offsets = (U32l*)&data[12];
        return offsets[track_num];
    }
};


struct CRT {
    const Bytes& data;

    struct Header {
        struct Version { const u8 major; const u8 minor; };

        const u8 signature[16];
        const U32b length;
        const Version version;
        const U16b hw_type;
        const u8 exrom;
        const u8 game;
        const u8 crt_hw;
        const u8 unused[5];
        const u8 name[32];

        bool valid() const { // NOTE: loader checks the signature
            if (exrom > 1 || game > 1) return false;

            return true;
        }
    };

    struct CHIP_packet {
        enum Type : u16 { rom = 0, ram = 1, flash = 2, last = flash };

        const u8 signature[4];
        const U32b length;
        const U16b type;
        const U16b bank;
        const U16b load_addr;
        const U16b data_size;
        const u8 _data_first;

        const u8* data() const { return &_data_first; }

        bool valid() const {
            static constexpr u8 chip[] = { 0x43, 0x48, 0x49, 0x50 };

            if (signature[0] != chip[0] || signature[1] != chip[1]
                    || signature[2] != chip[2] || signature[3] != chip[3]) return false;
            if (u16(length - 0x10) != u16(data_size)) return false;
            if (type > Type::last) return false;

            return true;
        }
    };

    const Header& header() const { return *((Header*)&data[0]); }

    std::vector<const CHIP_packet*> chip_packets() const {
        std::vector<const CHIP_packet*> packets;

        for (u32 offset = header().length; offset < data.size(); ) {
            CHIP_packet* cp = (CHIP_packet*)&data[offset];
            if (!cp->valid()) break;
            if ((offset + cp->length) > data.size()) break;

            packets.push_back(cp);

            offset += cp->length;
        }

        return packets;
    };
};


std::vector<const D64::Dir_entry*> d64_dir_entries(const D64& d64);
Bytes d64_read_file(const D64& d64, const D64::BL& start);

File load_from_t64(const T64& t64, const std::string& what);
File load_from_d64(const D64& d64, const std::string& what);


} // namespace Files

#endif // FILES_H_INCLUDED
