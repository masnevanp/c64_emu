#ifndef NMOS6502_H_INCLUDED
#define NMOS6502_H_INCLUDED

#include <string>
#include <functional>


namespace NMOS6502 {

    using u8 = uint8_t;
    using i8 = int8_t;
    using u16 = uint16_t;

    using Reg16 = u16;
    using Reg8 = u8;

    using Sig  = std::function<void (void)>;

    enum Vec { nmi = 0xfffa, rst = 0xfffc, irq = 0xfffe, };

    enum Flag : u8 { // nvubdizc, u = unused (bit 5)
        N = 0x80, V = 0x40, u = 0x20, B = 0x10,
        D = 0x08, I = 0x04, Z = 0x02, C = 0x01,
        all = 0xff, none = 0x00
    };

    // 16bit reg indices
    enum R16 : u8 { // TODO: enum class ?
        pc = 0, spf = 1, p_a = 2, x_y = 3,
        d_ir = 4, zpaf = 5, a1 = 6, a2 = 7, a3 = 8, a4 = 9
    };
    extern const std::string R16_str[];

    // 8bit reg indices
    enum R8 : u8 { // TODO: big-endian host
        pcl = 0, pch = 1, sp = 2, sph = 3, p = 4, a = 5, x = 6, y = 7,
        d = 8, ir = 9, zpa = 10, a1l = 12, a1h = 13, a2l = 14, a2h = 15,
    };
    extern const std::string R8_str[];

    // mem-op. direction
    enum RW : u8 { w = 0, r = 1 };
    extern const std::string RW_str[];

    extern const Sig sig_null;

} // namespace NMOS6502


#endif // NMOS6502_H_INCLUDED
