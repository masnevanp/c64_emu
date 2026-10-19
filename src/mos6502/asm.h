#ifndef MOS6502_ASM_H_INCLUDED
#define MOS6502_ASM_H_INCLUDED

#include <string>
#include "core.h"


namespace MOS6502::Asm {
    enum class Op : u8 {
        ADC, AHX, ALR, ANC, AND, ARR, ASL, AXS,
        BCC, BCS, BEQ, BIT, BMI, BNE, BPL, BRK,
        BVC, BVS, CLC, CLD, CLI, CLV, CMP, CPX,
        CPY, DCP, DEC, DEX, DEY, EOR, HLT, INC,
        INX, INY, ISC, JMP, JSR, LAS, LAX, LDA,
        LDX, LDY, LSR, NOP, ORA, PHA, PHP, PLA,
        PLP, RLA, ROL, ROR, RRA, RTI, RTS, SAX,
        SBC, SEC, SED, SEI, SHX, SHY, SLO, SRE,
        STA, STX, STY, TAS, TAX, TAY, TSX, TXA,
        TXS, TYA, XAA,
        count
    };

    extern const char* const Op_str[u8(Op::count)];

    enum class Addr_mode {
        impl, accu, abs, absx, absy, imm, ind, indx, indy, rel, zp, zpx, zpy
    };

    // TODO: (remove '.asm_format' --> e.g. Disassembler.disasm() should switch on addr.mode instead)
    struct Instruction {
        const u8 code;
        const u8 size;
        const Op op;
        const Addr_mode addr_mode;
        const std::string mnemonic;
        const std::string asm_format;
    };

    extern const Instruction instruction[256];

    static constexpr u8 undoc[256] = { // 0 = documented, 1 = undocumented
        /*       -0 -1 -2 -3 -4 -5 -6 -7 -8 -9 -a -b -c -d -e -f  */
        /* 0- */  0, 0, 1, 1, 1, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 1,
        /* 1- */  0, 0, 1, 1, 1, 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 1,
        /* 2- */  0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1,
        /* 3- */  0, 0, 1, 1, 1, 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 1,
        /* 4- */  0, 0, 1, 1, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1,
        /* 5- */  0, 0, 1, 1, 1, 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 1,
        /* 6- */  0, 0, 1, 1, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1,
        /* 7- */  0, 0, 1, 1, 1, 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 1,
        /* 8- */  1, 0, 1, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1,
        /* 9- */  0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 1, 0, 1, 1,
        /* a- */  0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1,
        /* b- */  0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1,
        /* c- */  0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1,
        /* d- */  0, 0, 1, 1, 1, 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 1,
        /* e- */  0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1,
        /* f- */  0, 0, 1, 1, 1, 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 1,
    };


    inline const Instruction& instr(u8 opc) { return instruction[opc]; }

    inline bool is_undoc(u8 opc) { return undoc[opc]; }

    inline bool is_hlt(u8 opc) { return ((opc & 0x0f) == 0x02) && !((opc & 0x90) == 0x80); }


    struct Line {
        // TODO: add opc, pos (if needed)
        const std::string pc; // TODO: change to u16 (if needed)
        const std::string bytes;
        const std::string text;
    };

    template<typename Bin>
    class Disassembler {
    public:
        Disassembler(const Bin& bin_, u16 start_addr_ = 0x0000)
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
            const auto& instr = instruction[opc];
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
                        if (instr.addr_mode == Addr_mode::rel) {
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

            char pc_hex[5];
            sprintf(pc_hex, "%04X", pc);

            return Line {
                std::string(pc_hex),
                std::string(instr_bytes),
                std::string(instr_disasm)
            };
        }
    };


    template<typename Bin>
    auto disasm(const Bin& bin, u16 start_addr = 0x0000) {
        Disassembler dis{bin, start_addr};

        std::vector<typename Disassembler<Bin>::Line> lines;

        for (std::size_t bin_pos = 0; bin_pos < std::size(bin); ) {
            lines.push_back(dis.at(bin_pos));
            bin_pos += instruction[bin[bin_pos]].size;
        }

        return lines;
    }


    template<typename Bin>
    auto disasm_first(const Bin& bin, u16 start_addr = 0x0000) {
        return Disassembler{bin, start_addr}.at(0);
    }


    auto inline disasm_one(u16 addr, u8 b1, u8 b2 = 0x00, u8 b3 = 0x00) {
        const u8 bytes[] = {b1, b2, b3};
        return disasm_first(bytes, addr);
    }

} // namespace MOS6502::Asm


#endif // MOS6502_ASM_H_INCLUDED
