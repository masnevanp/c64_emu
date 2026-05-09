#ifndef NMOS6502_H_INCLUDED
#define NMOS6502_H_INCLUDED

#include <string>
#include <functional>
#include <cstdint>


namespace NMOS6502 {

using u8 = uint8_t;
using i8 = int8_t;
using u16 = uint16_t;

using Sig  = std::function<void (void)>;

struct Vec { enum a : u16 { nmi = 0xfffa, rst = 0xfffc, irq = 0xfffe, }; };

enum Flag : u8 { // nvubdizc, u = unused (bit 5)
    N = 0b10000000, V = 0b01000000, u = 0b00100000, B = 0b00010000,
    D = 0b00001000, I = 0b00000100, Z = 0b00000010, C = 0b00000001,
    all = 0b11111111,
};

enum OPC {
    brk = 0x00, rts = 0x60,
    reset = 0x100, dispatch_cli, dispatch_sei, dispatch, dispatch_brk
};

enum class Addr_mode {
    impl, accu, abs, absx, absy, imm, ind, indx, indy, rel, zp, zpx, zpy
};

// TODO: (remove '.asm_format' --> e.g. Disassembler.disasm() should switch on addr.mode instead)
struct Instruction {
    const u8 code;
    const u8 size;
    const NMOS6502::Addr_mode addr_mode;
    const std::string mnemonic;
    const std::string asm_format;
};

extern const Instruction instruction[256];

} // namespace NMOS6502


#endif // NMOS6502_H_INCLUDED
