#ifndef FILES_H_INCLUDED
#define FILES_H_INCLUDED

#include <string>
#include <vector>
#include <optional>
#include <numeric>
#include <filesystem>
#include "common.h"



namespace Files {

enum class Type { t64, d64, g64, raw, unsupported };


static constexpr u8 g64_signature[] = { 0x47, 0x43, 0x52, 0x2D, 0x31, 0x35, 0x34, 0x31 };


struct T64 {
    const std::vector<u8>& data;

    struct Dir_entry {
        u8 _todo[2]; // types
        U16 start_address;
        U16 end_address;
        u8 pad1[2];
        U32 file_start;
        u8 pad2[4];
        u8 __todo[16]; // name
    };

    std::vector<u8> first_file() const { // TODO: handle multifile archives?
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


struct D64 {
    const std::vector<u8>& data;

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
        U16 size;

        bool file_exists() const { return file_type.file_type != 0x00; };
    };

    using dir_list = std::vector<const Dir_entry*>;

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

    dir_list dir() const {
        dir_list d;
        for (auto bc = block_chain(dir_start); bc.ok(); bc.next()) {
            const Dir_entry* de = (Dir_entry*)bc.data;
            for (int dei = 0; dei < 8; ++dei, ++de) { // 8 entries/sector
                if (de->file_exists()) d.push_back(de);
            }
        }
        return d;
    }

    std::vector<u8> read_file(const BL& start) {
        std::vector<u8> file_data;
        for (auto bc = block_chain(start); bc.ok(); bc.next()) {
            if (bc.last()) {
                auto end = &bc.data[bc.data[1] + 1];
                std::copy(&bc.data[2], end, std::back_inserter(file_data));
                return file_data;
            }
            std::copy(&bc.data[2], &bc.data[256], std::back_inserter(file_data));
            if (file_data.size() >= data.size()) break; // some kind of safe guard...
        }
        return {};
    }
};


struct G64 {
    const std::vector<u8> data;

    struct Header {
        const u8 signature[8];
        const u8 version;
        const u8 track_count; // 'half track' count
        const U16 max_track_length;
    };

    std::pair<std::size_t, const u8*> track(u8 half_track_num) const {
        if (const u32 tdo = track_data_offset(half_track_num)) {
            u16 len = *(U16*)(&data[tdo]);
            return {len, &data[tdo + 2]};
        }
        return {0, nullptr};
    }

    const Header& header() const { return *((Header*)&data[0]); }

    u32 track_data_offset(u8 track_num) const {
        if (track_num >= header().track_count) return 0;

        const U32* offsets = (U32*)&data[12];
        return offsets[track_num];
    }
};


enum Disk_img_op : u8 {
    fwd  = 0b001, mount = 0b010, describe = 0b100,
    none = 0,
};

using load_result = std::optional<std::vector<u8>>;
using consumer = std::function<void (std::string, Type, std::vector<u8>&)>;
using loader = std::function<load_result(const std::string&, const u8 disk_img_ops)>;

loader Loader(const std::string& init_dir, consumer& img_consumer);


} // namespace Files

#endif // FILES_H_INCLUDED
