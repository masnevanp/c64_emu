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


namespace CRT {

int load_static_chips(const Files::CRT& crt, u8* exp_ram);
int load_chips(const Files::CRT& crt, u8* exp_ram);

} // namespace CRT


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

    void set_int(IO::Int_sig::Src src) { IO::Int_sig{s.int_hub.state}.set(src); }
    void clr_int(IO::Int_sig::Src src) { IO::Int_sig{s.int_hub.state}.clr(src); }
};


struct T0 : public Base { // T0 Generic
    T0(State::System& s) : Base(s) {}

    void roml_r(const u16& a, u8& d) { d = s.exp_ram[a]; }
    void romh_r(const u16& a, u8& d) { d = s.exp_ram[0x2000 + a]; }

    bool attach(const Files::CRT& crt) {
        if (CRT::load_static_chips(crt, s.exp_ram) == 0) return false;
        set_exrom_game(crt);
        return true;
    }
};


struct T10 : public Base { // T10_Epyx_Fastload
    T10(State::System& s) : Base(s) {}

    struct status {
        bool inactive() const { return s.vic.cycle >= _inact_at(); }
        void activate()       { _inact_at() = s.vic.cycle + 512; }

        State::System& s;
        u64& _inact_at() const { return *(u64*)&s.exp_ram[0x2000]; }
    };

    void roml_r(const u16& a, u8& d) {
        if (status{s}.inactive()) {
            d = s.ram[0x8000 + a];
        } else {
            d = s.exp_ram[a];
            status{s}.activate();
        }
    }

    void io1_r(const u16& a, u8& d) { UNUSED2(a, d);
        status{s}.activate();
    };

    void io2_r(const u16& a, u8& d) { UNUSED2(a, d);
        d = s.exp_ram[0x1f00 | (a & 0x00ff)];
    };

    bool attach(const Files::CRT& crt) {
        if (CRT::load_static_chips(crt, s.exp_ram) != 1) return false;
        System::set_exrom_game(false, crt.header().game, s);
        return true;
    }

    void reset() { status{s}.activate(); }
};


struct T65534 : public Base { // REU 1764
    static constexpr u32 ram_size = 256 * 1024;

    struct REU_state {
        // register data
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

        // control data
        u8 swap_r;
        u8 swap_s;
        u8 swap_cycle;
    };

    // First 256k of ss.exp_ram acts as REU-ram. The 'State' follows REU-ram.
    // TODO: MUST prevent access beyond 256k (it will mess up the state)
    T65534(::State::System& s)
        : Base(s),
          r(*((REU_state*)(s.exp_ram + ram_size))) { }

    REU_state& r;

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
        if (s.dma_low) return; // switched out during dma

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
        if (s.dma_low) return; // switched out during dma

        switch (a & 0b11111) {
            case R::status:    return; // read-only
            case R::cmd:
                r.cmd = d;
                /*exp_ctx.tick = (r.cmd & R_cmd::exec)
                    ? (r.cmd & R_cmd::no_ff00_trig) ? tick_dispatch_op : tick_wait_ff00
                    : nullptr;*/
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
struct T65534_kludge : public T65534 { // REU
    T65534_kludge(State::System& s, Bus& bus_) : T65534(s), bus(bus_) {}

    void tick() {
        u8 i = 1;
        bus.access(53280, i, NMOS6502::MC::RW::w);
    }

private:
    Bus& bus;

    bool ba() const { return !s.ba_low; }

    void do_sr() { bus.access(r.a.saddr, s.exp_ram[r.a.raddr], State::System::Bus::RW::r); }
    void do_rs() { bus.access(r.a.saddr, s.exp_ram[r.a.raddr], State::System::Bus::RW::w); }

    void do_tlen() { if (r.a.tlen == 1) done(); else r.a.tlen -= 1; }

    bool do_swap() {
        if (ba()) {
            r.swap_cycle = !r.swap_cycle;

            if (!r.swap_cycle) {
                s.exp_ram[r.a.raddr] = r.swap_r;
                bus.access(r.a.saddr, r.swap_s, State::System::Bus::RW::w);
                return true;
            } else {
                r.swap_s = s.exp_ram[r.a.raddr];
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
        u8 ds;
        bus.access(r.a.saddr, ds, State::System::Bus::RW::r);
        const u8 dr = s.exp_ram[r.a.raddr];

        do_tlen();

        if (ds != dr) {
            r.status |= R_status::ver_err;
            done(); // might be an extra 'done()' (if tlen was 1), but meh...
        }
    }

    void done() {
        //exp_ctx.tick = nullptr;
        s.dma_low = false;
        if (r.a.tlen == 1) r.status |= R_status::eob;
        r.cmd = r.cmd & ~R_cmd::exec;
        r.cmd = r.cmd | R_cmd::no_ff00_trig;
        if (r.cmd & R_cmd::autoload) r.a = r._a;
        check_irq();
    }
};


bool attach(State::System& s, const Files::CRT& crt);

inline bool attach_REU(State::System& s) {
    s.expansion_type = Type::REU;
    System::set_exrom_game(true, true, s);
    return true;
}

void detach(State::System& s);

void reset(State::System& s);


template<typename Bus>
void tick(State::System& s, Bus& bus) {
    switch (s.expansion_type) {
        #define T(t) case t: T##t##_kludge<Bus>{s, bus}.tick(); break;

        T(65534)

        #undef T
    }
}


enum Bus_op : u8 {
    roml_r = 0,  roml_w = 1,
    romh_r = 2,  romh_w = 3,
    io1_r  = 4,  io2_r  = 5,
    io1_w  = 6,  io2_w  = 7,

    _cnt = 8
};


inline void bus_op(u16 exp_type, Bus_op op, const u16& a, u8& d, State::System& s) {
    #define T(t) case (t * Bus_op::_cnt) + Bus_op::roml_r: T##t{s}.roml_r(a, d); return; \
                 case (t * Bus_op::_cnt) + Bus_op::roml_w: T##t{s}.roml_w(a, d); return; \
                 case (t * Bus_op::_cnt) + Bus_op::romh_r: T##t{s}.romh_r(a, d); return; \
                 case (t * Bus_op::_cnt) + Bus_op::romh_w: T##t{s}.romh_w(a, d); return; \
                 case (t * Bus_op::_cnt) + Bus_op::io1_r: T##t{s}.io1_r(a, d); return; \
                 case (t * Bus_op::_cnt) + Bus_op::io2_r: T##t{s}.io2_r(a, d); return; \
                 case (t * Bus_op::_cnt) + Bus_op::io1_w: T##t{s}.io1_w(a, d); return; \
                 case (t * Bus_op::_cnt) + Bus_op::io2_w: T##t{s}.io2_w(a, d); return; \

    switch ((exp_type * Bus_op::_cnt) + op) {
        T(0) T(10)
    }

    #undef T
}

} // namespace Expansion


#endif // EXPANSION_H_INCLUDED
