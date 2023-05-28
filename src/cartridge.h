#ifndef CARTRIDGE_H_INCLUDED
#define CARTRIDGE_H_INCLUDED

#include "common.h"
#include "files.h"


/*
    Cartridge (and REU) support is possible only due to these (thanks!):
        https://vice-emu.sourceforge.io/vice_17.html
        https://sourceforge.net/p/vice-emu/code/HEAD/tree/trunk/vice/src/c64/cart/
        Wolfgang Moser: Technical Reference Documentation - Commodore RAM Expansion Unit Controller - 8726R1
*/


namespace Cartridge {


bool attach(const Files::CRT& crt, Expansion_ctx& exp_ctx);
void detach(Expansion_ctx& exp_ctx);


class REU { // 1764
public:
    enum R : u8 {
        status = 0, cmd, caddr_l, caddr_h, raddr_l, raddr_h, raddr_b, tlen_l,
        tlen_h, int_mask, addr_ctrl
    };

    enum R_status : u8 {
        // masks
        int_pend = 0b10000000, eob = 0b01000000, ver_err = 0b00100000, sz = 0b00010000,
        chip_ver = 0b00001111,
        // values
        sz_1764 = 0b10000, // same for 1750
        chip_ver_1764 = 0b000, // 0 for all (1700, 1750 and 1764)
    };
    enum R_cmd : u8 {
        // masks
        exec = 0b10000000, autoload = 0b00100000, ff00_trig = 0b00010000, xfer = 0b00000011,
        reserved = 0b01001100,
        // values
        ff00_trig_on = 0b00000000, ff00_trig_off = 0b00010000,
        xfer_c_to_r = 0b00, xfer_r_to_c = 0b01, xfer_swap = 0b10, xfer_ver = 0b11,
    };
    enum R_raddr : u32 {
        // masks
        raddr_bank = 0b11100000000, raddr_unused = 0b1111100000000000,
    };

    // TODO: R_int_mask & R_addr_ctrl

    bool attach(Expansion_ctx& exp_ctx);

    bool dma() {
        return false;
    }

private:
    enum Ctrl_state : u8 {

    };

    u8 sr;
    u8 cr;
    u16 caddr;
    u32 raddr;
    u16 tlen;

    void pre_attach() {
        // TODO: ram init
        std::cout << "REU init" << std::endl;
    }

    Expansion_ctx::Address_space::r io2_r {
        [this](const u16& a, u8& d) {
            std::cout << "REU io2 r: " << (int)a << std::endl;
            switch (a & 0b11111) {
                case R::status:
                    d = sr;
                    sr &= ~(int_pend | eob | ver_err);
                    return;
                case R::cmd:     d = cr;         return;
                case R::caddr_l: d = caddr;      return;
                case R::caddr_h: d = caddr >> 8; return;
                case R::raddr_l: d = raddr;      return;
                case R::raddr_h: d = raddr >> 8; return;
                case R::raddr_b: d = (raddr | R_raddr::raddr_unused) >> 16; return;
                case R::tlen_l:  d = tlen;       return;
                case R::tlen_h:  d = tlen >> 8;  return;
                default:         d = 0xff;       return;
            }
        }
    };

    Expansion_ctx::Address_space::w io2_w {
        [this](const u16& a, const u8& d) {
            std::cout << "REU io2 w: " << (int)a << std::endl;
            switch (a & 0b11111) {
                case R::status: return; // read-only
                default: return;
            }
        }
    };

    std::function<void ()> reset { // @cold reset
        [this]() {
            std::cout << "REU reset" << std::endl;
            sr = 0b00000000 | R_status::sz_1764 | R_status::chip_ver_1764;
            cr = 0b00000000 | R_cmd::ff00_trig_off;
            caddr = 0x0000;
            raddr = 0x0000; // unused bits get 'set' when reading the reg (i.e. raddr_b appears to be 0xf8 after reset)
            tlen = 0xffff;
        }
    };
};


} // namespace Cartridge


#endif // CARTRIDGE_H_INCLUDED