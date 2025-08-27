#ifndef EXPANSION_H_INCLUDED
#define EXPANSION_H_INCLUDED

#include "common.h"
#include "files.h"


/*
    Cartridge support is possible only due to these (thanks!):
        https://vice-emu.sourceforge.io/vice_17.html
        https://sourceforge.net/p/vice-emu/code/HEAD/tree/trunk/vice/src/c64/cart/
*/

/*
    REU support is possible only thanks to this:
        Wolfgang Moser: Technical Reference Documentation - Commodore RAM Expansion Unit Controller - 8726R1
*/


namespace Expansion {

enum Type : u16 {
    REU = 0xfffe, None = 0xffff,
};


int load_static_chips(const Files::CRT& crt, u8* exp_ram);
int load_chips(const Files::CRT& crt, u8* exp_ram);


struct Null {
    State::System& s;

    Null(State::System& s_) : s(s_) {}

    // TODO: reading unconnected areas should return whatever is 'floating' on the bus
    void roml_r(const u16& a, u8& d) { UNUSED(a); UNUSED(d); }
    void roml_w(const u16& a, u8& d) { UNUSED(a); UNUSED(d); }
    void romh_r(const u16& a, u8& d) { UNUSED(a); UNUSED(d); }
    void romh_w(const u16& a, u8& d) { UNUSED(a); UNUSED(d); }

    void io1_r(const u16& a, u8& d) { UNUSED(a); UNUSED(d); }
    void io1_w(const u16& a, u8& d) { UNUSED(a); UNUSED(d); }
    void io2_r(const u16& a, u8& d) { UNUSED(a); UNUSED(d); }
    void io2_w(const u16& a, u8& d) { UNUSED(a); UNUSED(d); }
};


struct T0 : public Null {
    T0(State::System& s) : Null(s) {}

    void roml_r(const u16& a, u8& d) { d = s.exp_ram[a]; }
    void romh_r(const u16& a, u8& d) { d = s.exp_ram[0x2000 + a]; }

    bool load(const Files::CRT& crt) {
        return (load_static_chips(crt, s.exp_ram) > 0) ;
    }
};


bool load_crt(const Files::CRT& crt, State::System& s);


enum Op : u8 {
    roml_r = 0,  roml_w = 1,
    romh_r = 2,  romh_w = 3,
    io1_r  = 4,  io2_r  = 5,
    io1_w  = 6,  io2_w  = 7,

    _cnt = 8
};


inline void do_op(u16 exp_type, Op op, const u16& a, u8& d, State::System& s) {
    #define CASE(t) case (t * Op::_cnt) + Op::roml_r: T##t{s}.roml_r(a, d); return; \
                    case (t * Op::_cnt) + Op::roml_w: T##t{s}.roml_w(a, d); return; \
                    case (t * Op::_cnt) + Op::romh_r: T##t{s}.romh_r(a, d); return; \
                    case (t * Op::_cnt) + Op::romh_w: T##t{s}.romh_w(a, d); return; \
                    case (t * Op::_cnt) + Op::io1_r: T##t{s}.io1_r(a, d); return; \
                    case (t * Op::_cnt) + Op::io2_r: T##t{s}.io2_r(a, d); return; \
                    case (t * Op::_cnt) + Op::io1_w: T##t{s}.io1_w(a, d); return; \
                    case (t * Op::_cnt) + Op::io2_w: T##t{s}.io2_w(a, d); return; \

    switch ((exp_type * Op::_cnt) + op) {
        CASE(0)
    }

    #undef CASE
}


} // namespace Expansion


/*
// gather everything required by an expansion (e.g. cart)
struct Expansion_ctx {
    struct IO {
        IO(const u16& ba_low_,
            std::function<void (const u16&, u8&, const u8 rw)> sys_addr_space_,
            std::function<void (bool e, bool g)> exrom_game_,
            ::IO::Int_sig& int_sig_,
            u16& dma_low_)
          : ba_low(ba_low_), sys_addr_space(sys_addr_space_), exrom_game(exrom_game_),
            int_sig(int_sig_), dma_low(dma_low_) {}

        using r = std::function<void (const u16& a, u8& d)>;
        using w = std::function<void (const u16& addr, const u8& data)>;

        // sys -> exp
        r roml_r;
        w roml_w;
        r romh_r;
        w romh_w;

        r io1_r;
        w io1_w;
        r io2_r;
        w io2_w;

        const u16& ba_low; // low == active

        // exp -> sys
        const std::function<void (const u16&, u8&, const u8 rw)> sys_addr_space;
        const std::function<void (bool e, bool g)> exrom_game;

        ::IO::Int_sig& int_sig;

        u16& dma_low; // low == active
    };

    IO& io;

    ::IO::Bus& sys_bus;

    u8* sys_ram;
    u64& sys_cycle;

    u8* ram;

    std::function<void ()> tick;
    std::function<void ()> reset;
};


namespace Cartridge {

bool attach(const Files::CRT& crt, Expansion_ctx& exp_ctx);
void detach(Expansion_ctx& exp_ctx);

bool attach_REU(Expansion_ctx& exp_ctx);

}
*/

#endif // EXPANSION_H_INCLUDED