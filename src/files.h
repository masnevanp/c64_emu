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
};

File read(const std::string& path);


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

    u8 cpu_mcp;
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

    Bytes read_file(const BL& start) {
        Bytes file_data;
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
    const Bytes& img;

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
            if (hw_type > Cartridge_HW_type::last) return false; // TODO: does this buy anything?
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

    const Header& header() const { return *((Header*)&img[0]); }

    std::vector<const CHIP_packet*> chip_packets() const {
        std::vector<const CHIP_packet*> packets;

        for (u32 offset = header().length; offset < img.size(); ) {
            CHIP_packet* cp = (CHIP_packet*)&img[offset];
            if (!cp->valid()) break;
            if ((offset + cp->length) > img.size()) break;

            packets.push_back(cp);

            offset += cp->length;
        }

        return packets;
    };

    // Source: VICE manual (https://vice-emu.sourceforge.io/vice_17.html)
    enum Cartridge_HW_type : u16 { // sweet...
        T0_Normal_cartridge = 0, T1_Action_Replay = 1, T2_KCS_Power_Cartridge = 2,
        T3_Final_Cartridge_III = 3, T4_Simons_BASIC = 4, T5_Ocean_type_1 = 5,
        T6_Expert_Cartridge = 6, T7_Fun_Play_Power_Play = 7, T8_Super_Games = 8,
        T9_Atomic_Power = 9, T10_Epyx_Fastload = 10, T11_Westermann_Learning = 11,
        T12_Rex_Utility = 12, T13_Final_Cartridge_I = 13, T14_Magic_Formel = 14,
        T15_C64_Game_System_System_3 = 15, T16_Warp_Speed = 16, T17_Dinamic = 17,
        T18_Zaxxon_Super_Zaxxon_SEGA = 18, T19_Magic_Desk_Domark_HES_Australia = 19, T20_Super_Snapshot_V5 = 20,
        T21_Comal_80 = 21, T22_Structured_BASIC = 22, T23_Ross = 23,
        T24_Dela_EP64 = 24, T25_Dela_EP7x8 = 25, T26_Dela_EP256 = 26,
        T27_Rex_EP256 = 27, T28_Mikro_Assembler = 28, T29_Final_Cartridge_Plus = 29,
        T30_Action_Replay_4 = 30, T31_Stardos = 31, T32_EasyFlash = 32,
        T33_EasyFlash_Xbank = 33, T34_Capture = 34, T35_Action_Replay_3 = 35,
        T36_Retro_Replay = 36, T37_MMC64 = 37, T38_MMC_Replay = 38,
        T39_IDE64 = 39, T40_Super_Snapshot_V4 = 40, T41_IEEE_488 = 41,
        T42_Game_Killer = 42, T43_Prophet64 = 43, T44_EXOS = 44,
        T45_Freeze_Frame = 45, T46_Freeze_Machine = 46, T47_Snapshot64 = 47,
        T48_Super_Explode_V5_0 = 48, T49_Magic_Voice = 49, T50_Action_Replay_2 = 50,
        T51_MACH_5 = 51, T52_Diashow_Maker = 52, T53_Pagefox = 53,
        T54_Kingsoft = 54, T55_Silverrock_128K_Cartridge = 55, T56_Formel_64 = 56,
        T57_RGCD = 57, T58_RR_Net_MK3 = 58, T59_EasyCalc = 59,
        T60_GMod2 = 60, T61_MAX_Basic = 61, T62_GMod3 = 62,
        T63_ZIPP_CODE_48 = 63, T64_Blackbox_V8 = 64, T65_Blackbox_V3 = 65,
        T66_Blackbox_V4 = 66, T67_REX_RAM_Floppy = 67, T68_BIS_Plus = 68,
        T69_SD_BOX = 69, T70_MultiMAX = 70, T71_Blackbox_V9 = 71,
        T72_Lt_Kernal_Host_Adaptor = 72, T73_RAMLink = 73, T74_HERO = 74,
        last = T74_HERO,
    };
};

} // namespace Files

#endif // FILES_H_INCLUDED
