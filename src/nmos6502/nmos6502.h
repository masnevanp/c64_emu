#ifndef NMOS6502_H_INCLUDED
#define NMOS6502_H_INCLUDED

#include <string>
#include <functional>
#include <cstdint>


namespace NMOS6502 {

    using u8 = uint8_t;
    using i8 = int8_t;
    using u16 = uint16_t;

    using Reg16 = u16;
    using Reg8 = u8;

    using Sig  = std::function<void (void)>;

    struct Vec { enum { nmi = 0xfffa, rst = 0xfffc, irq = 0xfffe, }; };

    enum Flag : u8 { // nvubdizc, u = unused (bit 5)
        N = 0b10000000, V = 0b01000000, u = 0b00100000, B = 0b00010000,
        D = 0b00001000, I = 0b00000100, Z = 0b00000010, C = 0b00000001,
        all = 0b11111111,
    };

    // 16bit reg indices
    enum Ri16 : u8 {
        zp16 = 0, pc = 1, d_ir = 2, a1 = 3, a2 = 4, sp16 = 5, p_a = 6, x_y = 7,
        a3 = 8, a4 = 9,
        _cnt16 = a4 + 1
    };
    extern const std::string Ri16_str[];

    // 8bit reg indices
    enum Ri8 : u8 { // TODO: big-endian host
        zp = 0, zph = 1, pcl = 2, pch = 3, d = 4, ir = 5, a1l = 6, a1h = 7, a2l = 8, a2h = 9,
        sp = 10, sph = 11, p = 12, a = 13, x = 14, y = 15, a3l = 16, a3h = 17, a4l = 18, a4h = 19,
        _cnt8 = a4h + 1
    };
    extern const std::string Ri8_str[];

    enum OPC {
        brk = 0x00, rts = 0x60, bne = 0xd0,
        reset = 0x100
    };

} // namespace NMOS6502


#endif // NMOS6502_H_INCLUDED
