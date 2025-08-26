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

struct exrom_game {
    bool exrom;
    bool game;
    exrom_game(bool e, bool g) : exrom(e), game(g) {}
    exrom_game(const Files::CRT& crt)
        : exrom(bool(crt.header().exrom)), game(bool(crt.header().game)) {} 
};


int load_static_chips(const Files::CRT& crt, u8* exp_ram);
int load_chips(const Files::CRT& crt, u8* exp_ram);


struct Null {
    u8* exp_ram;

    Null(u8* exp_ram_) : exp_ram(exp_ram_) {}

    // TODO: reading unconnected areas should return whatever is 'floating' on the bus
    void roml_r(const u16& addr, u8& data) { UNUSED(addr); UNUSED(data); }
    void roml_w(const u16& addr, u8& data) { UNUSED(addr); UNUSED(data); }
    void romh_r(const u16& addr, u8& data) { UNUSED(addr); UNUSED(data); }
    void romh_w(const u16& addr, u8& data) { UNUSED(addr); UNUSED(data); }

    void io1_r(const u16& addr, u8& data) { UNUSED(addr); UNUSED(data); }
    void io1_w(const u16& addr, u8& data) { UNUSED(addr); UNUSED(data); }
    void io2_r(const u16& addr, u8& data) { UNUSED(addr); UNUSED(data); }
    void io2_w(const u16& addr, u8& data) { UNUSED(addr); UNUSED(data); }
};


struct T0 : public Null {
    T0(u8* exp_ram) : Null(exp_ram) {}

    void roml_r(const u16& addr, u8& data) { data = exp_ram[addr]; }
    void romh_r(const u16& addr, u8& data) { data = exp_ram[0x2000 + addr]; }

    bool load(const Files::CRT& crt) {
        return (load_static_chips(crt, exp_ram) > 0) ;
    }
};


bool load_crt(const Files::CRT& crt, u8* exp_ram);


enum Op : u8 {
    roml_r = 0,  roml_w = 1,
    romh_r = 2,  romh_w = 3,
    io1_r  = 4,  io2_r  = 5,
    io1_w  = 6,  io2_w  = 7,

    _cnt = 8
};

inline void do_op(Expansion::Type type, Op op, const u16& addr, u8& data, u8* exp_ram) {
    switch ((type * Op::_cnt) + op) {
        case 0: T0{exp_ram}.roml_r(addr, data); return;
        case 1: T0{exp_ram}.roml_w(addr, data); return;
        case 2: T0{exp_ram}.romh_r(addr, data); return;
        case 3: T0{exp_ram}.romh_w(addr, data); return;
        case 4: T0{exp_ram}.io1_r(addr, data); return;
        case 5: T0{exp_ram}.io2_r(addr, data); return;
        case 6: T0{exp_ram}.io1_w(addr, data); return;
        case 7: T0{exp_ram}.io2_w(addr, data); return;
    }
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

        using r = std::function<void (const u16& addr, u8& data)>;
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