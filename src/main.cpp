#include <iostream>
#include "utils.h"
#include "dbg.h"
#include "system.h"
#include "test.h"



void test();


void run_c64() {
    u8 basic[0x2000];
    u8 kernal[0x2000];
    u8 charr[0x1000];
    u8 c1541[0x4000];

    auto read_roms = [&]() -> bool {
        return (read_file("data/c64_roms/basic.rom", basic) > 0)
                    && (read_file("data/c64_roms/kernal.rom", kernal) > 0)
                    && (read_file("data/c64_roms/char.rom", charr) > 0)
                    && (read_file("data/c1541_roms/c1541.rom", c1541) > 0);
    };

    if (!read_roms()) return;

    System::ROM roms{basic, kernal, charr, c1541};
    System::C64 c64(roms);

    c64.run();
}


int main(int argv, char** args) {
    UNUSED(argv); UNUSED(args);
    //Test::run_6502_func_test();
    //Test::run_test_suite();
    run_c64();
    //test();
    return 0;
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
    sys.cpu.set_irq(true);
    step(sys);
}