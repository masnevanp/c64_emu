#ifndef REU_H_INCLUDED
#define REU_H_INCLUDED

#include <iostream>
#include "common.h"


/*
    REU support is possible only thanks to this:
        Wolfgang Moser: Technical Reference Documentation - Commodore RAM Expansion Unit Controller - 8726R1
*/

/*
    - The REC switches itself out of the host memory space during the transfers. (http://codebase64.org/doku.php?id=base:reu_registers)

*/

namespace REU {

template<typename Address_space>
class Core {
public:
    Core(Address_space& sys_address_space_) : sys_address_space(sys_address_space_) {}

    enum R : u8 {
        status = 0, cmd, saddr_l, saddr_h, raddr_l, raddr_h, raddr_b, tlen_l,
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
        xfer_sr = 0b00, xfer_rs = 0b01, xfer_swap = 0b10, xfer_very = 0b11,
    };
    enum R_raddr : u32 {
        // masks
        raddr_lo = 0b00000001111, raddr_hi = 0b00011110000,
        raddr_bank = 0b11100000000, raddr_unused = 0b1111100000000000,
    };
    enum R_int_mask : u8 {
        // masks
        int_ena = 0b10000000, eob_m = 0b01000000, ver_err_m = 0b00100000, unused_im = 0b00011111,
    };
    enum R_addr_ctrl : u8 {
        // masks
        fix_s = 0b10000000, fix_r = 0b01000000, unused_ac = 0b00111111,
    };

    bool attach(Expansion_ctx& exp_ctx) {
        // TODO: check if already atexp_ctxtached (prevent re-init & system reset)
        //       (e.g. somehow check if exp_ctx already has reu ops connected...?)
        //       (or might need to add a flag/status --> requires notification on detach)

        std::cout << "CRT: REU attach" << std::endl;

        Cartridge::detach(exp_ctx);

        pre_attach();

        exp_ctx.io.io2_r = io2_r;
        exp_ctx.io.io2_w = io2_w;

        exp_ctx.tick = Tick(exp_ctx);
        exp_ctx.reset = Reset(exp_ctx);

        return true;
    }

private:
    Address_space& sys_address_space;

    enum Ctrl_state : u8 {

    };

    struct Reg_file {
        u32 raddr;
        u16 saddr;
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

    // sys_address_space.access(0xffff, d, NMOS6502::MC::RW::w);

    Expansion_ctx::IO::r io2_r {
        [this](const u16& a, u8& d) {
            std::cout << "REU io2 r: " << (int)a << std::endl;
            switch (a & 0b11111) {
                case R::status:
                    d = r.status | R_status::sz_1764 | R_status::chip_ver_1764;
                    r.status = 0x00;
                    return;
                case R::cmd:       d = r.cmd;        return;
                case R::saddr_l:   d = r.saddr;      return;
                case R::saddr_h:   d = r.saddr >> 8; return;
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

    Expansion_ctx::IO::w io2_w {
        // NOTE: set 'exp_ctx.io.dma_low = true' upon execution
        [this](const u16& a, const u8& d) {
            std::cout << "REU io2 w: " << (int)a << std::endl;
            switch (a & 0b11111) {
                case R::status: return; // read-only
                case R::cmd:                         return;
                case R::saddr_l:   r.saddr = (r.saddr & 0xff00) | d;             return;
                case R::saddr_h:   r.saddr = (r.saddr & 0x00ff) | (d << 8);      return;
                case R::raddr_l:   r.raddr = (r.raddr & ~R_raddr::raddr_lo) | d;          return;
                case R::raddr_h:   r.raddr = (r.raddr & ~R_raddr::raddr_hi) | (d << 8);   return;
                case R::raddr_b:   r.raddr = (r.raddr & ~R_raddr::raddr_bank) | (d << 8); return;
                case R::tlen_l:    r.tlen = (r.tlen & 0xff00) | d;               return;
                case R::tlen_h:    r.tlen = (r.tlen & 0x00ff) | (d << 8);        return;
                case R::int_mask:  r.int_mask = d | R_int_mask::unused_im;       return; 
                case R::addr_ctrl: r.addr_ctrl = d | R_addr_ctrl::unused_ac;     return; 
                default: return;
            }
        }
    };

    std::function<void ()> Tick(Expansion_ctx& exp_ctx) {
        return [&]() {
            UNUSED(exp_ctx);
            // if (executing && !exp_ctx.io.ba_low) do_work();
            // NOTE: set 'exp_ctx.io.dma_low = false' upon completion
        };
    }

    std::function<void ()> Reset(Expansion_ctx& exp_ctx) {
        return [&]() {
            std::cout << "REU reset" << std::endl;

            r.status = 0x00;
            r.cmd = 0b00000000 | R_cmd::ff00_trig;
            r.saddr = 0x0000;
            r.raddr = 0x0000; // unused bits get 'set' when reading the reg (i.e. raddr_b appears to be 0xf8 after reset)
            r.tlen = 0xffff;
            r.int_mask = 0x00;
            r.addr_ctrl = 0x00;

            exp_ctx.io.dma_low = false;
        };
    }
};


} // namespace REU


#endif // REU_H_INCLUDED