#include <iostream>
#include <string>
#include <vector>

#include "nmos6502/nmos6502.h"
#include "utils.h"
#include "dbg.h"
#include "system.h"
#include "file_utils.h"
#include "iec.h"
#include "iec_devs.h"
#include "tape.h"
#include "test.h"



// Halting instuctions are used as traps
enum Trap_OPC {
    IEC_routine = 0x02,
    tape_routine = 0x12,
};


void run_c64() {
    u8 basic[0x2000];
    u8 kernal[0x2000];
    u8 charr[0x1000];

    auto read_roms = [&]() -> bool {
        return (read_file("data/c64_roms/basic.rom", basic) > 0)
                    && (read_file("data/c64_roms/kernal.rom", kernal) > 0)
                    && (read_file("data/c64_roms/char.rom", charr) > 0);
    };

    if (!read_roms()) return;

    Loader loader("data/prg");

    IEC::Virtual::Controller iec_ctrl;
    Volatile_disk vol_disk;
    Dummy_device dd;
    Host_drive hd(loader);
    iec_ctrl.attach(vol_disk, 10);
    iec_ctrl.attach(dd, 30);
    iec_ctrl.attach(hd, 8);

    Tape::Virtual::install_kernal_traps(kernal, Trap_OPC::tape_routine);
    IEC::Virtual::install_kernal_traps(kernal, Trap_OPC::IEC_routine);

    System::ROM roms{basic, kernal, charr};
    System::C64 c64(roms);

    c64.cpu.sig_halt = [&]() {
        // TODO: verify that it is a kernal trap, e.g. 'banker.mapping(cpu.pc) == kernal' ?
        bool handled = false;
        switch (c64.cpu.ir) {
            case Trap_OPC::IEC_routine:
                handled = IEC::Virtual::on_trap(c64.cpu, c64.ram, iec_ctrl);
                break;
            case Trap_OPC::tape_routine:
                handled = Tape::Virtual::on_trap(c64.cpu, c64.ram, loader);
                break;
        }

        if (handled) {
            ++c64.cpu.mcp; // halted --> bump forward
        } else {
            std::cout << "\n****** CPU halted! ******\n";
            Dbg::print_status(c64.cpu, c64.ram);
        }
    };

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
