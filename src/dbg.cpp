
#include "dbg.h"
#include <sstream>
#include <iostream>
#include <iomanip>


using namespace NMOS6502;


std::string Dbg::flag_str(u8 r) {
    std::string s = "";
    s = s
        + std::string((r & 0x80 ) ? "n" : ".") + std::string((r & 0x40 ) ? "v" : ".")
        + std::string((r & 0x20 ) ? "1" : ".") + std::string((r & 0x10 ) ? "b" : ".")
        + std::string((r & 0x08 ) ? "d" : ".") + std::string((r & 0x04 ) ? "i" : ".")
        + std::string((r & 0x02 ) ? "z" : ".") + std::string((r & 0x01 ) ? "c" : ".");
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
    std::cout << "\n";
}


void Dbg::print_mem(u8* mem, u16 page) {
    print_mem(mem, page << 8, (page << 8) + 0xff);
}


void Dbg::print_status(const Core& cpu, u8* mem) {
    std::cout << "\npc: " << print_u16(cpu.pc);
    std::cout << " [ " << print_u8(mem[cpu.pc]);
    std::cout << " " << print_u8(mem[(cpu.pc+1) & 0xffff]) << " " << print_u8(mem[(cpu.pc+2) & 0xffff]);
    std::cout << " ]";
    std::cout << "\nsp: " << print_u16(cpu.spf);
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
    std::cout << "\n\nzp: " << print_u16(cpu.zpaf);
    std::cout << " a1: " << print_u16(cpu.a1);
    std::cout << " a2: " << print_u16(cpu.a2);
    std::cout << " a3: " << print_u16(cpu.a3);
    std::cout << " a4: " << print_u16(cpu.a4);
    std::cout << " d: " << print_u8(cpu.d);
    std::cout << " ir: " << print_u8(cpu.ir);
    std::cout << "\n";
    std::cout << "\n==> mar: " << print_u16(cpu.mar()) << " (" << R16_str[MC::code[cpu.mcc].ar] << ")";
    std::cout << "   mdr: " << (cpu.mrw() == MC::RW::w ? print_u8(cpu.mdr()) : "??");
    std::cout << " (" << R8_str[MC::code[cpu.mcc].dr] << ")";
    std::cout << "   r/w: " << MC::RW_str[cpu.mrw()];
    std::cout << "\n";
    /*
    operator std::string() const {
            return "(" + R16_str[ar] + ") " + R8_str[dr] + " " + RW_str[rw]
                    + PC_inc_str[pc_inc] + " " + MOPC_str[mopc];
        }*/
}


void Dbg::reg_diff(const Core& cpu) { // TODO: support for multiple cores
    static Reg16 r16_snap[Core::REG_CNT];
    static Reg8 r8_snap[Core::REG_CNT * 2];

    std::cout << "\n\n";
    for (int r = 0; r < Core::REG_CNT; ++r) {
        if (r == R16::pc || r == R16::spf || r >= R16::a1) {
            if (r16_snap[r] != cpu.r16[r]) {
                std::cout << R16_str[r] << ": ";
                std::cout << print_u16(r16_snap[r]) << " --> " << print_u16(cpu.r16[r]) << ", ";
                r16_snap[r] = cpu.r16[r];
            }
        } else if (r <= R16::zpaf) {
            int r8 = r * 2;
            if (r8_snap[r8] != cpu.r8[r8]) {
                std::cout << R8_str[r8] << ": ";
                std::cout << print_u8(r8_snap[r8]) << " --> " << print_u8(cpu.r8[r8]) << ", ";
                r8_snap[r8] = cpu.r8[r8];
            }
        }
    }
    std::cout << "\n";
}


void Dbg::step(System& sys, u16 until_pc)
{
    reg_diff(sys.cpu);

    print_status(sys.cpu, sys.mem);
    for (;;) {
        if (sys.tn == 0) {
            std::cout << "==========================================================";
            if (sys.cpu.pc == until_pc) break;
        } else std::cout << "----------------------------------------------------------";

        getchar();
        std::cout << std::dec << "C" << (int)sys.cn << " T" << (int)sys.tn << ": " << std::string(MC::code[sys.cpu.mcc]);

        sys.exec_cycle();
        reg_diff(sys.cpu);
        print_status(sys.cpu, sys.mem);

        if (sys.cpu.halted()) { std::cout << "\n[HALTED]"; break; }
    }
    std::cout << "\n\n=================== DONE (" << sys.cn << " cycles) ======================";
    print_status(sys.cpu, sys.mem);
}
