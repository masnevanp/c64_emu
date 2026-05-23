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

std::string flags_str(u8 p);

std::string print_u16(uint16_t x);
std::string print_u8(uint8_t x);

void print_mem(u8* mem, u16 from, u16 to);
void print_mem(u8* mem, u16 page);

void print_status(const Core& cpu, u8* mem);


template<typename Bin>
class Disassembler {
public:
    struct Line {
        // TODO: add opc, pos (if needed)
        const std::string pc; // TODO: change to u16 (if needed)
        const std::string bytes;
        const std::string text;
    };

    Disassembler(const Bin& bin_, const u16 start_addr_ = 0x0000)
        : bin(bin_), start_addr(start_addr_) {}

    Line at(const std::size_t bin_pos) const {
        if (bin_pos >= std::size(bin)) return {"????", "??", "<out of range>"};
        else return disasm(bin_pos);
    }

    Line at_pc(const u16 pc) const {
        if (pc < start_addr) return {"????", "??", "<out of range>"};
        else return at(pc - start_addr);
    }

    const Bin& bin;
    const u16 start_addr;

private:
    Line disasm(const std::size_t bin_pos) const {
        const auto opc = bin[bin_pos];
        const auto& instr = NMOS6502::instruction[opc];
        const u16 pc = start_addr + bin_pos;

        char instr_disasm[16];
        char instr_bytes[9]; // max: 3 bytes + 2 spaces + terminator = 6 + 2 + 1 

        const std::size_t byte_cnt = std::size(bin);

        if (bool bytes_missing = (bin_pos + instr.size) > byte_cnt; bytes_missing) {
            sprintf(instr_disasm, "%s <missing>", instr.mnemonic.c_str());

            const auto extra_bytes = byte_cnt - bin_pos;
            if (extra_bytes == 1) {
                sprintf(instr_bytes, "%02X", bin[bin_pos]);
            } else { // only 1 or 2 possible
                sprintf(instr_bytes, "%02X %02X", bin[bin_pos], bin[bin_pos + 1]);
            }
        } else {
            switch (instr.size) {
                case 1:
                    sprintf(instr_disasm, instr.asm_format.c_str());
                    sprintf(instr_bytes, "%02X", bin[bin_pos]);
                    break; 
                case 2:
                    if (instr.addr_mode == NMOS6502::Addr_mode::rel) {
                        const u16 tgt_addr = pc + 2 + i8(bin[bin_pos + 1]);
                        sprintf(instr_disasm, instr.asm_format.c_str(), tgt_addr);
                        sprintf(instr_bytes, "%02X %02X", bin[bin_pos], bin[bin_pos + 1]);
                    } else {
                        sprintf(instr_disasm, instr.asm_format.c_str(), bin[bin_pos + 1]);
                        sprintf(instr_bytes, "%02X %02X", bin[bin_pos], bin[bin_pos + 1]);
                    }
                    break;
                case 3:
                    sprintf(instr_disasm, instr.asm_format.c_str(), bin[bin_pos + 2], bin[bin_pos + 1]);
                    sprintf(instr_bytes, "%02X %02X %02X", bin[bin_pos], bin[bin_pos + 1], bin[bin_pos + 2]);
                    break;
            }
        }

        return Line {
            print_u16(pc),
            std::string(instr_bytes),
            std::string(instr_disasm)
        };
    }
};


template<typename Bin>
auto disasm(const Bin& bin, const u16 start_addr = 0x0000) {
    Disassembler dis{bin, start_addr};

    std::vector<typename Disassembler<Bin>::Line> lines;

    for (std::size_t bin_pos = 0; bin_pos < std::size(bin); ) {
        lines.push_back(dis.at(bin_pos));
        bin_pos += NMOS6502::instruction[bin[bin_pos]].size;
    }

    return lines;
}


template<typename Bin>
auto disasm_first(const Bin& bin, const u16 start_addr = 0x0000) {
    return Disassembler{bin, start_addr}.at(0);
}


/*
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
            sprintf(instr_disasm, "%s <missing>", instr.mnemonic.c_str());

            const auto extra_bytes = byte_cnt - byte_pos;
            if (extra_bytes == 1) {
                sprintf(instr_bytes, "%02X", bytes[byte_pos]);
            } else { // only 1 or 2 possible
                sprintf(instr_bytes, "%02X %02X", bytes[byte_pos], bytes[byte_pos + 1]);
            }
        } else {
            switch (instr.size) {
                case 1:
                    sprintf(instr_disasm, instr.asm_format.c_str());
                    sprintf(instr_bytes, "%02X", bytes[byte_pos]);
                    break; 
                case 2:
                    if (instr.addr_mode == NMOS6502::Addr_mode::rel) {
                        const u16 tgt_addr = pc + 2 + i8(bytes[byte_pos + 1]);
                        sprintf(instr_disasm, instr.asm_format.c_str(), tgt_addr);
                        sprintf(instr_bytes, "%02X %02X", bytes[byte_pos], bytes[byte_pos + 1]);
                    } else {
                        sprintf(instr_disasm, instr.asm_format.c_str(), bytes[byte_pos + 1]);
                        sprintf(instr_bytes, "%02X %02X", bytes[byte_pos], bytes[byte_pos + 1]);
                    }
                    break;
                case 3:
                    sprintf(instr_disasm, instr.asm_format.c_str(), bytes[byte_pos + 2], bytes[byte_pos + 1]);
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
*/

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
        if (cpu.s.bus.rw == NMOS6502::Core::State::Bus::RW::r) cpu.s.bus.d = mem[cpu.s.bus.a];
        else mem[cpu.s.bus.a] = cpu.s.bus.d;

        cpu.tick();
        ++cn; ++tn;
        // BEWARE
        if (cpu.s.opc() >= NMOS6502::OPC::dispatch_post_cli &&
                cpu.s.opc() <= NMOS6502::OPC::dispatch_post_brk) tn = 0;
    }

private:
    void do_reset() { cpu.reset(); tick(7, false); cn = 0; tn = 0; }

    NMOS6502::Sig_halt cpu_trap {
        [this](u8 opc, u8 d) {
            Log::error("****** CPU halted! (opc: %d, d: %d) ******", opc, d);
            Dbg::print_status(cpu, mem);
        }
    };
};

} // namespace Dbg

#endif // DBG_H_INCLUDED
