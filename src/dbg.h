#ifndef DBG_H_INCLUDED
#define DBG_H_INCLUDED

#include <string>
#include <vector>
#include <stdio.h>
#include "common.h"
#include "mos6502/core.h"
#include "mos6502/asm.h"

namespace Dbg {

using namespace MOS6502;

std::string flags_str(u8 p);

std::string print_u16(uint16_t x);
std::string print_u8(uint8_t x);

void print_mem(u8* mem, u16 from, u16 to);
void print_mem(u8* mem, u16 page);

void print_status(const Core& cpu, u8* mem);


class System {
public:
    System(u8* mem_) : mem(mem_) { do_reset(); }

    u8* mem;
    Core::State cpu_state;
    Core cpu{cpu_state, cpu_trap};

    uint64_t cn = 1;
    int tn = 0;

    void tick(u32 cycles = 1, bool verbose = true);

    void tick_one() {
        if (cpu.s.bus.rw == MOS6502::Core::State::Bus::RW::r) cpu.s.bus.d = mem[cpu.s.bus.a];
        else mem[cpu.s.bus.a] = cpu.s.bus.d;

        cpu.tick();
        ++cn; ++tn;
        // BEWARE
        if (cpu.s.opc() >= MOS6502::OPC::dispatch_post_cli &&
                cpu.s.opc() <= MOS6502::OPC::dispatch_post_brk) tn = 0;
    }

private:
    void do_reset() { cpu.reset(); tick(7, false); cn = 0; tn = 0; }

    MOS6502::Sig_halt cpu_trap {
        [this](u8 opc, u8 d) {
            Log::error("****** CPU halted! (opc: %d, d: %d) ******", opc, d);
            Dbg::print_status(cpu, mem);
        }
    };
};

} // namespace Dbg

#endif // DBG_H_INCLUDED
