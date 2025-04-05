#ifndef NMOS6502_CORE_H_INCLUDED
#define NMOS6502_CORE_H_INCLUDED

#include "nmos6502.h"
#include "nmos6502_mc.h"


namespace NMOS6502 {


struct Core {
public:
    struct State {
        Reg16 zpaf;
        Reg16 pc;
        Reg8 d; Reg8 ir;
        Reg16 a1;
        Reg16 a2;
        Reg16 spf;
        Reg8 p; Reg8 a;
        Reg8 x; Reg8 y;
        Reg16 a3;
        Reg16 a4;

        u8 nmi_act;
        u8 nmi_timer;
        u8 irq_act;
        u8 irq_timer;
        u8 brk_srcs;

        const Reg16& r16(Ri16 ri) const { return (&zpaf)[ri]; }
        Reg8& r8(Ri8 ri) const { return ((Reg8*)(&zpaf))[ri]; }

        Reg8& zpa{r8(Ri8::zpa)};
        Reg8& pcl{r8(Ri8::pcl)}; Reg8& pch{r8(Ri8::pch)};
        Reg8& a1l{r8(Ri8::a1l)}; Reg8& a1h{r8(Ri8::a1h)};
        Reg8& sp{r8(Ri8::sp)};
    };
    State& s;

    Core(State& s_, Sig& sig_halt_);

    const MC::MOP* mcp; // micro-code pointer ('mc pc')

    void reset_warm();
    void reset_cold();

    bool halted() const { return mcp->mopc == MC::hlt; }
    bool resume() {
        if (halted()) {
            ++mcp;
            return true;
        } else return false;
    }

    void set(Flag f, bool set = true) { s.p = set ? s.p | f : s.p & ~f; }
    void set_nz(const u8& res) { set(Flag::N, res & 0x80); set(Flag::Z, res == 0x00); }
    void clr(Flag f) { s.p &= ~f; }
    bool is_set(Flag f) const { return s.p & f; }
    bool is_clr(Flag f) const { return !is_set(f); }

    /* Address space access params (addr, data, r/w).
       The read/write is expected to happen between ticks, e.g.:
           for (;;) {
               do_access(core.mar(), core.mdr(), core.mrw());
               core.tick();
           }
    */
    u16 mar() const { return s.r16(mcp->ar); }
    u8& mdr() const { return s.r8(mcp->dr); }
    u8  mrw() const { return mcp->rw; }

    void set_nmi(bool act = true) {
        if (act) {
            if (s.nmi_timer == 0x00) s.nmi_timer = 0x01;
        } else {
            if (s.nmi_timer == 0x02) s.nmi_act = true;
            s.nmi_timer = 0x00;
        }
    }

    void set_irq(bool act = true) {
        if (act) { s.irq_timer |= 0b1; }
        else s.irq_timer = s.irq_act = 0x00;
    }

    void tick() {
        if (s.irq_timer || s.nmi_timer) {
            if (s.nmi_timer == 0x01) s.nmi_timer = 0x02;
            else if (s.nmi_timer == 0x02) {
                s.nmi_act = true;
                s.nmi_timer = 0x03;
            }

            if (s.irq_timer & 0b10) s.irq_act = true;
            s.irq_timer <<= 1;
        }

        s.pc += mcp->pc_inc;

        const auto mopc = (mcp++)->mopc;
        if(mopc != NMOS6502::MC::nmop) exec(mopc);
    }

private:
    void exec(const u8 mop);

    // carry & borrow
    u8 C() const { return s.p & Flag::C; }
    u8 B() const { return C() ^ 0x1; }

    void do_asl(u8& d) { set(Flag::C, d & 0x80); set_nz(d <<= 1); }
    void do_lsr(u8& d) { clr(Flag::N); set(Flag::C, d & 0x01); set(Flag::Z, !(d >>= 1)); }
    void do_rol(u8& d) {
        u8 old_c = s.p & Flag::C;
        set(Flag::C, d & 0x80);
        d <<= 1;
        set_nz(d |= old_c);
    }
    void do_ror(u8& d) {
        u8 old_c = s.p << 7;
        set(Flag::C, d & 0x01);
        d >>= 1;
        set_nz(d |= old_c);
    }
    void do_bit() { s.p = (s.p & 0x3f) | (s.d & 0xc0); set(Flag::Z, (s.a & s.d) == 0x00); }
    void do_cmp(const Reg8& r) { set(Flag::C, r >= s.d); set_nz(r - s.d); }
    void do_adc();
    void do_sbc();

    void do_ud_anc() { set_nz(s.a &= s.d); set(Flag::C, s.a & 0x80); }
    void do_ud_arr();
    void do_ud_axs() { s.x &= s.a; set(Flag::C, s.x >= s.d); set_nz(s.x -= s.d); }
    void do_ud_las() { set_nz(s.a = s.x = s.sp = (s.sp & s.d)); }
    void do_ud_lxa() { set_nz(s.a = s.x = (s.a | 0xee) & s.d); }
    void do_ud_rla() { do_rol(s.d); set_nz(s.a &= s.d); }
    void do_ud_rra() { do_ror(s.d); do_adc(); }
    void do_ud_slo() { set(Flag::C, s.d & 0x80); set_nz(s.a |= (s.d <<= 1)); }
    void do_ud_sre() { set(Flag::C, s.d & 0x01); set_nz(s.a ^= (s.d >>= 1)); }
    void do_ud_tas() { s.sp = s.a & s.x; s.d = s.sp & (s.a1h + 0x01); }
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
};


}


#endif // NMOS6502_CORE_H_INCLUDED
