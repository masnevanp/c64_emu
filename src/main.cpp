#include <iostream>
#include <string>

#include <iostream>
#include "nmos6502/nmos6502.h"
#include "nmos6502/nmos6502_core.h"
#include "utils.h"
#include "dbg.h"
#include "system.h"
#include "test.h"


void run_c64() {
    u8 basic[0x2000];
    u8 kernal[0x2000];
    u8 charr[0x1000];

    auto load_roms = [&]() {
        const std::string fp = "data/c64_roms/";
        read_bin_file(fp + "basic.rom", (char*)basic);
        read_bin_file(fp + "kernal.rom", (char*)kernal);
        read_bin_file(fp + "char.rom", (char*)charr);
    };

    /*auto load = [&](const std::string filename, u8* ram) -> bool {
        auto bin = read_bin_file(filename);
        auto sz = bin.size();

        if (sz > 2) {
            std::cout << "Loaded '" << filename << "', " << sz << " bytes ";
        } else {
            std::cout << "Failed to load '" << filename << "'\n";
            return false;
        }
        auto addr = bin[1] * 0x100 + bin[0];
        std::cout << "@ " << (int)addr << "\n";
        for (unsigned int b = 2; b < sz; ++b)
            ram[addr++] = bin[b];

        return true;
    };

    auto load_test = [&](const std::string test_name, u8* ram) -> bool {
        const std::string fn = "data/testsuite-2.15/bin/" + test_name;
        return load(fn, ram);
    };*/

    load_roms();
    System::ROM rom{basic, kernal, charr};
    System::C64 c64(rom);

    // test programs, 'SYS2068' to run

    // --------- fail ---------
    //load_test("trap12", c64.ram);
    //load_test("trap15", c64.ram); (due to trap12 failing)
    //load_test("cpuport", c64.ram);

    // ---------- ok ----------
    //load_test("icr01", c64.ram);
    //load_test("imr", c64.ram);
    //load_test("cia1ta", c64.ram);
    //load_test("cia1tb", c64.ram);
    //load_test("cia2ta", c64.ram);
    //load_test("cia2tb", c64.ram);
    //load_test("cntdef", c64.ram);
    //load_test("loadth", c64.ram);
    //load_test("cia1pb6", c64.ram);
    //load_test("cia1pb7", c64.ram);
    //load_test("cia2pb6", c64.ram);
    //load_test("cia2pb7", c64.ram);
    //load_test("cia1tb123 ", c64.ram);
    //load_test("cia2tb123 ", c64.ram);
    //load_test("cia1tab", c64.ram);
    //load_test("oneshot", c64.ram);
    //load_test("flipos", c64.ram);
    //load_test("cnto2", c64.ram);
    //load_test("cputiming", c64.ram);
    //load_test("irq", c64.ram);
    //load_test("nmi", c64.ram);
    //load_test("branchwrap", c64.ram);
    //load_test("mmu", c64.ram);
    //load_test("trap16", c64.ram);
    //load_test("trap17 ", c64.ram);
    //and the rest of traps

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
    UNUSED(argv);
    UNUSED(args);
    //Test::run_6502_func_test();
    //Test::run_test_suite();
    run_c64();
    //test();
    return 0;
}
