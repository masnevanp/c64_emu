#ifndef DBG_H_INCLUDED
#define DBG_H_INCLUDED

#include <string>
#include "common.h"
#include "nmos6502/nmos6502.h"
#include "nmos6502/nmos6502_core.h"

namespace Dbg {

std::string flag_str(u8 r);

std::string print_u16(uint16_t x);
std::string print_u8(uint8_t x);

void print_mem(u8* mem, u16 from, u16 to);
void print_mem(u8* mem, u16 page);

void print_status(const NMOS6502::Core& cpu, u8* mem);

void reg_diff(const NMOS6502::Core& cpu);

class System;

void step(System& sys, u16 until_pc = 0xffff);


class System {
public:
    System(u8* mem_, bool do_reset_=true) : mem(mem_) { if (do_reset_) do_reset(); }
    u8* mem;
    NMOS6502::Core cpu;
    uint64_t cn = 1;
    int tn = 0;
    void exec_cycle() {
        if (cpu.mrw() == NMOS6502::RW::r) cpu.mdr() = mem[cpu.mar()];
        else mem[cpu.mar()] = cpu.mdr();
        //if (cpu.mar() < 2) print_status(cpu, mem);
        cpu.tick();
        ++cn; ++tn;
        // BEWARE
        if (cpu.mcp->mopc >= NMOS6502::MOPC::dispatch && cpu.mcp->mopc <= NMOS6502::MOPC::dispatch_sei) tn = 0;
    }
    void do_reset() { cpu.reset_cold(); for (int i = 0; i < 7; ++i, ++cn) exec_cycle(); }
};

} // namespace Dbg

#endif // DBG_H_INCLUDED
