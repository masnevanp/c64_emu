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
        xfer_c_to_r = 0b00, xfer_r_to_c = 0b01, xfer_swap = 0b10, xfer_ver = 0b11,
    };
    enum R_raddr : u32 {
        // masks
        raddr_bank = 0b11100000000, raddr_unused = 0b1111100000000000,
    };
    enum R_int_mask : u8 {
        // masks
        int_ena = 0b10000000, eob_m = 0b01000000, ver_err_m = 0b00100000, unused_im = 0b00001111,
    };
    enum R_addr_ctrl : u8 {
        // masks
        addr_c = 0b10000000, addr_r = 0b01000000, unused_ac = 0b00111111,
    };

    bool attach(Expansion_ctx& exp_ctx);

    bool dma() {
        return false;
    }

private:
    enum Ctrl_state : u8 {

    };

    struct Reg_file {
        u32 raddr;
        u16 caddr;
        u16 tlen;
        u8 status;
        u8 cmd;
        u8 int_mask;
        u8 addr_ctrl;
    };
    Reg_file r;

    void pre_attach() {
        // TODO: ram init
        std::cout << "REU init" << std::endl;
    }

    Expansion_ctx::Address_space::r io2_r {
        [this](const u16& a, u8& d) {
            std::cout << "REU io2 r: " << (int)a << std::endl;
            switch (a & 0b11111) {
                case R::status:
                    d = r.status | R_status::sz_1764 | R_status::chip_ver_1764;
                    r.status = 0x00;
                    return;
                case R::cmd:       d = r.cmd;        return;
                case R::caddr_l:   d = r.caddr;      return;
                case R::caddr_h:   d = r.caddr >> 8; return;
                case R::raddr_l:   d = r.raddr;      return;
                case R::raddr_h:   d = r.raddr >> 8; return;
                case R::raddr_b:   d = (r.raddr | R_raddr::raddr_unused) >> 16; return;
                case R::tlen_l:    d = r.tlen;       return;
                case R::tlen_h:    d = r.tlen >> 8;  return;
                case R::int_mask:  d = r.int_mask;   return;
                case R::addr_ctrl: d = r.addr_ctrl;  return;
                default:           d = 0xff;         return;
            }
        }
    };

    Expansion_ctx::Address_space::w io2_w {
        [this](const u16& a, const u8& d) {
            std::cout << "REU io2 w: " << (int)a << std::endl;
            switch (a & 0b11111) {
                case R::status: return; // read-only
                case R::int_mask:  r.int_mask = d | R_int_mask::unused_im;   return; 
                case R::addr_ctrl: r.addr_ctrl = d | R_addr_ctrl::unused_ac; return; 
                default: return;
            }
        }
    };

    std::function<void ()> reset { // @cold reset
        [this]() {
            std::cout << "REU reset" << std::endl;
            r.status = 0x00;
            r.cmd = 0b00000000 | R_cmd::ff00_trig;
            r.caddr = 0x0000;
            r.raddr = 0x0000; // unused bits get 'set' when reading the reg (i.e. raddr_b appears to be 0xf8 after reset)
            r.tlen = 0xffff;
            r.int_mask = 0x00;
            r.addr_ctrl = 0x00;
        }
    };
};


} // namespace Cartridge


#endif // CARTRIDGE_H_INCLUDED