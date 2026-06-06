#ifndef MOS6502_ASM_H_INCLUDED
#define MOS6502_ASM_H_INCLUDED

#include <string>
#include "core.h"


namespace MOS6502::Asm {

    enum class Addr_mode {
        impl, accu, abs, absx, absy, imm, ind, indx, indy, rel, zp, zpx, zpy
    };

    // TODO: (remove '.asm_format' --> e.g. Disassembler.disasm() should switch on addr.mode instead)
    struct Instruction {
        const u8 code;
        const u8 size;
        const Addr_mode addr_mode;
        const std::string mnemonic;
        const std::string asm_format;
    };

    extern const Instruction instruction[256];

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
    auto disasm(const Bin& bin, const u16 start_addr = 0x0000) {
        Disassembler dis{bin, start_addr};

        std::vector<typename Disassembler<Bin>::Line> lines;

        for (std::size_t bin_pos = 0; bin_pos < std::size(bin); ) {
            lines.push_back(dis.at(bin_pos));
            bin_pos += instruction[bin[bin_pos]].size;
        }

        return lines;
    }


    template<typename Bin>
    auto disasm_first(const Bin& bin, const u16 start_addr = 0x0000) {
        return Disassembler{bin, start_addr}.at(0);
    }

} // namespace MOS6502::Asm


#endif // MOS6502_ASM_H_INCLUDED
