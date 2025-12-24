#ifndef EXPANSION_H_INCLUDED
#define EXPANSION_H_INCLUDED

#include "common.h"
#include "state.h"
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
    // NOTE: CRT IDs are offset by 2 (generic: 0 --> 2, action replay: 1 --> 3, etc..)
    none = 0, REU = 1, generic = 2,
};


namespace CRT {

int load_static_chips(const Files::CRT& crt, u8* tgt_mem);
int load_chips(const Files::CRT& crt, u8* tgt_mem);

} // namespace CRT


enum Ticker : u16 {
    idle = 0,
    reu_sr_n, reu_sr_r, reu_sr_s, reu_sr_b,
    reu_rs_n, reu_rs_r, reu_rs_s, reu_rs_b,
    reu_sw_n, reu_sw_r, reu_sw_s, reu_sw_b,
    reu_vr_n, reu_vr_r, reu_vr_s, reu_vr_b,
    reu_wait_ff00, reu_dispatch_op,
    epyx_fl,
};


struct Base {
    State::System& s;

    Base(State::System& s_) : s(s_) {}

    // TODO: reading unconnected areas should return whatever is 'floating' on the bus
    void roml_r(const u16& a, u8& d) { UNUSED2(a, d); }
    void roml_w(const u16& a, u8& d) { UNUSED2(a, d); }
    void romh_r(const u16& a, u8& d) { UNUSED2(a, d); }
    void romh_w(const u16& a, u8& d) { UNUSED2(a, d); }

    void io1_r(const u16& a, u8& d) { UNUSED2(a, d); }
    void io1_w(const u16& a, u8& d) { UNUSED2(a, d); }
    void io2_r(const u16& a, u8& d) { UNUSED2(a, d); }
    void io2_w(const u16& a, u8& d) { UNUSED2(a, d); }

    bool attach(const Files::CRT& crt) { UNUSED(crt); return false; }
    void reset() {}

protected:
    void set_exrom_game(const Files::CRT& crt) {
        System::set_exrom_game(crt.header().exrom, crt.header().game, s);
    }

    void set_exrom_game(bool e, bool g) {
        System::set_exrom_game(e, g, s);
    }

    void set_int(IO::Int_sig::Src src) { IO::Int_sig{s.int_hub.state}.set(src); }
    void clr_int(IO::Int_sig::Src src) { IO::Int_sig{s.int_hub.state}.clr(src); }
};


struct T0 : public Base { // T0 None
    T0(State::System& s) : Base(s) {}
};


using ES = State::System::Expansion;


struct T1 : public Base { // T1: REU (512 kb)
    T1(::State::System& s) : Base(s) {}

    ES::REU& r{s.exp.state.reu};

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

    void io2_r(const u16& a, u8& d) {
        if (s.dma) return; // switched out during dma

        switch (a & 0b11111) {
            case R::status:
                d = r.status | R_status::sz_1764 | R_status::chip_ver_1764;
                if (irq_on()) clr_int(IO::Int_sig::Src::exp_i);
                r.status = 0x00;
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
    }

    void io2_w(const u16& a, const u8& d) {
        if (s.dma) return; // switched out during dma

        switch (a & 0b11111) {
            case R::status:    return; // read-only
            case R::cmd:
                r.cmd = d;
                s.exp.ticker = (r.cmd & R_cmd::exec)
                    ? (r.cmd & R_cmd::no_ff00_trig) ? Ticker::reu_dispatch_op : Ticker::reu_wait_ff00
                    : Ticker::idle;
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
    }
    
    void reset() {
        r.status = 0x00;
        r.cmd = 0b00000000 | R_cmd::no_ff00_trig;
        r.a.saddr = 0x0000;
        r.a.raddr = 0x000000; // unused bits get 'set' when reading the reg (i.e. raddr_b appears to be 0xf8 after reset)
        r.a.tlen = 0xffff;
        r._a = r.a;
        r.int_mask = 0x00 | R_int_mask::unused_im;
        r.addr_ctrl = 0x00 | R_addr_ctrl::unused_ac;;

        r.swap_cycle = false;
    }

protected:
    bool irq_on() const { return r.status & R_status::int_pend; }

    void check_irq() {
        if (r.int_mask & R_int_mask::int_ena) {
            if ((r.int_mask & (R_int_mask::eob_m | R_int_mask::ver_err_m)) &
                (r.status & (R_status::eob | R_status::ver_err))) {
                r.status |= R_status::int_pend;
                set_int(IO::Int_sig::Src::exp_i);
            }
        }
    }
};

template<typename Bus>
struct T1_kludge : public T1 { // REU kludge
    T1_kludge(State::System& s, Bus& bus_) : T1(s), bus(bus_) {}

    // 4 operations: sys -> REU, REU -> sys, swap, verify
    // 4 addr. modes for each op.: fix none, fix reu addr, fix sys addr, fix both

    // sys -> REU
    void tick_sr_n() { if (bus_available()) { do_sr(); inc_raddr(); inc_saddr(); do_tlen(); } }
    void tick_sr_r() { if (bus_available()) { do_sr(); inc_saddr(); do_tlen(); } }
    void tick_sr_s() { if (bus_available()) { do_sr(); inc_raddr(); do_tlen(); } }
    void tick_sr_b() { if (bus_available()) { do_sr(); do_tlen(); } }
    // REU -> sys
    void tick_rs_n() { if (bus_available()) { do_rs(); inc_raddr(); inc_saddr(); do_tlen(); } }
    void tick_rs_r() { if (bus_available()) { do_rs(); inc_saddr(); do_tlen(); } }
    void tick_rs_s() { if (bus_available()) { do_rs(); inc_raddr(); do_tlen(); } }
    void tick_rs_b() { if (bus_available()) { do_rs(); do_tlen(); } }
    // swap
    void tick_sw_n() { if (do_swap()) { inc_raddr(); inc_saddr();  do_tlen(); } }
    void tick_sw_r() { if (do_swap()) { inc_saddr(); do_tlen(); } }
    void tick_sw_s() { if (do_swap()) { inc_raddr(); do_tlen(); } }
    void tick_sw_b() { if (do_swap()) { do_tlen(); } }
    // verify
    void tick_vr_n() { if (bus_available()) { do_ver(); inc_raddr(); inc_saddr(); } }
    void tick_vr_r() { if (bus_available()) { do_ver(); inc_saddr(); } }
    void tick_vr_s() { if (bus_available()) { do_ver(); inc_raddr(); } }
    void tick_vr_b() { if (bus_available()) { do_ver(); } }

    void tick_wait_ff00() {
        const bool ff00_written = (
            (s.bus.addr == 0xff00) && (s.bus.rw == State::System::Bus::RW::w)
        );
        if (ff00_written) tick_dispatch_op();
    }

    void tick_dispatch_op() {
        // resolve ticker from cmd & addr_ctrl regs
        const auto t = (((r.cmd & R_cmd::xfer) << 2) | ((r.addr_ctrl & R_addr_ctrl::fix) >> 6)) + 1;
        s.exp.ticker = t;
        s.dma = true;
    }

private:
    Bus& bus;

    bool bus_available() const { return !s.ba; }

    void inc_raddr() { r.a.raddr = ((r.a.raddr + 1) & ~R_raddr::raddr_unused); }
    void inc_saddr() { r.a.saddr += 1; }

    void do_tlen() { if (r.a.tlen == 1) done(); else r.a.tlen -= 1; }

    void done() {
        s.exp.ticker = Expansion::Ticker::idle;
        s.dma = false;
        if (r.a.tlen == 1) r.status |= R_status::eob;
        r.cmd = r.cmd & ~R_cmd::exec;
        r.cmd = r.cmd | R_cmd::no_ff00_trig;
        if (r.cmd & R_cmd::autoload) r.a = r._a;
        check_irq();
    }

    void do_sr() { bus.access(r.a.saddr, r.mem[r.a.raddr], State::System::Bus::RW::r); }
    void do_rs() { bus.access(r.a.saddr, r.mem[r.a.raddr], State::System::Bus::RW::w); }

    bool do_swap() {
        if (bus_available()) {
            r.swap_cycle = !r.swap_cycle;

            if (!r.swap_cycle) {
                r.mem[r.a.raddr] = r.swap_r;
                bus.access(r.a.saddr, r.swap_s, State::System::Bus::RW::w);
                return true;
            } else {
                r.swap_s = r.mem[r.a.raddr];
                bus.access(r.a.saddr, r.swap_r, State::System::Bus::RW::r);
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
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
        u8 ds;
        #pragma GCC diagnostic push

        bus.access(r.a.saddr, ds, State::System::Bus::RW::r);
        const u8 dr = r.mem[r.a.raddr];

        do_tlen();

        if (ds != dr) {
            r.status |= R_status::ver_err;
            done(); // might be an extra 'done()' (if tlen was 1), but meh...
        }
    }
};


struct T2 : public Base { // T2 Generic
    T2(State::System& s) : Base(s) {}

    ES::Generic& g{s.exp.state.generic};

    void roml_r(const u16& a, u8& d) { d = g.mem[a]; }
    void romh_r(const u16& a, u8& d) { d = g.mem[a]; }

    bool attach(const Files::CRT& crt) {
        if (CRT::load_static_chips(crt, g.mem) == 0) return false;
        set_exrom_game(crt);
        return true;
    }
};


struct T6 : public T2 { // T6 Simons' Basic
    T6(State::System& s) : T2(s) {}

    void io1_r(const u16& a, u8& d) { UNUSED2(a, d); set_8k(); }
    void io1_w(const u16& a, u8& d) { UNUSED2(a, d); set_16k(); }

    void reset() { set_8k(); }

private:
    void set_8k()  { set_exrom_game(false, true); }
    void set_16k() { set_exrom_game(false, false); }
};


struct T12 : public Base { // T12 Epyx Fastload
    T12(State::System& s) : Base(s) {}

    ES::Epyx_Fastload& efl{s.exp.state.epyx_fl};

    void roml_r(const u16& a, u8& d) { act(); d = efl.mem[a]; }
    void io1_r(const u16& a, u8& d)  { UNUSED2(a, d); act(); }
    void io2_r(const u16& a, u8& d)  { d = efl.mem[0x9f00 | (a & 0x00ff)]; }

    bool attach(const Files::CRT& crt) {
        if (CRT::load_static_chips(crt, efl.mem) == 0) return false;
        deact();
        return true;
    }

    void reset() { act(); }

    void tick() { if (s.vic.cycle == efl.deact_cycle) deact(); }

private:
    void act()   {
        if (not_act()) {
            set_exrom_game(false, true);
            s.exp.ticker = Ticker::epyx_fl;
        }
        efl.deact_cycle = s.vic.cycle + 512;
    }

    void deact() {
        set_exrom_game(true, true);
        s.exp.ticker = Ticker::idle;
    }

    bool not_act() const { return s.exp.ticker != Ticker::epyx_fl; }
};


void detach(State::System& s);
bool attach(State::System& s, const Files::CRT& crt);
bool attach_REU(State::System& s);
void reset(State::System& s);


template<typename Bus>
void tick(State::System& s, Bus& bus) {
    //#define T(t) case t: T##t##_kludge<Bus>{s, bus}.tick(); break;
    using REU = T1_kludge<Bus>;

    switch (s.exp.ticker) {
        case Ticker::idle: return;
        case Ticker::reu_sr_n: REU{s, bus}.tick_sr_n(); break;
        case Ticker::reu_sr_r: REU{s, bus}.tick_sr_r(); break;
        case Ticker::reu_sr_s: REU{s, bus}.tick_sr_s(); break;
        case Ticker::reu_sr_b: REU{s, bus}.tick_sr_b(); break;
        case Ticker::reu_rs_n: REU{s, bus}.tick_rs_n(); break;
        case Ticker::reu_rs_r: REU{s, bus}.tick_rs_r(); break;
        case Ticker::reu_rs_s: REU{s, bus}.tick_rs_s(); break;
        case Ticker::reu_rs_b: REU{s, bus}.tick_rs_b(); break;
        case Ticker::reu_sw_n: REU{s, bus}.tick_sw_n(); break;
        case Ticker::reu_sw_r: REU{s, bus}.tick_sw_r(); break;
        case Ticker::reu_sw_s: REU{s, bus}.tick_sw_s(); break;
        case Ticker::reu_sw_b: REU{s, bus}.tick_sw_b(); break;
        case Ticker::reu_vr_n: REU{s, bus}.tick_vr_n(); break;
        case Ticker::reu_vr_r: REU{s, bus}.tick_vr_r(); break;
        case Ticker::reu_vr_s: REU{s, bus}.tick_vr_s(); break;
        case Ticker::reu_vr_b: REU{s, bus}.tick_vr_b(); break;
        case Ticker::reu_wait_ff00:   REU{s, bus}.tick_wait_ff00();   break;
        case Ticker::reu_dispatch_op: REU{s, bus}.tick_dispatch_op(); break;
        case Ticker::epyx_fl: T12{s}.tick(); break;
    }
}


enum Bus_op : u8 {
    roml_r = 0,  roml_w = 1,
    romh_r = 2,  romh_w = 3,
    io1_r  = 4,  io2_r  = 5,
    io1_w  = 6,  io2_w  = 7,

    _cnt = 8
};


inline void bus_op(State::System& s, Bus_op op, const u16& a, u8& d) {
    #define T(t) case (t * Bus_op::_cnt) + Bus_op::roml_r: T##t{s}.roml_r(a, d); return; \
                 case (t * Bus_op::_cnt) + Bus_op::roml_w: T##t{s}.roml_w(a, d); return; \
                 case (t * Bus_op::_cnt) + Bus_op::romh_r: T##t{s}.romh_r(a, d); return; \
                 case (t * Bus_op::_cnt) + Bus_op::romh_w: T##t{s}.romh_w(a, d); return; \
                 case (t * Bus_op::_cnt) + Bus_op::io1_r: T##t{s}.io1_r(a, d); return; \
                 case (t * Bus_op::_cnt) + Bus_op::io2_r: T##t{s}.io2_r(a, d); return; \
                 case (t * Bus_op::_cnt) + Bus_op::io1_w: T##t{s}.io1_w(a, d); return; \
                 case (t * Bus_op::_cnt) + Bus_op::io2_w: T##t{s}.io2_w(a, d); return; \

    switch ((s.exp.type * Bus_op::_cnt) + op) {
        T(0) T(1) T(2) T(6) T(12)
    }

    #undef T
}

} // namespace Expansion


#endif // EXPANSION_H_INCLUDED
