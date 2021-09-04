#include "c1541.h"


/*
TODO:
    - setting to disable idle trap
        - remove dos patch & clear cpu.sig_halt handler
          (do inside the cpu trap handler to avoid any races...?)
    - 'power off' feature? (just keep in idle mode, until powerd on (and do a reset at that point))
*/

void C1541::System::tick() {
    address_space_op(cpu.mar(), cpu.mdr(), cpu.mrw());
    cpu.tick();
    iec.tick();
    dc.tick();
    check_irq();
}

/*
void C1541::System::install_idle_trap() {

    u8* rom_w = const_cast<u8*>(rom);

    // patch the DOS idle loop
    u8 checksum_fix = Trap_OPC::idle_trap - rom_w[0x2c9b];
    rom_w[0x2c9b] = Trap_OPC::idle_trap;
    rom_w[0x2c9d] -= checksum_fix;

    cpu.sig_halt = cpu_trap;
}
*/

/*void C1541::System::_dump(const CPU& cpu, u8* mem) {
    using namespace Dbg;
    using namespace NMOS6502;

    std::cout << "\npc: " << print_u16(cpu.pc);
    std::cout << "   sp: " << print_u16(cpu.spf);
    std::cout << " [";
    for (int sp = cpu.spf, i = 1; i < 9 && (sp + i) <= 0x1ff; ++i) {
        std::cout << " " << print_u8(mem[sp + i]);
    }
    std::cout << " ]";
    std::cout << "\n a: " << print_u8(cpu.a);
    std::cout << "   x: " << print_u8(cpu.x);
    std::cout << "   y: " << print_u8(cpu.y);
    std::cout << "   p: " << print_u8(cpu.p);
    std::cout << " [" << flag_str(cpu.p) << "]";
    //// zpaf, a1, a2, a3, a4
    std::cout << "\nzp: " << print_u16(cpu.zpaf);
    std::cout << " a1: " << print_u16(cpu.a1);
    std::cout << " a2: " << print_u16(cpu.a2);
    std::cout << " a3: " << print_u16(cpu.a3);
    std::cout << " a4: " << print_u16(cpu.a4);
    std::cout << " d: " << print_u8(cpu.d);
    std::cout << " ir: " << print_u8(cpu.ir);
    std::cout << "\n==> mar: " << print_u16(cpu.mar()) << " (" << R16_str[cpu.mcp->ar] << ")";
    std::cout << "   mdr: " << (cpu.mrw() == MC::RW::w ? print_u8(cpu.mdr()) : "??");
    std::cout << " (" << R8_str[cpu.mcp->dr] << ")";
    std::cout << "   r/w: " << MC::RW_str[cpu.mrw()];
    

    std::cout << "\n\nVIA-1:";
    for (u8 r = 0x0; r <= 0x7; ++r) std::cout << ' ' << print_u8(via1._r(r));
    std::cout << "\n      ";
    for (u8 r = 0x8; r <= 0xf; ++r) std::cout << ' ' << print_u8(via1._r(r));
    std::cout << std::endl;
    std::cout << "\nVIA-2:";
    for (u8 r = 0x0; r <= 0x7; ++r) std::cout << ' ' << print_u8(via2._r(r));
    std::cout << "\n      ";
    for (u8 r = 0x8; r <= 0xf; ++r) std::cout << ' ' << print_u8(via2._r(r));
    std::cout << std::endl;
}*/