#ifndef DBG_H_INCLUDED
#define DBG_H_INCLUDED

#include <string>
#include <vector>
#include <stdio.h>
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


struct Instruction {
    enum class Addr_mode : u8 {
        impl, abs, absx, absy, imm, ind, indx, indy, rel, zp, zpx, zpy
    };

    const u8 code;
    const u8 size;
    const Addr_mode addr_mode;
    const std::string mnemonic;
    const std::string format;
};

extern const Instruction instruction[256];


template<typename Bytes>
std::vector<std::string> disasm(const Bytes& bytes, const u16 start_addr = 0x0000) {
    std::vector<std::string> asm_list;

    const std::size_t byte_cnt = std::size(bytes);

    for (std::size_t byte_pos = 0; byte_pos < byte_cnt; ) {
        const auto opc = bytes[byte_pos];
        const auto& instr = instruction[opc];
        const u16 pc = start_addr + byte_pos;

        char instr_disasm[16];
        char instr_bytes[9]; // max: 3 bytes + 2 spaces + terminator = 6 + 2 + 1 

        if (bool bytes_missing = (byte_pos + instr.size) > byte_cnt; bytes_missing) {
            sprintf(instr_disasm, "%s ???", instr.mnemonic.c_str());

            const auto extra_bytes = byte_cnt - byte_pos;
            if (extra_bytes == 1) {
                sprintf(instr_bytes, "%02X", bytes[byte_pos]);
            } else { // only 1 or 2 possible
                sprintf(instr_bytes, "%02X %02X", bytes[byte_pos], bytes[byte_pos + 1]);
            }
        } else {
            switch (instr.size) {
                case 1:
                    sprintf(instr_disasm, instr.format.c_str());
                    sprintf(instr_bytes, "%02X", bytes[byte_pos]);
                    break; 
                case 2:
                    sprintf(instr_disasm, instr.format.c_str(), bytes[byte_pos + 1]);
                    sprintf(instr_bytes, "%02X %02X", bytes[byte_pos], bytes[byte_pos + 1]);
                    break;
                case 3:
                    sprintf(instr_disasm, instr.format.c_str(), bytes[byte_pos + 2], bytes[byte_pos + 1]);
                    sprintf(instr_bytes, "%02X %02X %02X", bytes[byte_pos], bytes[byte_pos + 1], bytes[byte_pos + 2]);
                    break;
            }
        }

        char disasm_line[32];
        sprintf(disasm_line, ".%04X  %-8s  %s", pc, instr_bytes, instr_disasm);
        asm_list.push_back(std::string{disasm_line});

        byte_pos += instr.size;
    }

    return asm_list;
}


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
        if (cpu.mop().mopc >= MC::MOPC::dispatch_cli && cpu.mop().mopc <= MC::MOPC::dispatch_brk) tn = 0;
    }
    void do_reset() { cpu.reset(); for (int i = 0; i < 7; ++i, ++cn) exec_cycle(); }

private:
    NMOS6502::Sig cpu_trap {
        [this]() {
            Log::error("****** CPU halted! ******");
            Dbg::print_status(cpu, mem);
        }
    };
};

} // namespace Dbg

#endif // DBG_H_INCLUDED
