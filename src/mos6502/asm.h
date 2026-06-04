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

} // namespace MOS6502::Asm


#endif // MOS6502_ASM_H_INCLUDED
