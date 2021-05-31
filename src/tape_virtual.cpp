
#include "tape_virtual.h"
#include "utils.h"
#include "file_utils.h"

#include <string>
#include <fstream>
#include <vector>



std::string get_filename(u8* ram) {
    u16 filename_addr = ram[0xbc] * 0x100 + ram[0xbb];
    u8 filename_len = ram[0xb7];
    return std::string(&ram[filename_addr], &ram[filename_addr + filename_len]);
}


void on_load(System::CPU& cpu, u8* ram, loader& load_file) {
    auto inject = [&](const std::vector<u8>& bin) {
        auto sz = bin.size();

        // load addr (used if 2nd.addr == 0)
        ram[0xc3] = cpu.x;
        ram[0xc4] = cpu.y;

        u8 scnd_addr = ram[0xb9];
        u16 addr = (scnd_addr == 0) ? cpu.y * 0x100 + cpu.x : bin[1] * 0x100 + bin[0];

        // 'load'
        for (u32 b = 2; b < sz; ++b) ram[addr++] = bin[b];

        // end pointer
        ram[0xae] = cpu.x = addr;
        ram[0xaf] = cpu.y = addr >> 8;

        // status
        cpu.clr(NMOS6502::Flag::C); // no error
        ram[0x90] = 0x00; // io status ok
    };

    std::string filename = get_filename(ram);
    auto bin = load_file(filename);
    if (bin && ((*bin).size() > 2)) return inject(*bin);
    else cpu.pc = 0xf704; // jmp to 'file not found'
}


// TODO: check if file already exists (i.e. don't overwrite)?
void on_save(System::CPU& cpu, u8* ram) {
    static const std::string dir = "data/prg/"; // TODO...

    auto do_save = [](const std::string& filename, u16 start_addr, u8* data, size_t sz) -> bool {
        // TODO: check stream state & hadle exceptions
        std::ofstream f(filename, std::ios::binary);
        f << (u8)(start_addr) << (u8)(start_addr >> 8);
        f.write((char*)data, sz);
        std::cout << "\nSaved '" << filename << "', " << (sz + 2) << " bytes ";
        return true;
    };

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

    do_save(filepath, start_addr, &ram[start_addr], sz);

    // status
    cpu.clr(NMOS6502::Flag::C); // no error
    ram[0x90] = 0x00; // io status ok
}


// 'halt' instructions used for trapping
void Tape_virtual::install_kernal_traps(u8* kernal, u8 trap_opc) {
    // trap loading (device 1)
    kernal[0x1539] = trap_opc; // a halting instruction
    kernal[0x153a] = Tape_virtual::Routine::load;
    kernal[0x153b] = 0x60; // rts
    // trap saving (device 1)
    kernal[0x165f] = trap_opc; // a halting instruction
    kernal[0x1660] = Tape_virtual::Routine::save;
    kernal[0x1661] = 0x60; // rts
}


bool Tape_virtual::on_trap(System::CPU& cpu, u8* ram, loader& load_file) {
    u8 tape_routine = cpu.d;
    switch (tape_routine) {
        case Routine::load:
            on_load(cpu, ram, load_file);
            return true;
        case Routine::save:
            on_save(cpu, ram);
            return true;
        default:
            std::cout << "\nUnknown tape routine: " << (int)tape_routine;
            return false;
    }
}