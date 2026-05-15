
#include "dbg.h"
#include "utils.h"
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


void print_bus_status(const Core& cpu) {
    std::cout << "b:" << Dbg::print_u16(cpu.s.bus.a);
    std::cout << (cpu.s.bus.rw ? " > " : " < ");
    std::cout << Dbg::print_u8(cpu.s.bus.d);
}


void Dbg::print_status(const Core& cpu, u8* mem) {
    std::cout << "  pc:" << print_u16(cpu.s.pc);
    std::cout << " [ " << print_u8(mem[cpu.s.pc]);
    std::cout << " " << print_u8(mem[(cpu.s.pc+1) & 0xffff]) << " " << print_u8(mem[(cpu.s.pc+2) & 0xffff]);
    std::cout << " ]";

    std::cout << " a:" << print_u8(cpu.s.a);
    std::cout << " x:" << print_u8(cpu.s.x);
    std::cout << " y:" << print_u8(cpu.s.y);
    std::cout << " p:" << print_u8(cpu.s.p);
    std::cout << " [" << flags_str(cpu.s.p) << "]";

    std::cout << " s:" << print_u16(cpu.s.sp);
    std::cout << " [";
    for (int sp = cpu.s.sp, i = 1; i < 4 && (sp + i) <= 0x1ff; ++i) {
        std::cout << " " << print_u8(mem[sp + i]);
    }
    std::cout << " ]";
}


void Dbg::System::tick(u32 cycles, bool verbose) {
    if (verbose) {
        std::cout << "\n  cn   ab  db w  pc  ac xr yr  sp  ps    ps    n/i mcop/n t";
        std::cout << std::endl;
    }

    while (cycles--) {
        if (cpu.s.bus.rw == Core::State::Bus::RW::r) cpu.s.bus.d = mem[cpu.s.bus.a];
        else mem[cpu.s.bus.a] = cpu.s.bus.d;

        // BEWARE
        if (cpu.s.opc() >= OPC::dispatch_post_cli && cpu.s.opc() <= OPC::dispatch_post_brk) {
            tn = 1;
        }

        if (verbose) {
            std::cout << ' ' << print_u16(cn) << ' '
                    << print_u16(cpu.s.bus.a) << ' ' << print_u8(cpu.s.bus.d)
                    << (cpu.s.bus.rw ? "   " : " * ") << print_u16(cpu.s.pc) << ' '
                    << print_u8(cpu.s.a) << ' ' << print_u8(cpu.s.x) << ' ' << print_u8(cpu.s.y) << ' '
                    << print_u16(cpu.s.sp) << ' ' << print_u8(cpu.s.p) << ' ' << flags_str(cpu.s.p) << ' '
                    << (cpu.s.nmi_act ? "n/" : "-/") << (cpu.s.irq_act ? "i " : "- ")
                    << print_u16(cpu.s.opc()) << '/' << (cpu.s.mcc & 0b111) << ' ' << tn;

            if (tn == 1) {
                const auto bytes = Bytes{{mem[cpu.s.bus.a], mem[u16(cpu.s.bus.a + 1)], mem[u16(cpu.s.bus.a + 2)]}};
                std::cout << "  " + as_lower(disasm_first(bytes, cpu.s.bus.a).text) + " ";
            } else {
                std::cout << "  .";
            }
            /*else if (cpu.s.opc() <= 0xff) {
                std::cout << "   " << as_lower(NMOS6502::instruction[cpu.s.opc()].mnemonic) << "";
            }*/

            std::cout << std::endl;
        }

        cpu.tick();

        ++cn;
        ++tn;
    }
}