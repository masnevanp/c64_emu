
#include <iostream>
#include <optional>
#include "expansion.h"
#include "utils.h"
#include "nmos6502/nmos6502_mc.h"



/*
  TODO:
    - handle .CRTs with banks out-of-order (although .CRTs seem to have the chips always
      in order)
*/
using Ctx = Expansion_ctx;


// for the basic 'no fancy dynamic banking' carts
int load_static_chips(const Files::CRT& crt, Ctx& ctx) {
    int count = 0;

    for (auto cp : crt.chip_packets()) {
        u32 tgt_addr;

        if (cp->load_addr == 0x8000) tgt_addr = 0x0000;
        else if (cp->load_addr == 0xa000 || cp->load_addr == 0xe000) tgt_addr = 0x2000;
        else {
            std::cout << "CRT: invalid CHIP packet address - " << (int)cp->load_addr << std::endl;
            continue;
        }

        std::copy(cp->data(), cp->data() + cp->data_size, &ctx.ram[tgt_addr]);

        ++count;
    }

    return count;
}

// just load all the chips in the order found (TODO: ever need to check the order?)
int load_chips(const Files::CRT& crt, Ctx& ctx) {
    int count = 0;

    u8* tgt = ctx.ram;
    for (auto cp : crt.chip_packets()) {
        std::copy(cp->data(), cp->data() + cp->data_size, tgt);
        tgt += cp->data_size;
        ++count;
    }

    return count;
}


struct exrom_game {
    bool exrom; bool game;
    exrom_game(bool e, bool g) : exrom(e), game(g) {}
    exrom_game(const Files::CRT& crt)
        : exrom(bool(crt.header().exrom)), game(bool(crt.header().game)) {} 
};

using Result = std::optional<exrom_game>;


static const auto& not_attached = std::nullopt;


Result T0_Normal_cartridge(const Files::CRT& crt, Ctx& ctx) {
    if (load_static_chips(crt, ctx) == 0) return not_attached;

    ctx.io.roml_r = [&](const u16& a, u8& d) { d = ctx.ram[a]; };
    ctx.io.romh_r = [&](const u16& a, u8& d) { d = ctx.ram[0x2000 + a]; };

    return exrom_game(crt);
}


Result T4_Simons_BASIC(const Files::CRT& crt, Ctx& ctx) {
    if (load_static_chips(crt, ctx) != 2) return not_attached;

    struct romh_active {
        void operator=(bool act) { _c.io.exrom_game(false, !act); }
        Ctx& _c;
    };

    ctx.io.roml_r = [&](const u16& a, u8& d) {
        d = ctx.ram[a];
    };
    ctx.io.romh_r = [&](const u16& a, u8& d) {
        d = ctx.ram[0x2000 + a];
    };

    ctx.io.io1_r = [&](const u16& a, u8& d) { UNUSED(a); UNUSED(d);
        romh_active{ctx} = false;
    };
    ctx.io.io1_w = [&](const u16& a, const u8& d) { UNUSED(a); UNUSED(d);
        romh_active{ctx} = true;
    };

    ctx.reset = [&]() { romh_active{ctx} = false; };

    return exrom_game(crt);
}


Result T5_Ocean_type_1(const Files::CRT& crt, Ctx& ctx) {
    u32 exp_mem_addr = 0x0001; // current bank stored at 0x0000
    int banks = 0;

    for (auto cp : crt.chip_packets()) {
        auto la = cp->load_addr;
        if ((la != 0x8000 && la != 0xa000) || cp->data_size != 0x2000) {
            std::cout << "CRT: invalid CHIP packet" << std::endl;
            return not_attached;
        }

        std::copy(cp->data(), cp->data() + cp->data_size, &ctx.ram[exp_mem_addr]);

        exp_mem_addr += 0x2000;
        ++banks;
    }

    if (banks == 0) return not_attached;

    struct bank {
        void operator=(u8 b) { _c.ram[0x0000] = b & 0b00111111; }
        operator u32() const { return (_c.ram[0x0000] * 0x2000) + 1; }
        Ctx& _c;
    };

    const bool game = (banks == 64);

    if (!game) {
        ctx.io.romh_r = [&](const u16& a, u8& d) { d = ctx.ram[bank{ctx} + a]; };
    }

    ctx.io.roml_r = [&](const u16& a, u8& d) { d = ctx.ram[bank{ctx} + a]; };

    ctx.io.io1_w = [&](const u16& a, const u8& d) {
        if (a == 0x0e00) bank{ctx} = d & 0b00111111;
    };

    ctx.reset = [&]() { bank{ctx} = 0; };

    return exrom_game(false, game);
}


Result T10_Epyx_Fastload(const Files::CRT& crt, Ctx& ctx) {
    if (load_static_chips(crt, ctx) != 1) return not_attached;

    struct status {
        bool inactive() const { return _c.sys_cycle >= _inact_at(); }
        void activate()       { _inact_at() = _c.sys_cycle + 512; }

        Ctx& _c;
        u64& _inact_at() const { return *(u64*)&_c.ram[0x2000]; }
    };

    ctx.io.roml_r = [&](const u16& a, u8& d) {
        if (status{ctx}.inactive()) {
            d = ctx.sys_ram[0x8000 + a];
        } else {
            d = ctx.ram[a];
            status{ctx}.activate();
        }
    };

    ctx.io.io1_r = [&](const u16& a, u8& d) { UNUSED(a); UNUSED(d);
        status{ctx}.activate();
    };
    ctx.io.io2_r = [&](const u16& a, u8& d) {
        d = ctx.ram[0x1f00 | (a & 0x00ff)];
    };

    ctx.reset = [&]() { status{ctx}.activate(); };

    return exrom_game(false, bool(crt.header().game));
}


Result T16_Warp_Speed(const Files::CRT& crt, Ctx& ctx) {
    if (load_static_chips(crt, ctx) != 1) return not_attached; // TODO: verify the size (0x4000)

    struct active {
        operator bool() const { return _c.ram[0x4000]; }
        void operator=(bool act) {
            _c.ram[0x4000] = act;
            _c.io.exrom_game(!act, !act);
        }

        Ctx& _c;
    };

    ctx.io.roml_r = [&](const u16& a, u8& d) {
        d = active{ctx}
                ? ctx.ram[0x0000 + a]
                : ctx.sys_ram[0x8000 + a];
    };
    ctx.io.romh_r = [&](const u16& a, u8& d) {
        d = active{ctx}
                ? ctx.ram[0x2000 + a]
                : ctx.sys_ram[0xa000 + a];
    };

    ctx.io.io1_r = [&](const u16& a, u8& d) {
        d = ctx.ram[0x1e00 | (a & 0x00ff)];
    };
    ctx.io.io2_r = [&](const u16& a, u8& d) {
        d = ctx.ram[0x1f00 | (a & 0x00ff)];
    };
    ctx.io.io1_w = [&](const u16& a, const u8& d) { UNUSED(a); UNUSED(d);
        active{ctx} = true;
    };
    ctx.io.io2_w = [&](const u16& a, const u8& d) { UNUSED(a); UNUSED(d);
        active{ctx} = false;
    };

    ctx.reset = [&]() { active{ctx} = true; };

    return exrom_game(false, false);
}


Result T18_Zaxxon_Super_Zaxxon_SEGA(const Files::CRT& crt, Ctx& ctx) {
    if (load_chips(crt, ctx) != 3) return not_attached;

    struct romh_base {
        void operator=(u16 b) { _base() = b; }
        operator u16() const  { return _base(); }
        u16& _base() const    { return *(u16*)&_c.ram[0x5000]; }
        Ctx& _c;
    };

    ctx.io.roml_r = [&](const u16& a, u8& d) {
        romh_base{ctx} = 0x1000 | ((a & 0x1000) << 1);
        d = ctx.ram[a & 0x0fff];
    };
    ctx.io.romh_r = [&](const u16& a, u8& d) {
        d = ctx.ram[romh_base{ctx} + a];
    };

    return exrom_game(crt);
}


Result T51_MACH_5(const Files::CRT& crt, Ctx& ctx) {
    if (load_static_chips(crt, ctx) != 1) return not_attached;

    struct active {
        void operator=(bool act) { _c.io.exrom_game(!act, true); }
        Ctx& _c;
    };

    ctx.io.roml_r = [&](const u16& a, u8& d) { d = ctx.ram[a]; };

    ctx.io.io1_r = [&](const u16& a, u8& d) { d = ctx.ram[0x1e00 | (a & 0x00ff)]; };
    ctx.io.io2_r = [&](const u16& a, u8& d) { d = ctx.ram[0x1f00 | (a & 0x00ff)]; };
    ctx.io.io1_w = [&](const u16& a, const u8& d) { UNUSED(a); UNUSED(d);
        active{ctx} = true;
    };
    ctx.io.io2_w = [&](const u16& a, const u8& d) { UNUSED(a); UNUSED(d);
        active{ctx} = false;
    };

    ctx.reset = [&]() { active{ctx} = true; };

    return exrom_game(crt);
}


bool Cartridge::attach(const Files::CRT& crt, Ctx& exp_ctx) {
    detach(exp_ctx);

    Result result;
    switch (const auto type = crt.header().hw_type; type) {
        using T = Files::CRT::Cartridge_HW_type;

        case T::T0_Normal_cartridge:
            result = T0_Normal_cartridge(crt, exp_ctx);
            break;
        case T::T4_Simons_BASIC:
            result = T4_Simons_BASIC(crt, exp_ctx);
            break;
        case T::T5_Ocean_type_1:
            result = T5_Ocean_type_1(crt, exp_ctx);
            break;
        case T::T10_Epyx_Fastload:
            result = T10_Epyx_Fastload(crt, exp_ctx);
            break;
        case T::T16_Warp_Speed:
            result = T16_Warp_Speed(crt, exp_ctx);
            break;
        case T::T18_Zaxxon_Super_Zaxxon_SEGA:
            result = T18_Zaxxon_Super_Zaxxon_SEGA(crt, exp_ctx);
            break;
        case T::T51_MACH_5:
            result = T51_MACH_5(crt, exp_ctx);
            break;
        default:
            std::cout << "CRT: unsupported HW type: " << (int)type << std::endl;
            break;
    }

    if (result) {
        exp_ctx.io.exrom_game((*result).exrom, (*result).game);
        return true;
    }

    return false;
}


void Cartridge::detach(Ctx& exp_ctx) {
    // reading unconnected areas should return whatever is 'floating'
    // on the bus, so this is not 100% accurate, but meh....
    static Expansion_ctx::IO::r null_r { [](const u16& a, u8& d) { UNUSED(a); UNUSED(d); } };
    static Expansion_ctx::IO::w null_w { [](const u16& a, const u8& d) { UNUSED(a); UNUSED(d); } };

    exp_ctx.io.roml_r = null_r;
    exp_ctx.io.roml_w = null_w;
    exp_ctx.io.romh_r = null_r;
    exp_ctx.io.romh_w = null_w;

    exp_ctx.io.io1_r = null_r;
    exp_ctx.io.io1_w = null_w;
    exp_ctx.io.io2_r = null_r;
    exp_ctx.io.io2_w = null_w;

    exp_ctx.io.exrom_game(true, true);

    exp_ctx.io.dma_low = false;

    exp_ctx.tick = nullptr;
    exp_ctx.reset = [](){};
}


class REU {
public:
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
        exec = 0b10000000, autoload = 0b00100000, no_ff00_trig = 0b00010000, xfer = 0b00000011,
        reserved = 0b01001100,
        // values
        xfer_sr = 0b00, xfer_rs = 0b01, xfer_swap = 0b10, xfer_very = 0b11,
    };
    enum R_raddr : u32 {
        // masks
        raddr_lo     = 0b000000000000000011111111,
        raddr_hi     = 0b000000001111111100000000,
        raddr_bank   = 0b000001110000000000000000,
        raddr_unused = 0b111110000000000000000000,
    };
    enum R_int_mask : u8 {
        // masks
        int_ena = 0b10000000, eob_m = 0b01000000, ver_err_m = 0b00100000, unused_im = 0b00011111,
    };
    enum R_addr_ctrl : u8 {
        // masks
        fix_s = 0b10000000, fix_r = 0b01000000, fix = 0b11000000, unused_ac = 0b00111111,
    };

    REU(Ctx& exp_ctx_) : exp_ctx(exp_ctx_) {}

    bool attach() {
        exp_ctx.io.io2_r = [&](const u16& a, u8& d) {
            // std::cout << "r: " << (int)(a & 0xf) << ' ';
            if (exp_ctx.io.dma_low) return; // switched out during dma

            switch (a & 0b11111) {
                case R::status:
                    d = r.status | R_status::sz_1764 | R_status::chip_ver_1764;
                    if (irq_on()) clr_irq();
                    r.status = 0x00;
                    //std::cout << "(S: " << (int)(d) << ')';
                    return;
                case R::cmd:       d = r.cmd;          return;
                case R::saddr_l:   d = r.a.saddr;      return;
                case R::saddr_h:   d = r.a.saddr >> 8; return;
                case R::raddr_l:   d = r.a.raddr;      return;
                case R::raddr_h:   d = r.a.raddr >> 8; return;
                case R::raddr_b:   d = (r.a.raddr | R_raddr::raddr_unused) >> 16; return;
                case R::tlen_l:    d = r.a.tlen;       return;
                case R::tlen_h:    d = r.a.tlen >> 8;  return;
                case R::int_mask:  d = r.int_mask;     return;
                case R::addr_ctrl: d = r.addr_ctrl;    return;
                default:           d = 0xff;           return;
            }
        };

        exp_ctx.io.io2_w = [&](const u16& a, const u8& d) {
            // std::cout << "w: " << (int)(a & 0xf) << ' ' << (int)d << ' ';
            if (exp_ctx.io.dma_low) return; // switched out during dma

            switch (a & 0b11111) {
                case R::status:    return; // read-only
                case R::cmd:
                    r.cmd = d;
                    exp_ctx.tick = (r.cmd & R_cmd::exec)
                        ? (r.cmd & R_cmd::no_ff00_trig) ? tick_dispatch_op : tick_wait_ff00
                        : nullptr;
                    return;
                case R::saddr_l:   r.a.saddr = r._a.saddr = (r._a.saddr & 0xff00) | d;        return;
                case R::saddr_h:   r.a.saddr = r._a.saddr = (r._a.saddr & 0x00ff) | (d << 8); return;
                case R::raddr_l:   r.a.raddr = r._a.raddr = ((r._a.raddr & ~R_raddr::raddr_lo) | d);        return;
                case R::raddr_h:   r.a.raddr = r._a.raddr = ((r._a.raddr & ~R_raddr::raddr_hi) | (d << 8)); return;
                case R::raddr_b:   r.a.raddr = r._a.raddr = ((r.a.raddr & ~R_raddr::raddr_bank)
                                                                | ((d << 16) & R_raddr::raddr_bank)); return;
                case R::tlen_l:    r.a.tlen = r._a.tlen = (r._a.tlen & 0xff00) | d;        return;
                case R::tlen_h:    r.a.tlen = r._a.tlen = (r._a.tlen & 0x00ff) | (d << 8); return;
                case R::int_mask:  r.int_mask = d | R_int_mask::unused_im; check_irq(); return;
                case R::addr_ctrl: r.addr_ctrl = d | R_addr_ctrl::unused_ac; return;
                default: return;
            }
        };

        exp_ctx.reset = [&]() {
            r.status = 0x00;
            r.cmd = 0b00000000 | R_cmd::no_ff00_trig;
            r.a.saddr = 0x0000;
            r.a.raddr = 0x000000; // unused bits get 'set' when reading the reg (i.e. raddr_b appears to be 0xf8 after reset)
            r.a.tlen = 0xffff;
            r._a = r.a;
            r.int_mask = 0x00 | R_int_mask::unused_im;
            r.addr_ctrl = 0x00 | R_addr_ctrl::unused_ac;;

            exp_ctx.io.dma_low = false;

            swap_cycle = false;
        };

        return true;
    }
private:
    struct Reg_file {
        struct Addr {
            u32 raddr;
            u16 saddr;
            u16 tlen;

            // TODO: handle addr beyond actual RAM (past 256k)
            void inc_raddr() { raddr = ((raddr + 1) & ~R_raddr::raddr_unused); }
            void inc_saddr() { saddr += 1; }
        };
        Addr a;
        Addr _a;
        u8 status;
        u8 cmd;
        u8 int_mask;
        u8 addr_ctrl;
    };

    Reg_file r;

    u8 swap_r;
    u8 swap_s;
    u8 swap_cycle;

    Ctx& exp_ctx;

    bool ba() const { return !exp_ctx.io.ba_low; }

    bool irq_on() const { return r.status & R_status::int_pend; }

    void set_irq() { exp_ctx.io.int_sig.set(::IO::Int_sig::Src::exp_i); }
    void clr_irq() { exp_ctx.io.int_sig.clr(::IO::Int_sig::Src::exp_i); }

    void do_sr() { exp_ctx.io.sys_addr_space(r.a.saddr, exp_ctx.ram[r.a.raddr], NMOS6502::MC::RW::r); }
    void do_rs() { exp_ctx.io.sys_addr_space(r.a.saddr, exp_ctx.ram[r.a.raddr], NMOS6502::MC::RW::w); }

    void do_tlen() { if (r.a.tlen == 1) done(); else r.a.tlen -= 1; }

    bool do_swap() {
        if (ba()) {
            swap_cycle = !swap_cycle;

            if (!swap_cycle) {
                exp_ctx.ram[r.a.raddr] = swap_r;
                exp_ctx.io.sys_addr_space(r.a.saddr, swap_s, NMOS6502::MC::RW::w);
                return true;
            } else {
                swap_s = exp_ctx.ram[r.a.raddr];
                exp_ctx.io.sys_addr_space(r.a.saddr, swap_r, NMOS6502::MC::RW::r);
            }
        }

        return false;
    }

    /* TOFIX (very low prio.... comments copied from 'vice/src/c64/cart/reu.c'):
            * failed verify operations consume one extra cycle, except if
            * the failed comparison happened on the last byte of the buffer.

            * If the last byte failed, the "end of block transfer" bit is set, too

            * If the next-to-last byte failed, the "end of block transfer" bit is
            * set, but only if the last byte compares equal
    */
    void do_ver() {
        u8 ds;
        exp_ctx.io.sys_addr_space(r.a.saddr, ds, NMOS6502::MC::RW::r);
        const u8 dr = exp_ctx.ram[r.a.raddr];

        do_tlen();

        if (ds != dr) {
            //std::cout << "\nVERR: raddr: " << (int)(r.a.raddr) << ", saddr: " << (int)(r.a.saddr)
            //            << ", tlen: " << (int)(r.a.tlen);
            r.status |= R_status::ver_err;
            done(); // might be an extra 'done()' (if tlen was 1), but meh...
        }
    }

    void check_irq() {
        if (r.int_mask & R_int_mask::int_ena) {
            if ((r.int_mask & (R_int_mask::eob_m | R_int_mask::ver_err_m)) &
                (r.status & (R_status::eob | R_status::ver_err))) {
                r.status |= R_status::int_pend;
                set_irq();
            }
        }
    }

    std::function<void ()> Tick[16] {
        // 4 addr. types for each: fix none, fix reu, fix sys, fix both
        // sys -> REU
        [this] { if (ba()) { do_sr(); r.a.inc_raddr(); r.a.inc_saddr(); do_tlen(); } },
        [this] { if (ba()) { do_sr(); r.a.inc_saddr(); do_tlen(); } },
        [this] { if (ba()) { do_sr(); r.a.inc_raddr(); do_tlen(); } },
        [this] { if (ba()) { do_sr(); do_tlen(); } },
        // REU -> sys
        [this] { if (ba()) { do_rs(); r.a.inc_raddr(); r.a.inc_saddr(); do_tlen(); } },
        [this] { if (ba()) { do_rs(); r.a.inc_saddr(); do_tlen(); } },
        [this] { if (ba()) { do_rs(); r.a.inc_raddr(); do_tlen(); } },
        [this] { if (ba()) { do_rs(); do_tlen(); } },
        // swap
        [this] { if (do_swap()) { r.a.inc_raddr(); r.a.inc_saddr();  do_tlen(); } },
        [this] { if (do_swap()) { r.a.inc_saddr(); do_tlen(); } },
        [this] { if (do_swap()) { r.a.inc_raddr(); do_tlen(); } },
        [this] { if (do_swap()) { do_tlen(); } },
        // verify
        [this] { if (ba()) { do_ver(); r.a.inc_raddr(); r.a.inc_saddr(); } },
        [this] { if (ba()) { do_ver(); r.a.inc_saddr(); } },
        [this] { if (ba()) { do_ver(); r.a.inc_raddr(); } },
        [this] { if (ba()) { do_ver(); } },
    };

    std::function<void ()> tick_wait_ff00 {
        [this] {
            const bool ff00_written = (
                (exp_ctx.sys_bus.addr() == 0xff00) && (exp_ctx.sys_bus.rw() == NMOS6502::MC::RW::w)
            );
            if (ff00_written) tick_dispatch_op();
        }
    };

    std::function<void ()> tick_dispatch_op {
        [this] {
            const auto op = ((r.cmd & R_cmd::xfer) << 2) | ((r.addr_ctrl & R_addr_ctrl::fix) >> 6);
            exp_ctx.tick = Tick[op];
            exp_ctx.io.dma_low = true;
            //std::cout << "\n\nOP: raddr: " << (int)(r.a.raddr) << ", saddr: " << (int)(r.a.saddr)
            //            << ", tlen: " << (int)(r.a.tlen) << ", cmd: " << (int)(r.cmd)
            //            << ", op: " << op;
        }
    };

    void done() {
        exp_ctx.tick = nullptr;
        exp_ctx.io.dma_low = false;
        if (r.a.tlen == 1) r.status |= R_status::eob;
        r.cmd = r.cmd & ~R_cmd::exec;
        r.cmd = r.cmd | R_cmd::no_ff00_trig;
        if (r.cmd & R_cmd::autoload) r.a = r._a;
        //std::cout << "\nOP: done, sr: " << (int)(r.status);
        //std::cout << "    raddr: " << (int)(r.a.raddr) << ", saddr: " << (int)(r.a.saddr)
        //    << ", tlen: " << (int)(r.a.tlen) << ", cmd: " << (int)(r.cmd) << std::endl;
        check_irq();
    }
};


bool Cartridge::attach_REU(Ctx& exp_ctx) {
    // TODO: check if already attached (prevent re-init & system reset)
    //       (e.g. somehow check if exp_ctx already has reu ops connected...?)
    //       (or might need to add a flag/status --> requires notification on detach)
    Cartridge::detach(exp_ctx);

    REU& reu = *(new REU(exp_ctx)); // TODO: allocate in expansion ram (placement new)
    return reu.attach();
}
