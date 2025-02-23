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


#endif // EXPANSION_H_INCLUDED