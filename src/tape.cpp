
#include "tape.h"

#include <string>
#include <fstream>
#include <filesystem>
#include <vector>
#include <algorithm>
#include <iterator>



std::string get_filename(u8* ram) {
    u16 filename_addr = ram[0xbc] * 0x100 + ram[0xbb];
    u8 filename_len = ram[0xb7];
    return std::string(&ram[filename_addr], &ram[filename_addr + filename_len]);
}


void load(const std::vector<u8>& bin, System::CPU& cpu, u8* ram) {
    auto sz = bin.size();

    // load addr (used if 2nd.addr == 0)
    ram[0xc3] = cpu.x;
    ram[0xc4] = cpu.y;

    u8 scnd_addr = ram[0xb9];

    u16 addr = (scnd_addr == 0)
        ? cpu.y * 0x100 + cpu.x
        : bin[1] * 0x100 + bin[0];

    for (u16 b = 2; b < sz; ++b)
        ram[addr++] = bin[b];

    // end pointer
    ram[0xae] = cpu.x = addr;
    ram[0xaf] = cpu.y = addr >> 8;

    // status
    cpu.clr(NMOS6502::Flag::C); // no error
    ram[0x90] = 0x00; // io status ok
}


bool save(const std::string& filename, u16 start_addr, u8* data, size_t sz) {
    // TODO: check stream state & hadle exceptions
    std::ofstream f(filename, std::ios::binary);
    f << (u8)(start_addr) << (u8)(start_addr >> 8);
    f.write((char*)data, sz);
    std::cout << "\nSaved '" << filename << "', " << (sz + 2) << " bytes ";
    return true;
}


struct Basic_line {
    u16 num;
    std::string text;
};

using Basic_listing = std::vector<Basic_line>;

std::vector<u8> to_bin(const Basic_listing& bl) {
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
}


// generates a basic program listing (with the dir contents)
Basic_listing dir_basic_listing(const std::string& dir) {
    Basic_listing bl;
    u8 list64[] = { 0x9b, 0x36, 0x34, 0x00 }; // 'list' token, '6', '4', eol
    Basic_line first_line{ 0x0, std::string((char*)list64)};
    bl.push_back(first_line);

    auto di = std::filesystem::directory_iterator(dir);
    std::transform(begin(di), end(di), std::back_inserter(bl),
        [](const std::filesystem::directory_entry& e) -> Basic_line {
            std::string filename(e.path().filename().string());
            for (auto& c : filename) {
                if (c >= 97 && c <= 122) c -= 32; // to upper case
            }
            return { 0x40, std::string(": \"") + filename + '"' };
        }
    );

    return bl;
}


// 'halt' instructions used for trapping
void Tape::Virtual::install_kernal_traps(u8* kernal, u8 trap_opc) {
    // trap loading (device 1)
    kernal[0x1539] = trap_opc; // a halting instruction
    kernal[0x153a] = Tape::Routine::load;
    kernal[0x153b] = 0x60; // rts
    // trap saving (device 1)
    kernal[0x165f] = trap_opc; // a halting instruction
    kernal[0x1660] = Tape::Routine::save;
    kernal[0x1661] = 0x60; // rts
}


void Tape::Virtual::load(System::CPU& cpu, u8* ram) {
    // IDEAs:
    //   - 'LOAD' or 'LOAD""' loads the directory listing (auto-list somehow?)
    //   - listing has entries for sub/parent directories (parent = '..')
    //        LOAD".." --> cd up
    //        LOAD"some-sub-dir" --> cd some-sub-dir
    //   - or 'LOAD' loads & autostart a loader program (native, but can interact with host)
    static const std::string dir = "data/prg/"; // TODO...

    std::string filename = get_filename(ram);

    if (filename == "") {
        Basic_listing bl = dir_basic_listing(dir);
        ::load(to_bin(bl), cpu, ram);
        return;
    }

    std::string filepath = as_lower(dir + filename);

    auto bin = read_file(filepath);
    auto sz = bin.size();
    if (sz > 2) { // min: load addr + 1 byte
        std::cout << "\nLoaded '" << filename << "', " << sz << " bytes ";
        ::load(bin, cpu, ram);
    } else {
        std::cout << "\nFailed to load '" << filename << "'";
        cpu.pc = 0xf704; // jmp to 'file not found'
    }

    return;
}


// TODO: check if file already exists (i.e. don't overwrite)?
void Tape::Virtual::save(System::CPU& cpu, u8* ram) {
    static const std::string dir = "data/prg/"; // TODO...

    std::string filename = get_filename(ram);
    if (filename.length() == 0) {
        // TODO: error_code enum(s)
        cpu.a = 0x08; // missing filename
        cpu.set(NMOS6502::Flag::C); // error    
        return;
    }

    std::string filepath = as_lower(dir + filename);

    // TODO: end < start ??
    u16 start_addr = ram[0xc2] * 0x100 + ram[0xc1];
    u16 end_addr = ram[0xaf] * 0x100 + ram[0xae]; // 1 beyond end
    u16 sz = end_addr - start_addr;

    ::save(filepath, start_addr, &ram[start_addr], sz);

    // status
    cpu.clr(NMOS6502::Flag::C); // no error
    ram[0x90] = 0x00; // io status ok
}
