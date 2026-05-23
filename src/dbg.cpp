
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
    std::cout << " [" << flags_str(cpu.s.p) << "]";
    //// zpaf, a1, a2, a3, a4
    std::cout << "\n\nzp: " << print_u16((cpu.s.zph <<8) | cpu.s.zp);
    std::cout << " a1: " << print_u16((cpu.s.a1h << 8) | cpu.s.a1l);
    std::cout << " a2: " << print_u16(cpu.s.a2);
    std::cout << " a3: " << print_u16(cpu.s.a3);
    std::cout << " a4: " << print_u16(cpu.s.a4);
    std::cout << " d: " << print_u8(cpu.s.d);
    std::cout << " ir: " << print_u8(cpu.s.ir);
    std::cout << "\n";
    std::cout << "\n==> mar: " << print_u16(cpu.mar()) << " (" << Ri16_str[cpu.mop().ar] << ")";
    std::cout << "   mdr: " << (cpu.mrw() == MC::RW::w ? print_u8(cpu.mdr()) : "??");
    std::cout << " (" << Ri8_str[cpu.mop().dr] << ")";
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


void Dbg::System::tick(u32 cycles, bool verbose) {
    if (verbose) {
        std::cout << "\n  cn   ab  db w  pc  ac xr yr  sp  ps    ps    n/i mcop/n t";
        std::cout << std::endl;
    }

    while (cycles-- && !cpu.halted()) {
        if (cpu.mrw() == 1) cpu.mdr() = mem[cpu.mar()];
        else mem[cpu.mar()] = cpu.mdr();

        if (cpu.mop().mopc >= NMOS6502::MC::dispatch_cli && cpu.mop().mopc <= NMOS6502::MC::dispatch_brk) tn = 0;

        if (verbose) {
            std::cout << ' ' << print_u16(cn) << ' '
                    << print_u16(cpu.mar()) << ' ' << print_u8(cpu.mdr())
                    << (cpu.mrw() ? "   " : " * ") << print_u16(cpu.s.pc) << ' '
                    << print_u8(cpu.s.a) << ' ' << print_u8(cpu.s.x) << ' ' << print_u8(cpu.s.y) << ' '
                    << print_u16(cpu.s.sp) << ' ' //<< print_u16(cpu.s.aux) << ' '
                    << print_u8(cpu.s.p) << ' ' << flags_str(cpu.s.p) << ' '
                    << (cpu.s.nmi_act ? "n/" : "-/") << (cpu.s.irq_act ? "i " : "- ")
                    << print_u16(cpu.s.ir) << '/' << (cpu.s.mcc & 0b111) << ' ' << tn;

            if (tn == 0) {
                const auto bytes = Bytes{{mem[cpu.s.pc], mem[u16(cpu.s.pc + 1)], mem[u16(cpu.s.pc + 2)]}};
                std::cout << "  > " + as_lower(disasm_first(bytes, cpu.s.pc).text) + " ";
            } else {
                std::cout << "  .";
            }

            std::cout << std::endl;
        }

        cpu.tick();

        ++cn;
        ++tn;
    }
}