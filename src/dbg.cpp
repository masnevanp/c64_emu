
#include "dbg.h"
#include <sstream>
#include <iostream>
#include <iomanip>


using namespace NMOS6502;


std::string Dbg::flags_str(u8 p) {
    using NMOS6502::Flag;

    std::string s = "........";

    if (p & Flag::N) s[0] = 'n';
    if (p & Flag::V) s[1] = 'v';
    if (p & Flag::u) s[2] = '1';
    if (p & Flag::B) s[3] = 'b';
    if (p & Flag::D) s[4] = 'd';
    if (p & Flag::I) s[5] = 'i';
    if (p & Flag::Z) s[6] = 'z';
    if (p & Flag::C) s[7] = 'c';

    return s;
}


std::string Dbg::print_u16(uint16_t x) {
    std::stringstream ss;
    ss << std::hex << std::setw(4) << std::setfill('0') << (int)x;
    return ss.str();
}

std::string Dbg::print_u8(uint8_t x) {
    std::stringstream ss;
    ss << std::hex << std::setw(2) << std::setfill('0') << (int)x;
    return ss.str();
}

void Dbg::print_mem(u8* mem, u16 from, u16 to) {
    const int BPR = 8; // bytes/row
    // std::string ascii;
    // TODO: set 'to' to max(to, from+16)
    for (from -= from % BPR; from <= to; ++from) {
        if (from % BPR == 0) {
            std::cout << '\n';
            //std::cout << ascii << '\n';
            //ascii.clear();
            std::cout << print_u16(from) << ": ";
        }
        //ascii += (char)mem[from];
        std::cout << print_u8(mem[from]) << ' ';
    }
    std::cout << std::endl;
}


void Dbg::print_mem(u8* mem, u16 page) {
    print_mem(mem, page << 8, (page << 8) + 0xff);
}


void Dbg::print_status(const Core& cpu, u8* mem) {
    std::cout << "\npc: " << print_u16(cpu.s.pc);
    std::cout << " [ " << print_u8(mem[cpu.s.pc]);
    std::cout << " " << print_u8(mem[(cpu.s.pc+1) & 0xffff]) << " " << print_u8(mem[(cpu.s.pc+2) & 0xffff]);
    std::cout << " ]";

    std::cout << "   sp: " << print_u16(0x0100 | cpu.s.sp);
    std::cout << " [";
    for (int sp = cpu.s.sp + 0x100, i = 1; i < 9 && (sp + i) <= 0x1ff; ++i) {
        std::cout << " " << print_u8(mem[sp + i]);
    }
    std::cout << " ]";

    std::cout << "\n a: " << print_u8(cpu.s.a);
    std::cout << "   x: " << print_u8(cpu.s.x);
    std::cout << "   y: " << print_u8(cpu.s.y);
    std::cout << "   p: " << print_u8(cpu.s.p);
    std::cout << " [" << flags_str(cpu.s.p) << "]";

    std::cout << "\n";
    std::cout << "\n    ba: " << print_u16(cpu.s.bus_a);
    std::cout << "   bd: " << print_u8(cpu.s.bus_d);
    std::cout << "   r/w: " << print_u8(cpu.s.bus_rw);
}


void Dbg::System::tick(u32 cycles) {
    while (cycles--) {
        if (cpu.s.bus_rw == MC::RW::r) cpu.s.bus_d = mem[cpu.s.bus_a];
        else mem[cpu.s.bus_a] = cpu.s.bus_d;

        // BEWARE
        if (cpu.s.opc() >= OPC::dispatch_cli && cpu.s.opc() <= OPC::dispatch_brk) {
            tn = 0;
        }

        std::string sep = tn == 0 ? "============" : "------------";
        std::cout << std::dec
            << "\n" << sep << " c:" << (int)cn
            << " op:" << print_u16(cpu.s.opc())
            << " t:" << tn
            << sep << "\n";

        print_status(cpu, mem);

        if (tn == 0) {
            const auto bytes = Bytes{{mem[cpu.s.bus_a], mem[u16(cpu.s.bus_a + 1)], mem[u16(cpu.s.bus_a + 2)]}};
            std::cout << "   [" + disasm_first(bytes, cpu.s.bus_a).text + "]";
            cpu.s.pc = cpu.s.bus_a;
        }

        std::cout << std::endl;

        cpu.tick();
        ++cn;
        ++tn;
    }
}