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
    void tick() {}

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


bool attach(State::System& s, const Files::CRT& crt);
void detach(State::System& s);

void reset(State::System& s);
void tick(State::System& s);


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
