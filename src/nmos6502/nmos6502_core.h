#ifndef NMOS6502_CORE_H_INCLUDED
#define NMOS6502_CORE_H_INCLUDED

#include "nmos6502.h"
#include "nmos6502_mc.h"


namespace NMOS6502 {


struct Core {
public:
    Core(Sig& sig_halt_);

    const Reg16& r16(Ri16 ri) const { return (&zpaf)[ri]; }
    Reg8& r8(Ri8 ri) const { return ((Reg8*)(&zpaf))[ri]; }

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

    Reg8& zpa{r8(Ri8::zpa)};
    Reg8& pcl{r8(Ri8::pcl)}; Reg8& pch{r8(Ri8::pch)};
    Reg8& a1l{r8(Ri8::a1l)}; Reg8& a1h{r8(Ri8::a1h)};
    Reg8& sp{r8(Ri8::sp)};

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

    void set(Flag f, bool set = true) { p = set ? p | f : p & ~f; }
    void set_nz(const u8& res) { set(Flag::N, res & 0x80); set(Flag::Z, res == 0x00); }
    void clr(Flag f) { p &= ~f; }
    bool is_set(Flag f) const { return p & f; }
    bool is_clr(Flag f) const { return !is_set(f); }

    /* Address space access params (addr, data, r/w).
       The read/write is expected to happen between ticks, e.g.:
           for (;;) {
               do_access(core.mar(), core.mdr(), core.mrw());
               core.tick();
           }
    */
    u16 mar() const { return r16(mcp->ar); }
    u8& mdr() const { return r8(mcp->dr); }
    u8  mrw() const { return mcp->rw; }

    void set_nmi(bool act = true) {
        if (act) {
            if (nmi_timer == 0x00) nmi_timer = 0x01;
        } else {
            if (nmi_timer == 0x02) nmi_bit = NMI_BIT;
            nmi_timer = 0x00;
        }
    }

    void set_irq(bool act = true) {
        if (act) { irq_timer |= 0b1; }
        else irq_timer = irq_bit = 0x00;
    }

    void tick() {
        if (nmi_timer == 0x01) nmi_timer = 0x02;
        else if (nmi_timer == 0x02) {
            nmi_bit = NMI_BIT;
            nmi_timer = 0x03;
        }

        if (irq_timer & 0b10) irq_bit = IRQ_BIT;
        irq_timer <<= 1;

        pc += mcp->pc_inc;

        const auto mopc = (mcp++)->mopc;
        if(mopc != NMOS6502::MC::nmop) exec(mopc);
    }

private:
    void exec(const u8 mop);

    // carry & borrow
    u8 C() const { return p & Flag::C; }
    u8 B() const { return C() ^ 0x1; }

    void do_asl(u8& d) { set(Flag::C, d & 0x80); set_nz(d <<= 1); }
    void do_lsr(u8& d) { clr(Flag::N); set(Flag::C, d & 0x01); set(Flag::Z, !(d >>= 1)); }
    void do_rol(u8& d) {
        u8 old_c = p & Flag::C;
        set(Flag::C, d & 0x80);
        d <<= 1;
        set_nz(d |= old_c);
    }
    void do_ror(u8& d) {
        u8 old_c = p << 7;
        set(Flag::C, d & 0x01);
        d >>= 1;
        set_nz(d |= old_c);
    }
    void do_bit() { p = (p & 0x3f) | (d & 0xc0); set(Flag::Z, (a & d) == 0x00); }
    void do_cmp(const Reg8& r) { set(Flag::C, r >= d); set_nz(r - d); }
    void do_adc();
    void do_sbc();

    void do_ud_anc() { set_nz(a &= d); set(Flag::C, a & 0x80); }
    void do_ud_arr();
    void do_ud_axs() { x &= a; set(Flag::C, x >= d); set_nz(x -= d); }
    void do_ud_las() { set_nz(a = x = sp = (sp & d)); }
    void do_ud_lxa() { set_nz(a = x = (a | 0xee) & d); }
    void do_ud_rla() { do_rol(d); set_nz(a &= d); }
    void do_ud_rra() { do_ror(d); do_adc(); }
    void do_ud_slo() { set(Flag::C, d & 0x80); set_nz(a |= (d <<= 1)); }
    void do_ud_sre() { set(Flag::C, d & 0x01); set_nz(a ^= (d >>= 1)); }
    void do_ud_tas() { sp = a & x; d = sp & (a1h + 0x01); }
    void do_ud_xaa() { set_nz(a = (a | 0xee) & x & d); }

    void st_reg_sel() {
        switch (ir & 0x3) {
            case 0x0: d = y; return;
            case 0x1: d = a; return;
            case 0x2: d = x; return;
            case 0x3: d = a & x; return;
        }
    }

    Sig& sig_halt;

    u8 nmi_timer;
    u8 irq_timer;

    u8 nmi_bit; // bit 1 (set --> active)
    u8 irq_bit; // bit 2 (set --> active)
    u8 brk_src; // bitmap (b0: sw, b1: nmi, b2: irq)

    static constexpr u8 NMI_BIT = 0b00000010;
    static constexpr u8 IRQ_BIT = 0b00000100;
};


}


#endif // NMOS6502_CORE_H_INCLUDED
