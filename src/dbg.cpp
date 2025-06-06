
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
    std::cout << "\nsp: " << print_u16(cpu.s.sp);
    std::cout << " [";
    for (int sp = cpu.s.sp, i = 1; i < 9 && (sp + i) <= 0x1ff; ++i) {
        std::cout << " " << print_u8(mem[sp + i]);
    }
    std::cout << " ]";
    std::cout << "\n a: " << print_u8(cpu.s.a);
    std::cout << "   x: " << print_u8(cpu.s.x);
    std::cout << "   y: " << print_u8(cpu.s.y);
    std::cout << "   p: " << print_u8(cpu.s.p);
    std::cout << " [" << flag_str(cpu.s.p) << "]";
    //// zpaf, a1, a2, a3, a4
    std::cout << "\n\nzp: " << print_u16((cpu.s.zph <<8) | cpu.s.zp);
    std::cout << " a1: " << print_u16((cpu.s.a1h << 8) | cpu.s.a1l);
    std::cout << " a2: " << print_u16(cpu.s.a2);
    std::cout << " a3: " << print_u16(cpu.s.a3);
    std::cout << " a4: " << print_u16(cpu.s.a4);
    std::cout << " d: " << print_u8(cpu.s.d);
    std::cout << " ir: " << print_u8(cpu.s.ir);
    std::cout << "\n";
    std::cout << "\n==> mar: " << print_u16(cpu.mar()) << " (" << Ri16_str[cpu.mcp->ar] << ")";
    std::cout << "   mdr: " << (cpu.mrw() == MC::RW::w ? print_u8(cpu.mdr()) : "??");
    std::cout << " (" << Ri8_str[cpu.mcp->dr] << ")";
    std::cout << "   r/w: " << MC::RW_str[cpu.mrw()];
    std::cout << std::endl;
    /*
    operator std::string() const {
            return "(" + R16_str[ar] + ") " + R8_str[dr] + " " + RW_str[rw]
                    + PC_inc_str[pc_inc] + " " + MOPC_str[mopc];
        }*/
}


// TODO: still working? (reg. indices changed....)
void Dbg::reg_diff(const Core& cpu) {
    static Reg16 r16_snap[Ri16::_cnt16];
    static Reg8 r8_snap[Ri8::_cnt8];

    std::cout << "\n\n";
    for (int r = 0; r < Ri16::_cnt16; ++r) {
        if (r == Ri16::pc || r == Ri16::sp16 || r >= Ri16::a1) {
            if (r16_snap[r] != cpu.s.r16((Ri16)r)) {
                std::cout << Ri16_str[r] << ": ";
                std::cout << print_u16(r16_snap[r]) << " --> " << print_u16(cpu.s.r16((Ri16)r)) << ", ";
                r16_snap[r] = cpu.s.r16((Ri16)r);
            }
        } else if (r <= Ri16::zp16) {
            int r8 = r * 2;
            if (r8_snap[r8] != cpu.s.r8((Ri8)r8)) {
                std::cout << Ri8_str[r8] << ": ";
                std::cout << print_u8(r8_snap[r8]) << " --> " << print_u8(cpu.s.r8((Ri8)r8)) << ", ";
                r8_snap[r8] = cpu.s.r8((Ri8)r8);
            }
        }
    }
    std::cout << std::endl;
}


void Dbg::step(System& sys, u16 until_pc)
{
    reg_diff(sys.cpu);

    print_status(sys.cpu, sys.mem);
    for (;;) {
        if (sys.tn == 0) {
            std::cout << "==========================================================";
            if (sys.cpu.s.pc == until_pc) break;
        } else std::cout << "----------------------------------------------------------";

        //getchar();
        std::cout << std::dec << "C" << (int)sys.cn << " T" << (int)sys.tn << ": " << std::string(*sys.cpu.mcp);

        sys.exec_cycle();
        reg_diff(sys.cpu);
        print_status(sys.cpu, sys.mem);

        if (sys.cpu.halted()) { std::cout << "\n[HALTED]"; break; }
    }
    std::cout << "\n\n=================== DONE (" << sys.cn << " cycles) ======================";
    print_status(sys.cpu, sys.mem);
}
