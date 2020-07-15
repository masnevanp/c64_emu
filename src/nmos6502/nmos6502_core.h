#ifndef NMOS6502_CORE_H_INCLUDED
#define NMOS6502_CORE_H_INCLUDED

#include "nmos6502.h"
#include "nmos6502_mc.h"


namespace NMOS6502 {


class Core {
public:
    static const int REG_CNT = 10;

    Reg16 r16[REG_CNT]; // pc, sp, p|a, x|y, d|ir, zpaf, a1, a2, a3, a4
    Reg8* r8;

    Reg16& pc; Reg16& spf; Reg16& zpaf; Reg16& a1; Reg16& a2; Reg16& a3; Reg16& a4;
    Reg8& pcl; Reg8& pch; Reg8& sp; Reg8& p; Reg8& a; Reg8& x; Reg8& y;
    Reg8& d; Reg8& ir; Reg8& zpa; Reg8& a1l; Reg8& a1h;

    const MC::MOP* mcp; // micro-code pointer ('mc pc')

    Sig sig_halt = [](){};


    Core();

    void reset_warm();
    void reset_cold();

    bool halted() const { return mcp->mopc == MC::hlt; }

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
    u16 mar() const { return r16[mcp->ar]; }
    u8& mdr() const { return r8[mcp->dr]; }
    u8  mrw() const { return mcp->rw; }

    void set_nmi(bool act = true) {
        if (act) {
            if (nmi_req == 0x00) nmi_req = 0x01;
        } else {
            if (nmi_req == 0x02) nmi_bit = NMI_BIT;
            nmi_req = 0x00;
        }
    }

    void set_irq(bool act = true) {
        if (act) { irq_req |= 0x01; }
        else irq_req = irq_bit = 0x00;
    }

    void tick() {
        if (nmi_req == 0x01) nmi_req = 0x02;
        else if (nmi_req == 0x02) {
            nmi_bit = NMI_BIT;
            nmi_req = 0x03;
        }

        if (irq_req & 0x02) irq_bit = IRQ_BIT;
        irq_req <<= 1;

        exec_cycle();
    }

private:
    static const u8 OPC_brk = 0x00;
    static const u8 NMI_taken = 0x80;

    void exec_cycle();

    // carry & borrow
    u8 C() const { return p & Flag::C; }
    u8 B() const { return C() ^ 0x1; }

    void do_bra() {
        // mapping: condition code -> flag bit position
        static const u8 cc_flag_pos[8] = { 7, 7, 6, 6, 0, 0, 1, 1 };

        int cc = ir >> 5;
        bool no_bra = ((p >> cc_flag_pos[cc]) ^ cc) & 0x01;

        if (no_bra) {
            ++mcp;
        } else {
            a1 = a2 = pc;
            a1l += d;
            pc += (i8)d;
            mcp += (pch - a1h ? 4 : 2);
        }
    }

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
        switch (ir & 0x03) {
            case 0x0: d = y; return;
            case 0x1: d = a; return;
            case 0x2: d = x; return;
            case 0x3: d = a & x; return;
        }
    }

    u8 nmi_req;
    u8 irq_req;

    u8 nmi_bit; // bit 1 (0x02 --> active)
    u8 irq_bit; // bit 2 (0x04 --> active)
    u8 brk_src; // bitmap (b0: sw, b1: nmi, b2: irq)

    static const u8 NMI_BIT = 0x02;
    static const u8 IRQ_BIT = 0x04;

    struct BrkCtrl {
        u16 pc_t0;  // pc upd @t0
        u16 pc_t1;  // pc upd @t1
        u16 vec;    // int.vec. addr.
        u8  p_mask; // for reseting b (if not a sw brk)
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
