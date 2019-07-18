#include <iostream>
#include <string>

#include "nmos6502/nmos6502.h"
#include "nmos6502/nmos6502_core.h"
#include "utils.h"
#include "dbg.h"
#include "system.h"
#include "iec.h"
#include "test.h"


static const u8 insta_load_opc = 0xf2;


// 'halt' instructions used for trapping
void patch_insta_load_kernal_trap(u8* kernal) {
    // instant load - trap loading from the tape device (device 1)
    kernal[0x1539] = insta_load_opc; // a halting instruction
    kernal[0x153a] = 0xea; // nop
    kernal[0x153b] = 0x60; // rts
}


void insta_load(System::C64& c64) {
    // IDEAs:
    //   - 'LOAD' or 'LOAD""' loads the directory listing (auto-list somehow?)
    //   - listing has entries for sub/parent directories (parent = '..')
    //        LOAD".." --> cd up
    //        LOAD"some-sub-dir" --> cd some-sub-dir
    //   - or 'LOAD' loads & autostart a loader program (native, but can interact with host)
    static const std::string dir = "data/prg/"; // TODO...
    u16 filename_addr = c64.ram[0xbc] * 0x100 + c64.ram[0xbb];
    u8 filename_len = c64.ram[0xb7];
    std::string filename(&c64.ram[filename_addr], &c64.ram[filename_addr + filename_len]);
    filename = as_lower(dir + filename);

    auto bin = read_bin_file(filename);
    auto sz = bin.size();
    if (sz > 2) {
        std::cout << "\nLoaded '" << filename << "', " << sz << " bytes ";

        // load addr (used if 2nd.addr == 0)
        c64.ram[0xc3] = c64.cpu.x;
        c64.ram[0xc4] = c64.cpu.y;

        u8 scnd_addr = c64.ram[0xb9];

        u16 addr = (scnd_addr == 0)
            ? c64.cpu.y * 0x100 + c64.cpu.x
            : bin[1] * 0x100 + bin[0];

        for (u16 b = 2; b < sz; ++b)
            c64.ram[addr++] = bin[b];

        // end pointer
        c64.ram[0xae] = c64.cpu.x = addr;
        c64.ram[0xaf] = c64.cpu.y = addr >> 8;

        // status
        c64.cpu.clr(NMOS6502::Flag::C); // no error
        c64.ram[0x90] = 0x00; // io status ok
    } else {
        std::cout << "\nFailed to load '" << filename << "'";
        c64.cpu.pc = 0xf704; // jmp to 'file not found'
    }

    return;
}


class Dummy_device : public IEC::Virtual::Device {
public:
    virtual IEC::IO_ST talk(u8 a)         { msg("t", a); x = a; c = 0; return IEC::IO_ST::ok; }
    virtual IEC::IO_ST listen(u8 a)       { msg("l", a); x = a; c = 0; return IEC::IO_ST::ok; }
    virtual void sleep()                  { msg("s", c); }
    virtual IEC::IO_ST read(u8& d)        { msg("r", x); d = x; ++c; return IEC::IO_ST::eof;}
    virtual IEC::IO_ST write(const u8& d) { msg("w", d); x = d; ++c; return IEC::IO_ST::ok; }
private:
    u8 x = 0x00;
    int c = 0;
    void msg(const char* m, int d) { std::cout << " DD:" << m << ',' << d; }
};


void run_c64() {
    u8 basic[0x2000];
    u8 kernal[0x2000];
    u8 charr[0x1000];

    read_bin_file("data/c64_roms/basic.rom", (char*)basic);
    read_bin_file("data/c64_roms/kernal.rom", (char*)kernal);
    read_bin_file("data/c64_roms/char.rom", (char*)charr);

    patch_insta_load_kernal_trap(kernal);
    IEC::Virtual::patch_kernal_traps(kernal);

    System::ROM roms{basic, kernal, charr};
    System::C64 c64(roms);

    IEC::Virtual::Controller iec_ctrl;
    IEC::Virtual::Volatile_disk vol_disk;
    Dummy_device dd;
    iec_ctrl.attach(vol_disk, 10);
    iec_ctrl.attach(dd, 30);

    NMOS6502::Sig on_halt = [&]() {
        // TODO: verify that it is a kernal trap, e.g. 'banker.mapping(cpu.pc) == kernal' ?
        if (c64.cpu.ir == insta_load_opc) {
            insta_load(c64);
        } else if (!IEC::Virtual::handle_trap(c64, iec_ctrl)) {
            std::cout << "\n****** CPU halted! ******\n";
            Dbg::print_status(c64.cpu, c64.ram);
            return;
        }

        ++c64.cpu.mcp; // halted --> bump forward
    };

    c64.cpu.sig_halt = on_halt;

    c64.run();
}


void test()
{
    u8 mem[0x10000]= {
        0x58, 0xea, 0x04, 0xea, 0xea, 0xea, 0xea, 0xea, 0xea, 0xea, 0, 0, 0, 0, 0, 0,
        0xe8, 0x40, 0x58, 0x05, 0xdd, 0x00, 0x32, 0x42, 0, 0, 0, 0, 0, 0, 0, 0,
        0xe8, 0x40, 0x58, 0x05, 0xdd, 0x00, 0x32, 0x42, 0, 0, 0, 0, 0, 0, 0, 0,
        0xe8, 0x60, 0x4c, 0x00, 0x00, 0x00, 0x32, 0x42, 0, 0, 0, 0, 0, 0, 0, 0,
    };
    mem[0xfffa] = 0x10; mem[0xfffb] = 0x00;
    mem[0xfffc] = 0x00; mem[0xfffd] = 0x00;
    mem[0xfffe] = 0x20; mem[0xffff] = 0x00;

    Dbg::System sys{mem};
    sys.cpu.set_irq();
    step(sys);
}


int main(int argv, char** args) {
    UNUSED(argv); UNUSED(args);
    //Test::run_6502_func_test();
    //Test::run_test_suite();
    run_c64();
    //test();
    return 0;
}
