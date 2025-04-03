#ifndef DBG_H_INCLUDED
#define DBG_H_INCLUDED

#include <string>
#include <iostream>
#include "common.h"
#include "nmos6502/nmos6502.h"
#include "nmos6502/nmos6502_core.h"

namespace Dbg {

using namespace NMOS6502;

std::string flag_str(u8 r);

std::string print_u16(uint16_t x);
std::string print_u8(uint8_t x);

void print_mem(u8* mem, u16 from, u16 to);
void print_mem(u8* mem, u16 page);

void print_status(const Core& cpu, u8* mem);

void reg_diff(const Core& cpu);

class System;

void step(System& sys, u16 until_pc = 0xffff);


class System {
public:
    System(u8* mem_, bool do_reset_=true) : mem(mem_) { if (do_reset_) do_reset(); }
    u8* mem;
    Core::State cpu_state;
    Core cpu{cpu_state, cpu_trap};
    uint64_t cn = 1;
    int tn = 0;
    void exec_cycle() {
        if (cpu.mrw() == MC::RW::r) cpu.mdr() = mem[cpu.mar()];
        else mem[cpu.mar()] = cpu.mdr();
        //if (cpu.mar() < 2) print_status(cpu, mem);
        cpu.tick();
        ++cn; ++tn;
        // BEWARE
        if (cpu.mcp->mopc >= MC::MOPC::dispatch_cli && cpu.mcp->mopc <= MC::MOPC::dispatch_brk) tn = 0;
    }
    void do_reset() { cpu.reset_cold(); for (int i = 0; i < 7; ++i, ++cn) exec_cycle(); }

private:
    NMOS6502::Sig cpu_trap {
        [this]() {
            std::cout << "\n****** CPU halted! ******" << std::endl;
            Dbg::print_status(cpu, mem);
        }
    };
};

} // namespace Dbg

#endif // DBG_H_INCLUDED
