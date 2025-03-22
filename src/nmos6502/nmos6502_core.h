#ifndef NMOS6502_CORE_H_INCLUDED
#define NMOS6502_CORE_H_INCLUDED

#include "nmos6502.h"
#include "nmos6502_mc.h"


namespace NMOS6502 {


class Core {
public:
    struct State {
        struct R16 { // TODO: big-endian host
            u8 l; u8 h;

            operator u16& () { return *((u16*)&l); }
            void operator=(u16 w) { *((u16*)&l) = w; }
            u16 operator+(u16 w) { return u16(*this) + w; }
            u16 operator+(u8 b) { return u16(*this) + u16(b); }
            u16 operator-(u16 w) { return u16(*this) - w; }
            u16 operator-(u8 b) { return u16(*this) - u16(b); }
        };
        using R8 = u8;

        R16 mcc; // micro-code counter

        R16 pc;
        R16 zpa;
        R16 sp;
        R16 a1;
        R16 a2;
        R16 a3;
        R16 a4;
        R8 d;
        R8 ir;
        R8 p;
        R8 a;
        R8 x;
        R8 y;

        u8 nmi_req;
        u8 nmi_bit; // bit 1 (set --> active)

        u8 irq_req;
        u8 irq_bit; // bit 2 (set --> active)

        u8 brk_src; // bitmap (b0: sw, b1: nmi, b2: irq)

        R16* const r16 = (R16*)&pc;
        R8* const r8 = (R8*)&pc;
    };

    State& s;

    Core(State& s_, Sig& sig_halt_) : s(s_), sig_halt(sig_halt_) { reset_cold(); }

    void reset_warm();
    void reset_cold();

    bool halted() const { return mop().mopc == MC::hlt; }
    bool resume() {
        if (halted()) {
            ++s.mcc;
            return true;
        } else return false;
    }

    void set(Flag f, bool set = true) { s.p = set ? s.p | f : s.p & ~f; }
    void set_nz(const u8& res) { set(Flag::N, res & 0x80); set(Flag::Z, res == 0x00); }
    void clr(Flag f) { s.p &= ~f; }
    bool is_set(Flag f) const { return s.p & f; }
    bool is_clr(Flag f) const { return !is_set(f); }

    const MC::MOP& mop() const { return MC::code[s.mcc]; } // current micro-op

    /* Address space access params (addr, data, r/w).
       The read/write is expected to happen between ticks, e.g.:
           for (;;) {
               do_access(core.mar(), core.mdr(), core.mrw());
               core.tick();
           }
    */
    u16 mar() const { return s.r16[mop().ar]; }
    u8& mdr() const { return s.r8[mop().dr]; }
    u8  mrw() const { return mop().rw; }

    void set_nmi(bool act = true) {
        if (act) {
            if (s.nmi_req == 0x00) s.nmi_req = 0x01;
        } else {
            if (s.nmi_req == 0x02) s.nmi_bit = NMI_BIT;
            s.nmi_req = 0x00;
        }
    }

    void set_irq(bool act = true) {
        if (act) { s.irq_req |= 0x01; }
        else s.irq_req = s.irq_bit = 0x00;
    }

    void tick() {
        if (s.nmi_req == 0x01) s.nmi_req = 0x02;
        else if (s.nmi_req == 0x02) {
            s.nmi_bit = NMI_BIT;
            s.nmi_req = 0x03;
        }

        if (s.irq_req & 0x02) s.irq_bit = IRQ_BIT;
        s.irq_req <<= 1;

        s.pc += mop().pc_inc;

        const auto mopc = mop().mopc;
        ++s.mcc;
        if(mopc != NMOS6502::MC::nmop) exec(mopc);
    }

private:
    void exec(const MC::MOPC mopc);

    // carry & borrow
    u8 C() const { return s.p & Flag::C; }
    u8 B() const { return C() ^ 0x1; }

    void do_asl(u8& d) { set(Flag::C, d & 0x80); set_nz(d <<= 1); }
    void do_lsr(u8& d) { clr(Flag::N); set(Flag::C, d & 0x01); set(Flag::Z, !(d >>= 1)); }
    void do_rol(u8& d) {
        const u8 old_c = s.p & Flag::C;
        set(Flag::C, d & 0x80);
        d <<= 1;
        set_nz(d |= old_c);
    }
    void do_ror(u8& d) {
        const u8 old_c = s.p << 7;
        set(Flag::C, d & 0x01);
        d >>= 1;
        set_nz(d |= old_c);
    }
    void do_bit() { s.p = (s.p & 0x3f) | (s.d & 0xc0); set(Flag::Z, (s.a & s.d) == 0x00); }
    void do_cmp(const State::R8& r) { set(Flag::C, r >= s.d); set_nz(r - s.d); }
    void do_adc();
    void do_sbc();

    void do_ud_anc() { set_nz(s.a &= s.d); set(Flag::C, s.a & 0x80); }
    void do_ud_arr();
    void do_ud_axs() { s.x &= s.a; set(Flag::C, s.x >= s.d); set_nz(s.x -= s.d); }
    void do_ud_las() { set_nz(s.a = s.x = s.sp.l = (s.sp.l & s.d)); }
    void do_ud_lxa() { set_nz(s.a = s.x = (s.a | 0xee) & s.d); }
    void do_ud_rla() { do_rol(s.d); set_nz(s.a &= s.d); }
    void do_ud_rra() { do_ror(s.d); do_adc(); }
    void do_ud_slo() { set(Flag::C, s.d & 0x80); set_nz(s.a |= (s.d <<= 1)); }
    void do_ud_sre() { set(Flag::C, s.d & 0x01); set_nz(s.a ^= (s.d >>= 1)); }
    void do_ud_tas() { s.sp.l = s.a & s.x; s.d = s.sp.l & (s.a1.h + 0x01); }
    void do_ud_xaa() { set_nz(s.a = (s.a | 0xee) & s.x & s.d); }

    void st_reg_sel() {
        switch (s.ir & 0x3) {
            case 0x0: s.d = s.y; return;
            case 0x1: s.d = s.a; return;
            case 0x2: s.d = s.x; return;
            case 0x3: s.d = s.a & s.x; return;
        }
    }

    Sig& sig_halt;

    static constexpr u8 NMI_BIT = 0b00000010;
    static constexpr u8 IRQ_BIT = 0b00000100;

    struct BrkCtrl {
        const u16 pc_t0;  // pc upd @t0
        const u16 pc_t1;  // pc upd @t1
        const u16 vec;    // int.vec. addr.
        const u8  p_mask; // for reseting b (if not a sw brk)
    };
    static constexpr BrkCtrl brk_ctrl[8] = {
        { 0, 0, 0,        0,           }, // unused
        { 0, 1, Vec::irq, Flag::all,   }, // sw brk
        { 1, 0, Vec::nmi, (u8)~Flag::B }, // nmi
        { 1, 0, Vec::nmi, (u8)~Flag::B }, // nmi* & sw brk
        { 1, 0, Vec::irq, (u8)~Flag::B }, // irq
        { 1, 0, Vec::irq, (u8)~Flag::B }, // irq* & sw brk
        { 1, 0, Vec::nmi, (u8)~Flag::B }, // irq & nmi*
        { 1, 0, Vec::nmi, (u8)~Flag::B }, // irq & nmi* & sw brk
    };

};


}


#endif // NMOS6502_CORE_H_INCLUDED
