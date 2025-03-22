#ifndef NMOS6502_H_INCLUDED
#define NMOS6502_H_INCLUDED

#include <string>
#include <functional>
#include <cstdint>


namespace NMOS6502 {

    using u8 = uint8_t;
    using i8 = int8_t;
    using u16 = uint16_t;
    using i16 = int16_t;

    using Sig  = std::function<void (void)>;

    enum Vec : u16 { nmi = 0xfffa, rst = 0xfffc, irq = 0xfffe, };

    enum Flag : u8 { // nvubdizc, u = unused (bit 5)
        N = 0b10000000, V = 0b01000000, u = 0b00100000, B = 0b00010000,
        D = 0b00001000, I = 0b00000100, Z = 0b00000010, C = 0b00000001,
        all = 0b11111111,
    };

    // 16bit reg indices
    enum Ri16 : u8 {
        pc = 0, zpa = 1, sp = 2, a1 = 3, a2 = 4, a3 = 5, a4 = 6,
        ri16_none = 0xff,
    };
    extern const std::string Ri16_str[];

    // 8bit reg indices
    enum Ri8 : u8 { // TODO: big-endian host
        pcl = 0, pch = 1, zpal = 2, zpah = 3, spl = 4, sph = 5, a1l = 6, a1h = 7,
        a2l = 8, a2h = 9, a3l = 10, a3h = 11, a4l = 12, a4h = 13,
        d = 14, ir = 15, p = 16, a = 17, x = 18, y = 19,
        ri8_none = 0xff,
    };
    extern const std::string Ri8_str[];

    enum OPC {
        brk = 0x00, bne = 0xd0,
        reset = 0x100,
        _last = reset
    };

} // namespace NMOS6502


#endif // NMOS6502_H_INCLUDED
