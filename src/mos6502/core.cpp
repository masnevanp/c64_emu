
#include "core.h"



namespace MOS6502 {
    using RW = MOS6502::Core::State::Bus::RW;

    enum Brk_src : u8 { sw = 0b001, nmi = 0b010, irq = 0b100, };

    inline u16 zp(u16 a) { return a & 0x00ff; }
    inline u16 sp(u16 a) { return (a & 0xff) | 0x0100; }

    struct Op {
        Core::State& s;

        void schedule(u16 opc) { s.mcc = opc << 3; } // bits 0-2 encode the step (0..7)

        void check_irq() { if (s.irq_act && is_clr(Flag::I)) s.brk_srcs |= Brk_src::irq; }
    
        bool is_set(Flag f) const { return s.p & f; }
        bool is_clr(Flag f) const { return !is_set(f); }

        void set_nz(const u8& res) { s.set(Flag::N, res & 0x80); s.set(Flag::Z, res == 0x00); }

        u8 C() const { return s.p & Flag::C; } // carry
        u8 B() const { return C() ^ 1; } // borrow

        void bra(bool cond) {
            s.bus.a += 1;

            if (cond) {
                s.pc = s.bus.a + i8(s.bus.d);
                schedule(OPC::bra);
                if ((s.pc ^ s.bus.a) & 0xff00) { // page cross?
                    s.aux = (s.bus.a & 0xff00) | (s.pc & 0x00ff);
                    s.mcc += 1; // schedule 'page cross' step
                }
            } else {
                schedule(OPC::dispatch);
            }
        }

        void adc();
        void and_()     { set_nz(s.a &= s.bus.d); }
        void asl(u8& d) { s.set(Flag::C, d & 0x80); set_nz(d <<= 1); }
        void bit()      { s.p = (s.p & 0x3f) | (s.bus.d & 0xc0); s.set(Flag::Z, (s.a & s.bus.d) == 0x00); }
        void cmp(u8 r)  { s.set(Flag::C, r >= s.bus.d); set_nz(r - s.bus.d); }
        void dec(u8& d) { set_nz(--d); }
        void eor()      { set_nz(s.a ^= s.bus.d); }
        void inc(u8& d) { set_nz(++d); }
        void lax()      { set_nz(s.a = s.x = s.bus.d); }
        void ldr(u8& r) { set_nz(r = s.bus.d); }
        void lsr(u8& d) { s.clr(Flag::N); s.set(Flag::C, d & 0x01); s.set(Flag::Z, !(d >>= 1)); }
        void ora()      { set_nz(s.a |= s.bus.d); }
        void rol(u8& d) {
            const u8 old_c = s.p & Flag::C;
            s.set(Flag::C, d & 0x80);
            d <<= 1;
            set_nz(d |= old_c);
        }
        void ror(u8& d) {
            const u8 old_c = s.p << 7;
            s.set(Flag::C, d & 0x01);
            d >>= 1;
            set_nz(d |= old_c);
        }
        void sbc();
        void trr(u8 sr, u8& tr) { set_nz(tr = sr); }

        void ud_alr() { s.a &= s.bus.d; lsr(s.a); }
        void ud_anc() { Op{s}.and_(); s.set(Flag::C, s.a & 0x80); }
        void ud_arr();
        void ud_axs() { s.x &= s.a; s.set(Flag::C, s.x >= s.bus.d); set_nz(s.x -= s.bus.d); }
        void ud_dcp() { --s.bus.d; cmp(s.a); }
        void ud_isc() { ++s.bus.d; sbc(); }
        void ud_las() { set_nz(s.a = s.x = (s.sp & s.bus.d)); s.sp = sp(s.a); }
        void ud_lxa() { set_nz(s.a = s.x = (s.a | 0xee) & s.bus.d); }
        void ud_rla() { rol(s.bus.d); Op{s}.and_(); }
        void ud_rra() { ror(s.bus.d); adc(); }
        void ud_slo() { s.set(Flag::C, s.bus.d & 0x80); set_nz(s.a |= (s.bus.d <<= 1)); }
        void ud_sre() { s.set(Flag::C, s.bus.d & 0x01); set_nz(s.a ^= (s.bus.d >>= 1)); }
        void ud_tas() { s.bus.d &= s.a; s.sp = sp(s.a & s.x); }
        void ud_xaa() { set_nz(s.a = (s.a | 0xee) & s.x & s.bus.d); }

        void halt(u8 opc, u8 d);
    };

    void Op::adc() {
        if (is_set(Flag::D)) {
            /* "The Z flag is computed before performing any decimal adjust. In other words,
                the Z flag reflects the result of a binary addition. The N and V flags are
                computed after a decimal adjust of the low nibble, but before adjusting
                the high nibble."

                -- Jorge Cwik, Flags on Decimal mode in the NMOS 6502
                   http://atariage.com/forums/topic/163876-flags-on-decimal-mode-on-the-nmos-6502/
            */
            auto adc_dec_nib = [](u8 res, u8& co) -> u8 {
                if (res > 0x9) { co = 0x1; return (res + 0x6) & 0xf; }
                else { co = 0x0; return res; }
            };
            s.set(Flag::Z, ((s.a + s.bus.d + C()) & 0xff) == 0x00); // Z set based on binary result
            u8 n1; u8 c1; u8 n2; u8 c2;
            n1 = adc_dec_nib((s.a & 0xf) + (s.bus.d & 0xf) + C(), c1);
            n2 = (s.a >> 4) + (s.bus.d >> 4) + c1;
            u8 r = (n2 << 4) | n1; // high nib not adjusted yet (N&V set based on this)
            s.set(Flag::N, r & 0x80);
            s.set(Flag::V, ((s.a ^ r) & (s.bus.d ^ r) & 0x80));
            n2 = adc_dec_nib(n2, c2);
            s.set(Flag::C, c2); // decimal carry
            s.a = (n2 << 4) | n1;
        } else {
            u16 r = s.a + s.bus.d + C();
            s.set(Flag::C, r & 0x100);
            s.set(Flag::V, ((s.a ^ r) & (s.bus.d ^ r) & 0x80));
            set_nz(s.a = r);
        }
    }

    void Op::sbc() {
        if (is_set(Flag::D)) {
            auto sbc_dec_nib = [](u8 res, u8& co) -> u8 {
                if (res > 0xf) { co = 0x1; return (res - 0x6) & 0xf; }
                else { co = 0x0; return res; }
            };
            s.set(Flag::Z, ((s.a - s.bus.d - B()) & 0xff) == 0x00);
            u8 n1; u8 c1; u8 n2; u8 c2;
            n1 = sbc_dec_nib((s.a & 0xf) - (s.bus.d & 0xf) - B(), c1);
            n2 = (s.a >> 4) - (s.bus.d >> 4) - c1;
            u8 r = (n2 << 4) | n1;
            s.set(Flag::N, r & 0x80);
            s.set(Flag::V, ((s.a ^ s.bus.d) & (s.a ^ r)) & 0x80);
            n2 = sbc_dec_nib(n2, c2);
            s.set(Flag::C, !c2);
            s.a = (n2 << 4) | n1;
        } else {
            u16 r = s.a - s.bus.d - B();
            s.set(Flag::C, r < 0x100);
            s.set(Flag::V, ((s.a ^ s.bus.d) & (s.a ^ r)) & 0x80);
            set_nz(s.a = r);
        }
    }

    void Op::ud_arr() {
        /* Totally based on c64doc.txt by John West & Marko Mäkelä.
            (http://www.6502.org/users/andre/petindex/local/64doc.txt)
        */
        if (is_set(Flag::D)) {
            s.bus.d &= s.a;

            u8 ah = s.bus.d >> 4;
            u8 al = s.bus.d & 0xf;

            s.set(Flag::N, is_set(Flag::C));
            s.a = (s.bus.d >> 1) | (s.p << 7);
            s.set(Flag::Z, s.a == 0x00);
            s.set(Flag::V, (s.bus.d ^ s.a) & 0x40);

            if (al + (al & 0x1) > 0x5) s.a = (s.a & 0xf0) | ((s.a + 0x6) & 0xf);
            if ((ah + (ah & 0x1)) > 0x5) { s.set(Flag::C); s.a += 0x60; }
            else s.clr(Flag::C);
        } else {
            s.a &= s.bus.d;
            s.a >>= 1;
            s.a |= (s.p << 7);
            set_nz(s.a);
            s.set(Flag::C, s.a & 0x40);
            s.set(Flag::V, (s.a ^ (s.a << 1)) & 0x40);
        }
    }

    void Op::halt(u8 opc, u8 d) {
        s.aux = (d << 8) | opc; // temp store for sig_halt()
        s.pc = s.bus.a + 1;
        s.bus.a = 0xffff;
        schedule(OPC::halt);
    }
}


MOS6502::Core::Core(State& s_, Sig_halt& sig_halt_) : s(s_), sig_halt(sig_halt_) {}


void MOS6502::Core::reset() {
    s.brk_srcs = 0;
    s.nmi_timer = s.irq_timer = 0;
    s.nmi_act = s.irq_act = false;
    s.bus(RW::r);
    Op{s}.schedule(OPC::reset);
}


void MOS6502::Core::set_nmi(bool act) {
    if (act) {
        if (s.nmi_timer == 0x00) s.nmi_timer = 0x01;
    } else {
        if (s.nmi_timer == 0x02) s.nmi_act = true;
        s.nmi_timer = 0x00;
    }
}

void MOS6502::Core::set_irq(bool act) {
    if (act) { s.irq_timer |= 0b1; }
    else s.irq_timer = s.irq_act = 0x00;
}


// **************** Single Byte Instructions ****************

#define sb(opc, op) { \
    case mc(opc, 0): \
        op; \
        Op{s}.schedule(OPC::dispatch); \
        break; \
}

// ******** Internal Execution On Memory Data ********

#define ie_i(opc, op) { \
    case mc(opc, 0): \
        op; \
        s.bus.a += 1; \
        Op{s}.schedule(OPC::dispatch); \
        break; \
}

#define ie_z(opc, op) { \
    case mc(opc, 0): \
        s.pc = s.bus.a + 1; \
        s.bus.a = s.bus.d; \
        break; \
    case mc(opc, 1): \
        op; \
        s.bus.a = s.pc; \
        Op{s}.schedule(OPC::dispatch); \
        break; \
}

#define ie_a(opc, op) { \
    case mc(opc, 0): \
        s.aux = s.bus.d; \
        s.pc = s.bus.a + 2; \
        s.bus.a += 1; \
        break; \
    case mc(opc, 1): \
        s.bus.a = s.aux | (s.bus.d << 8); \
        break; \
    case mc(opc, 2): \
        op; \
        s.bus.a = s.pc; \
        Op{s}.schedule(OPC::dispatch); \
        break; \
}

#define ie_izx(opc, op) { \
    case mc(opc, 0): \
        s.pc = s.bus.a + 1; \
        s.bus.a = s.bus.d; \
        break; \
    case mc(opc, 1): \
        s.bus.a = zp(s.bus.a + s.x); \
        break; \
    case mc(opc, 2): \
        s.aux = s.bus.d; \
        s.bus.a = zp(s.bus.a + 1); \
        break; \
    case mc(opc, 3): \
        s.bus.a = s.aux | (s.bus.d << 8); \
        break; \
    case mc(opc, 4): \
        op; \
        s.bus.a = s.pc; \
        Op{s}.schedule(OPC::dispatch); \
        break; \
}

#define ie_ai(opc, ireg, op) { \
    case mc(opc, 0): \
        s.aux = s.bus.d + ireg; \
        s.pc = s.bus.a + 2; \
        s.bus.a += 1; \
        break; \
    case mc(opc, 1): \
        s.bus.a = (s.aux & 0xff) | (s.bus.d << 8); \
        s.mcc += (s.aux >> 8); \
        break; \
    case mc(opc, 2): \
        op; \
        s.bus.a = s.pc; \
        Op{s}.schedule(OPC::dispatch); \
        break; \
    case mc(opc, 3): \
        s.bus.a += 0x100; \
        break; \
    case mc(opc, 4): \
        op; \
        s.bus.a = s.pc; \
        Op{s}.schedule(OPC::dispatch); \
        break; \
}

#define ie_zi(opc, ireg, op) { \
    case mc(opc, 0): \
        s.pc = s.bus.a + 1; \
        s.bus.a = s.bus.d; \
        break; \
    case mc(opc, 1): \
        s.bus.a = zp(s.bus.a + ireg); \
        break; \
    case mc(opc, 2): \
        op; \
        s.bus.a = s.pc; \
        Op{s}.schedule(OPC::dispatch); \
        break; \
}

#define ie_izy(opc, op) { \
    case mc(opc, 0): \
        s.pc = s.bus.a + 1; \
        s.bus.a = s.bus.d; \
        break; \
    case mc(opc, 1): \
        s.aux = s.bus.d + s.y; \
        s.bus.a = zp(s.bus.a + 1); \
        break; \
    case mc(opc, 2): \
        s.bus.a = (s.aux & 0xff) | (s.bus.d << 8); \
        s.mcc += (s.aux >> 8); \
        break; \
    case mc(opc, 3): \
        op; \
        s.bus.a = s.pc; \
        Op{s}.schedule(OPC::dispatch); \
        break; \
    case mc(opc, 4): \
        s.bus.a += 0x100; \
        break; \
    case mc(opc, 5): \
        op; \
        s.bus.a = s.pc; \
        Op{s}.schedule(OPC::dispatch); \
        break; \
}

// **************** Store Operations ****************

#define st_z(opc, reg) { \
    case mc(opc, 0): \
        s.pc = s.bus.a + 1; \
        s.bus.a = s.bus.d; \
        s.bus.d = reg; \
        s.bus(RW::w); \
        break; \
    case mc(opc, 1): \
        s.bus.a = s.pc; \
        s.bus(RW::r); \
        Op{s}.schedule(OPC::dispatch); \
        break; \
}

#define st_a(opc, reg) { \
    case mc(opc, 0): \
        s.aux = s.bus.d; \
        s.pc = s.bus.a + 2; \
        s.bus.a += 1; \
        break; \
    case mc(opc, 1): \
        s.bus.a = s.aux | (s.bus.d << 8); \
        s.bus.d = reg; \
        s.bus(RW::w); \
        break; \
    case mc(opc, 2): \
        s.bus.a = s.pc; \
        s.bus(RW::r); \
        Op{s}.schedule(OPC::dispatch); \
        break; \
}

#define st_izx(opc, reg) { \
    case mc(opc, 0): \
        s.pc = s.bus.a + 1; \
        s.bus.a = s.bus.d; \
        break; \
    case mc(opc, 1): \
        s.bus.a = zp(s.bus.a + s.x); \
        break; \
    case mc(opc, 2): \
        s.aux = s.bus.d; \
        s.bus.a = zp(s.bus.a + 1); \
        break; \
    case mc(opc, 3): \
        s.bus.a = s.aux | (s.bus.d << 8); \
        s.bus.d = reg; \
        s.bus(RW::w); \
        break; \
    case mc(opc, 4): \
        s.bus.a = s.pc; \
        s.bus(RW::r); \
        Op{s}.schedule(OPC::dispatch); \
        break; \
}

#define st_ai(opc, ireg) { \
    case mc(opc, 0): \
        s.aux = s.bus.d + ireg; \
        s.pc = s.bus.a + 2; \
        s.bus.a += 1; \
        break; \
    case mc(opc, 1): \
        s.bus.a = (s.aux & 0xff) | (s.bus.d << 8); \
        break; \
    case mc(opc, 2): \
        s.bus.a += (s.aux & 0x100); \
        s.bus.d = s.a; \
        s.bus(RW::w); \
        break; \
    case mc(opc, 3): \
        s.bus.a = s.pc; \
        s.bus(RW::r); \
        Op{s}.schedule(OPC::dispatch); \
        break; \
}

#define st_zi(opc, ireg, reg) { \
    case mc(opc, 0): \
        s.pc = s.bus.a + 1; \
        s.bus.a = s.bus.d; \
        break; \
    case mc(opc, 1): \
        s.bus.a = zp(s.bus.a + ireg); \
        s.bus.d = reg; \
        s.bus(RW::w); \
        break; \
    case mc(opc, 2): \
        s.bus.a = s.pc; \
        s.bus(RW::r); \
        Op{s}.schedule(OPC::dispatch); \
        break; \
}

#define st_izy(opc) { \
    case mc(opc, 0): \
        s.pc = s.bus.a + 1; \
        s.bus.a = s.bus.d; \
        break; \
    case mc(opc, 1): \
        s.aux = s.bus.d + s.y; \
        s.bus.a = zp(s.bus.a + 1); \
        break; \
    case mc(opc, 2): \
        s.bus.a = (s.aux & 0xff) | (s.bus.d << 8); \
        break; \
    case mc(opc, 3): \
        s.bus.a += (s.aux & 0x100); \
        s.bus.d = s.a; \
        s.bus(RW::w); \
        break; \
    case mc(opc, 4): \
        s.bus.a = s.pc; \
        s.bus(RW::r); \
        Op{s}.schedule(OPC::dispatch); \
        break; \
}

// **************** Read-Modify-Write -operations ****************

#define rmw_z(opc, op) { \
    case mc(opc, 0): \
        s.pc = s.bus.a + 1; \
        s.bus.a = s.bus.d; \
        break; \
    case mc(opc, 1): \
        s.bus(RW::w); \
        break; \
    case mc(opc, 2): \
        op; \
        break; \
    case mc(opc, 3): \
        s.bus.a = s.pc; \
        s.bus(RW::r); \
        Op{s}.schedule(OPC::dispatch); \
        break; \
}

#define rmw_a(opc, op) { \
    case mc(opc, 0): \
        s.aux = s.bus.d; \
        s.pc = s.bus.a + 2; \
        s.bus.a += 1; \
        break; \
    case mc(opc, 1): \
        s.bus.a = s.aux | (s.bus.d << 8); \
        break; \
    case mc(opc, 2): \
        s.bus(RW::w); \
        break; \
    case mc(opc, 3): \
        op; \
        break; \
    case mc(opc, 4): \
        s.bus.a = s.pc; \
        s.bus(RW::r); \
        Op{s}.schedule(OPC::dispatch); \
        break; \
}

#define rmw_zx(opc, op) { \
    case mc(opc, 0): \
        s.pc = s.bus.a + 1; \
        s.bus.a = s.bus.d; \
        break; \
    case mc(opc, 1): \
        s.bus.a = zp(s.bus.a + s.x); \
        break; \
    case mc(opc, 2): \
        s.bus(RW::w); \
        break; \
    case mc(opc, 3): \
        op; \
        break; \
    case mc(opc, 4): \
        s.bus.a = s.pc; \
        s.bus(RW::r); \
        Op{s}.schedule(OPC::dispatch); \
        break; \
}

#define rmw_ai(opc, ireg, op) { \
    case mc(opc, 0): \
        s.aux = s.bus.d + ireg; \
        s.pc = s.bus.a + 2; \
        s.bus.a += 1; \
        break; \
    case mc(opc, 1): \
        s.bus.a = (s.aux & 0xff) | (s.bus.d << 8); \
        break; \
    case mc(opc, 2): \
        s.bus.a += (s.aux & 0x100); \
        break; \
    case mc(opc, 3): \
        s.bus(RW::w); \
        break; \
    case mc(opc, 4): \
        op; \
        break; \
    case mc(opc, 5): \
        s.bus.a = s.pc; \
        s.bus(RW::r); \
        Op{s}.schedule(OPC::dispatch); \
        break; \
}

// **************** Miscellaneous Operations ****************

/* Interrupt hijacking:
    - happens if an interrupt of a higher priority is signalled before
        the 'brk' execution of the lower priority reaches T5 (=T4 ph2 at latest).
    - T0&T1 PC, and pushed b-flag according to the former (lower prio)
    - vector address according to the latter (higher prio)
*/
#define m_brk(opc) \
    case mc(opc, 0): \
        s.bus(s.sp, (s.pc >> 8), RW::w); \
        break; \
    case mc(opc, 1): \
        s.bus(sp(s.bus.a - 1), s.pc); \
        break; \
    case mc(opc, 2): { \
        const auto p = s.p | Flag::u; \
        s.bus(sp(s.bus.a - 1), p); \
        s.sp = sp(s.bus.a - 1); \
        break; } \
    case mc(opc, 3): \
        if (s.nmi_timer & 0b11) /*Potential hijacking by nmi*/ { \
            s.brk_srcs = Brk_src::nmi; \
            s.nmi_timer = nmi_timer_handled; \
        } \
        s.bus.a = (s.brk_srcs & Brk_src::nmi) ? Vec::nmi : Vec::irq; \
        s.brk_srcs = 0; \
        s.bus(RW::r); \
        break; \
    case mc(opc, 4): \
        s.aux = s.bus.d; \
        s.bus.a += 1; \
        break; \
    case mc(opc, 5): \
        s.bus.a = s.aux | (s.bus.d << 8); \
        s.set(Flag::I); \
        Op{s}.schedule(OPC::dispatch_post_brk); \
        break; \

#define m_ja(opc) \
    case mc(opc, 0): \
        s.aux = s.bus.d; \
        s.bus.a += 1; \
        break; \
    case mc(opc, 1): \
        s.bus.a = s.aux | (s.bus.d << 8); \
        Op{s}.schedule(OPC::dispatch); \
        break; \

#define m_ji(opc) \
    case mc(opc, 0): \
        s.aux = s.bus.d; \
        s.bus.a += 1; \
        break; \
    case mc(opc, 1): \
        s.bus.a = s.aux | (s.bus.d << 8); \
        break; \
    case mc(opc, 2): \
        s.aux = s.bus.d; \
        /* the 'missing carry propagation' feature*/ \
        s.bus.a = (s.bus.a & 0xff00) | ((s.bus.a + 1) & 0xff); \
        break; \
    case mc(opc, 3): \
        s.bus.a = s.aux | (s.bus.d << 8); \
        Op{s}.schedule(OPC::dispatch); \
        break; \

#define m_js(opc) \
    case mc(opc, 0): \
        s.aux = s.bus.d; \
        s.pc = s.bus.a + 1; \
        s.bus.a = s.sp; \
        s.sp = sp(s.sp - 2); \
        break; \
    case mc(opc, 1): \
        s.bus.d = (s.pc >> 8); \
        s.bus(RW::w); \
        break; \
    case mc(opc, 2): \
        s.bus(sp(s.bus.a - 1), u8(s.pc)); \
        break; \
    case mc(opc, 3): \
        s.bus.a = s.pc; \
        s.bus(RW::r); \
        break; \
    case mc(opc, 4): \
        s.bus.a = (s.aux | (s.bus.d << 8)); \
        Op{s}.schedule(OPC::dispatch); \
        break; \

#define m_rts(opc) \
    case mc(opc, 0): \
        s.bus.a = s.sp; \
        s.sp = sp(s.sp + 2); \
        break; \
    case mc(opc, 1): \
        s.bus.a = sp(s.bus.a + 1); \
        break; \
    case mc(opc, 2): \
        s.pc = s.bus.d; \
        s.bus.a = sp(s.bus.a + 1); \
        break; \
    case mc(opc, 3): \
        s.bus.a = s.pc | (s.bus.d << 8); \
        break; \
    case mc(opc, 4): \
        s.bus.a += 1; \
        Op{s}.schedule(OPC::dispatch); \
        break; \

#define m_rti(opc) \
    case mc(opc, 0): \
        s.bus.a = s.sp; \
        s.sp = sp(s.sp + 3); \
        break; \
    case mc(opc, 1): \
        s.bus.a = sp(s.bus.a + 1); \
        break; \
    case mc(opc, 2): \
        s.p = s.bus.d; \
        s.bus.a = sp(s.bus.a + 1); \
        break; \
    case mc(opc, 3): \
        s.pc = s.bus.d; \
        s.bus.a = sp(s.bus.a + 1); \
        break; \
    case mc(opc, 4): \
        s.bus.a = s.pc | (s.bus.d << 8); \
        Op{s}.schedule(OPC::dispatch); \
        break; \

#define m_brs(opc, flag) { \
    case mc(opc, 0): \
        Op{s}.bra(Op{s}.is_set(flag)); \
        break; \
}

#define m_brc(opc, flag) { \
    case mc(opc, 0): \
        Op{s}.bra(Op{s}.is_clr(flag)); \
        break; \
}

#define m_phr(opc, reg) { \
    case mc(opc, 0): \
        s.pc = s.bus.a; \
        s.bus(s.sp, reg, RW::w); \
        s.sp = sp(s.sp - 1); \
        break; \
    case mc(opc, 1): \
        s.bus(s.pc, RW::r); \
        Op{s}.schedule(OPC::dispatch); \
        break; \
}

#define m_plr(opc, op) { \
    case mc(opc, 0): \
        s.pc = s.bus.a; \
        s.bus.a = s.sp; \
        break; \
    case mc(opc, 1): \
        s.sp = sp(s.sp + 1); \
        s.bus.a = s.sp; \
        break; \
    case mc(opc, 2): \
        op; \
        s.bus.a = s.pc; \
        Op{s}.schedule(OPC::dispatch); \
        break; \
}

#define m_hlt(opc) { \
    case mc(opc, 0): \
        Op{s}.halt(opc, s.bus.d); \
        break; \
}

#define m_sch(opc, nxt) { \
    case mc(opc, 0): \
        Op{s}.schedule(nxt); \
        break; \
}

// **************** Undefined ****************

#define ud_izx(opc, op) { \
    case mc(opc, 0): \
        s.pc = s.bus.a + 1; \
        s.bus.a = s.bus.d; \
        break; \
    case mc(opc, 1): \
        s.bus.a = zp(s.bus.a + s.x); \
        break; \
    case mc(opc, 2): \
        s.aux = s.bus.d; \
        s.bus.a = zp(s.bus.a + 1); \
        break; \
    case mc(opc, 3): \
        s.bus.a = s.aux | (s.bus.d << 8); \
        break; \
    case mc(opc, 4): \
        s.bus(RW::w); \
        break; \
    case mc(opc, 5): \
        op; \
        break; \
    case mc(opc, 6): \
        s.bus.a = s.pc; \
        s.bus(RW::r); \
        Op{s}.schedule(OPC::dispatch); \
        break; \
}

#define ud_izy(opc, op) { \
    case mc(opc, 0): \
        s.pc = s.bus.a + 1; \
        s.bus.a = s.bus.d; \
        break; \
    case mc(opc, 1): \
        s.aux = s.bus.d + s.y; \
        s.bus.a = zp(s.bus.a + 1); \
        break; \
    case mc(opc, 2): \
        s.bus.a = (s.aux & 0xff) | (s.bus.d << 8); \
        break; \
    case mc(opc, 3): \
        s.bus.a += (s.aux & 0x100); \
        break; \
    case mc(opc, 4): \
        s.bus(RW::w); \
        break; \
    case mc(opc, 5): \
        op; \
        break; \
    case mc(opc, 6): \
        s.bus.a = s.pc; \
        s.bus(RW::r); \
        Op{s}.schedule(OPC::dispatch); \
        break; \
}

#define ud_ai(opc, ir1, ir2, op) { \
    case mc(opc, 0): \
        s.aux = s.bus.d + ir1; \
        s.pc = s.bus.a + 2; \
        s.bus.a += 1; \
        break; \
    case mc(opc, 1): \
        s.bus.a = (s.bus.d << 8) | (s.aux & 0xff); \
        break; \
    case mc(opc, 2): \
        s.bus.d = ir2 & ((s.bus.a + 0x100) >> 8); \
        op; \
        s.bus.a = (s.aux & 0x100) \
            ? ((s.bus.d << 8) | (s.bus.a & 0xff)) \
            : s.bus.a; \
        s.bus(RW::w); \
        break; \
    case mc(opc, 3): \
        s.bus.a = s.pc; \
        s.bus(RW::r); \
        Op{s}.schedule(OPC::dispatch); \
        break; \
}

#define ud_ahx(opc) { \
    case mc(opc, 0): \
        s.pc = s.bus.a + 1; \
        s.bus.a = s.bus.d; \
        break; \
    case mc(opc, 1): \
        s.aux = s.bus.d + s.y; \
        s.bus.a = zp(s.bus.a + 1); \
        break; \
    case mc(opc, 2): \
        s.bus.a = (s.bus.d << 8) | (s.aux & 0xff); \
        break; \
    case mc(opc, 3): \
        s.bus.d = s.a & s.x & ((s.bus.a + 0x100) >> 8); \
        s.bus.a = (s.aux & 0x100) \
            ? ((s.bus.d << 8) | (s.bus.a & 0xff)) \
            : s.bus.a; \
        s.bus(RW::w); \
        break; \
    case mc(opc, 4): \
        s.bus.a = s.pc; \
        s.bus(RW::r); \
        Op{s}.schedule(OPC::dispatch); \
        break; \
}


static constexpr MOS6502::u16 mc(MOS6502::u16 opc, MOS6502::u8 cn = 0) { // micro-code label
    return (opc << 3) | (cn & 0b111);
}


void MOS6502::Core::tick() {
    static constexpr u8 nmi_timer_handled = 0b10000000;

    if (s.irq_timer || s.nmi_timer) {
        if (s.nmi_timer == 0x01) s.nmi_timer = 0x02;
        else if (s.nmi_timer == 0x02) {
            s.nmi_act = true;
            s.nmi_timer = 0x03;
        }

        s.irq_timer <<= 1;
        if (s.irq_timer & 0b100) {
            s.irq_timer = 0;
            s.irq_act = true;
        }
    }

    switch (s.mcc++) {
         m_brk(0x00);                            // brk
        ie_izx(0x01, Op{s}.ora());               // ora izx
         m_hlt(0x02);                            // hlt
        ud_izx(0x03, Op{s}.ud_slo());            // slo izx
          ie_z(0x04, );                          // nop zp
          ie_z(0x05, Op{s}.ora());               // ora zp
         rmw_z(0x06, Op{s}.asl(s.bus.d));        // asl zp
         rmw_z(0x07, Op{s}.ud_slo());            // slo zp
         m_phr(0x08, s.p | (Flag::B | Flag::u)); // php
          ie_i(0x09, Op{s}.ora());               // ora imm
            sb(0x0a, Op{s}.asl(s.a));            // asl
          ie_i(0x0b, Op{s}.ud_anc());            // anc imm
          ie_a(0x0c, );                          // nop abs
          ie_a(0x0d, Op{s}.ora());               // ora abs
         rmw_a(0x0e, Op{s}.asl(s.bus.d));        // asl abs
         rmw_a(0x0f, Op{s}.ud_slo());            // slo abs
         m_brc(0x10, Flag::N);                   // bpl
        ie_izy(0x11, Op{s}.ora());               // ora izy
         m_hlt(0x12);                            // hlt
        ud_izy(0x13, Op{s}.ud_slo());            // slo izy
         ie_zi(0x14, s.x, );                     // nop zpx
         ie_zi(0x15, s.x, Op{s}.ora());          // ora zpx
        rmw_zx(0x16, Op{s}.asl(s.bus.d));        // asl zpx
        rmw_zx(0x17, Op{s}.ud_slo());            // slo zpx
            sb(0x18, s.clr(Flag::C));            // clc
         ie_ai(0x19, s.y, Op{s}.ora());          // ora absy
            sb(0x1a, );                          // nop
        rmw_ai(0x1b, s.y, Op{s}.ud_slo());       // slo absy
         ie_ai(0x1c, s.x, );                     // nop absx
         ie_ai(0x1d, s.x, Op{s}.ora());          // ora absx
        rmw_ai(0x1e, s.x, Op{s}.asl(s.bus.d));   // asl absx
        rmw_ai(0x1f, s.x, Op{s}.ud_slo());       // slo absx
          m_js(0x20);                            // jsr
        ie_izx(0x21, Op{s}.and_());              // and izx
         m_hlt(0x22);                            // hlt
        ud_izx(0x23, Op{s}.ud_rla());            // rla izx
          ie_z(0x24, Op{s}.bit());               // bit zp
          ie_z(0x25, Op{s}.and_());              // and zp
         rmw_z(0x26, Op{s}.rol(s.bus.d));        // rol zp
         rmw_z(0x27, Op{s}.ud_rla());            // rla zp
         m_plr(0x28, s.p = s.bus.d);             // plp
          ie_i(0x29, Op{s}.and_());              // and imm
            sb(0x2a, Op{s}.rol(s.a));            // rol
          ie_i(0x2b, Op{s}.ud_anc());            // anc imm
          ie_a(0x2c, Op{s}.bit());               // bit abs
          ie_a(0x2d, Op{s}.and_());              // and abs
         rmw_a(0x2e, Op{s}.rol(s.bus.d));        // rol abs
         rmw_a(0x2f, Op{s}.ud_rla());            // rla abs
         m_brs(0x30, Flag::N);                   // bmi
        ie_izy(0x31, Op{s}.and_());              // and izy
         m_hlt(0x32);                            // hlt
        ud_izy(0x33, Op{s}.ud_rla());            // rla izy
         ie_zi(0x34, s.x, );                     // nop zpx
         ie_zi(0x35, s.x, Op{s}.and_());         // and zpx
        rmw_zx(0x36, Op{s}.rol(s.bus.d));        // rol zpx
        rmw_zx(0x37, Op{s}.ud_rla());            // rla zpx
            sb(0x38, s.set(Flag::C));            // sec
         ie_ai(0x39, s.y, Op{s}.and_());         // and absy
            sb(0x3a, );                          // nop
        rmw_ai(0x3b, s.y, Op{s}.ud_rla());       // rla absy
         ie_ai(0x3c, s.x, );                     // nop absx
         ie_ai(0x3d, s.x, Op{s}.and_());         // and absx
        rmw_ai(0x3e, s.x, Op{s}.rol(s.bus.d));   // rol absx
        rmw_ai(0x3f, s.x, Op{s}.ud_rla());       // rla absx
         m_rti(0x40);                            // rti
        ie_izx(0x41, Op{s}.eor());               // eor izx
         m_hlt(0x42);                            // hlt
        ud_izx(0x43, Op{s}.ud_sre());            // sre izx
          ie_z(0x44, );                          // nop zp
          ie_z(0x45, Op{s}.eor());               // eor zp
         rmw_z(0x46, Op{s}.lsr(s.bus.d));        // lsr zp
         rmw_z(0x47, Op{s}.ud_sre());            // sre zp
         m_phr(0x48, s.a);                       // pha
          ie_i(0x49, Op{s}.eor());               // eor imm
            sb(0x4a, Op{s}.lsr(s.a));            // lsr
          ie_i(0x4b, Op{s}.ud_alr());            // alr imm
          m_ja(0x4c);                            // jmp abs
          ie_a(0x4d, Op{s}.eor());               // eor abs
         rmw_a(0x4e, Op{s}.lsr(s.bus.d));        // lsr abs
         rmw_a(0x4f, Op{s}.ud_sre());            // sre abs
         m_brc(0x50, Flag::V);                   // bvc
        ie_izy(0x51, Op{s}.eor());               // eor izy
         m_hlt(0x52);                            // hlt
        ud_izy(0x53, Op{s}.ud_sre());            // sre izy
         ie_zi(0x54, s.x, );                     // nop zpx
         ie_zi(0x55, s.x, Op{s}.eor());          // eor zpx
        rmw_zx(0x56, Op{s}.lsr(s.bus.d));        // lsr zpx
        rmw_zx(0x57, Op{s}.ud_sre());            // sre zpx
         m_sch(0x58, OPC::dispatch_post_cli);    // cli
         ie_ai(0x59, s.y, Op{s}.eor());          // eor absy
            sb(0x5a, );                          // nop
        rmw_ai(0x5b, s.y, Op{s}.ud_sre());       // sre absy
         ie_ai(0x5c, s.x, );                     // nop absx
         ie_ai(0x5d, s.x, Op{s}.eor());          // eor absx
        rmw_ai(0x5e, s.x, Op{s}.lsr(s.bus.d));   // lsr absx
        rmw_ai(0x5f, s.x, Op{s}.ud_sre());       // sre absx
         m_rts(0x60);                            // rts
        ie_izx(0x61, Op{s}.adc());               // adc izx
         m_hlt(0x62);                            // hlt
        ud_izx(0x63, Op{s}.ud_rra());            // rra izx
          ie_z(0x64, );                          // nop zp
          ie_z(0x65, Op{s}.adc());               // adc zp
         rmw_z(0x66, Op{s}.ror(s.bus.d));        // ror zp
         rmw_z(0x67, Op{s}.ud_rra());            // rra zp
         m_plr(0x68, Op{s}.ldr(s.a));            // pla
          ie_i(0x69, Op{s}.adc());               // adc imm
            sb(0x6a, Op{s}.ror(s.a));            // ror
          ie_i(0x6b, Op{s}.ud_arr());            // arr imm
          m_ji(0x6c);                            // jmp ind
          ie_a(0x6d, Op{s}.adc());               // adc abs
         rmw_a(0x6e, Op{s}.ror(s.bus.d));        // ror abs
         rmw_a(0x6f, Op{s}.ud_rra());            // rra abs
         m_brs(0x70, Flag::V);                   // bvs
        ie_izy(0x71, Op{s}.adc());               // adc izy
         m_hlt(0x72);                            // hlt
        ud_izy(0x73, Op{s}.ud_rra());            // rra izy
         ie_zi(0x74, s.x, );                     // nop zpx
         ie_zi(0x75, s.x, Op{s}.adc());          // adc zpx
        rmw_zx(0x76, Op{s}.ror(s.bus.d));        // ror zpx
        rmw_zx(0x77, Op{s}.ud_rra());            // rra zpx
         m_sch(0x78, OPC::dispatch_post_sei);    // sei
         ie_ai(0x79, s.y, Op{s}.adc());          // adc absy
            sb(0x7a, );                          // nop
        rmw_ai(0x7b, s.y, Op{s}.ud_rra());       // rra absy
         ie_ai(0x7c, s.x, );                     // nop absx
         ie_ai(0x7d, s.x, Op{s}.adc());          // adc absx
        rmw_ai(0x7e, s.x, Op{s}.ror(s.bus.d));   // ror absx
        rmw_ai(0x7f, s.x, Op{s}.ud_rra());       // rra absx
          ie_i(0x80, );                          // nop imm
        st_izx(0x81, s.a);                       // sta izx
          ie_i(0x82, );                          // nop imm
        st_izx(0x83, s.a & s.x);                 // sax izx
          st_z(0x84, s.y);                       // sty zp
          st_z(0x85, s.a);                       // sta zp
          st_z(0x86, s.x);                       // stx zp
          st_z(0x87, s.a & s.x);                 // sax zp
            sb(0x88, Op{s}.dec(s.y));            // dey
          ie_i(0x89, );                          // nop imm
            sb(0x8a, Op{s}.trr(s.x, s.a));       // txa
          ie_i(0x8b, Op{s}.ud_xaa());            // xaa imm
          st_a(0x8c, s.y);                       // sty abs
          st_a(0x8d, s.a);                       // sta abs
          st_a(0x8e, s.x);                       // stx abs
          st_a(0x8f, s.a & s.x);                 // sax abs
         m_brc(0x90, Flag::C);                   // bcc
        st_izy(0x91);                            // sta izy
         m_hlt(0x92);                            // hlt
        ud_ahx(0x93);                            // ahx izy
         st_zi(0x94, s.x, s.y);                  // sty zpx
         st_zi(0x95, s.x, s.a);                  // sta zpx
         st_zi(0x96, s.y, s.x);                  // stx zpy
         st_zi(0x97, s.y, s.a & s.x);            // sax zpy
            sb(0x98, Op{s}.trr(s.y, s.a));       // tya
         st_ai(0x99, s.y);                       // sta absy
            sb(0x9a, s.sp = sp(s.x));            // txs
         ud_ai(0x9b, s.y, s.x, Op{s}.ud_tas());  // tas absy
         ud_ai(0x9c, s.x, s.y, );                // shy absx
         st_ai(0x9d, s.x);                       // sta absx
         ud_ai(0x9e, s.y, s.x, );                // shx absy
         ud_ai(0x9f, s.y, s.x, s.bus.d &= s.a);  // ahx absy
          ie_i(0xa0, Op{s}.ldr(s.y));            // ldy imm
        ie_izx(0xa1, Op{s}.ldr(s.a));            // lda izx
          ie_i(0xa2, Op{s}.ldr(s.x));            // ldx imm
        ie_izx(0xa3, Op{s}.lax());               // lax izx
          ie_z(0xa4, Op{s}.ldr(s.y));            // ldy zp
          ie_z(0xa5, Op{s}.ldr(s.a));            // lda zp
          ie_z(0xa6, Op{s}.ldr(s.x));            // ldx zp
          ie_z(0xa7, Op{s}.lax());               // lax zp
            sb(0xa8, Op{s}.trr(s.a, s.y));       // tay
          ie_i(0xa9, Op{s}.ldr(s.a));            // lda imm
            sb(0xaa, Op{s}.trr(s.a, s.x));       // tax
          ie_i(0xab, Op{s}.ud_lxa());            // lxa imm
          ie_a(0xac, Op{s}.ldr(s.y));            // ldy abs
          ie_a(0xad, Op{s}.ldr(s.a));            // lda abs
          ie_a(0xae, Op{s}.ldr(s.x));            // ldx abs
          ie_a(0xaf, Op{s}.lax());               // lax abs
         m_brs(0xb0, Flag::C);                   // bcs
        ie_izy(0xb1, Op{s}.ldr(s.a));            // lda izy
         m_hlt(0xb2);                            // hlt
        ie_izy(0xb3, Op{s}.lax());               // lax izy
         ie_zi(0xb4, s.x, Op{s}.ldr(s.y));       // ldy zpx
         ie_zi(0xb5, s.x, Op{s}.ldr(s.a));       // lda zpx
         ie_zi(0xb6, s.y, Op{s}.ldr(s.x));       // ldx zpy
         ie_zi(0xb7, s.y, Op{s}.lax());          // lax zpy
            sb(0xb8, s.clr(Flag::V));            // clv
         ie_ai(0xb9, s.y, Op{s}.ldr(s.a));       // lda absy
            sb(0xba, Op{s}.trr(u8(s.sp), s.x));  // tsx
         ie_ai(0xbb, s.y, Op{s}.ud_las());       // las absy
         ie_ai(0xbc, s.x, Op{s}.ldr(s.y));       // ldy absx
         ie_ai(0xbd, s.x, Op{s}.ldr(s.a));       // lda absx
         ie_ai(0xbe, s.y, Op{s}.ldr(s.x));       // ldx absy
         ie_ai(0xbf, s.y, Op{s}.lax());          // lax absy
          ie_i(0xc0, Op{s}.cmp(s.y));            // cpy imm
        ie_izx(0xc1, Op{s}.cmp(s.a));            // cmp izx
          ie_i(0xc2, );                          // nop imm
        ud_izx(0xc3, Op{s}.ud_dcp());            // dcp izx
          ie_z(0xc4, Op{s}.cmp(s.y));            // cpy zp
          ie_z(0xc5, Op{s}.cmp(s.a));            // cmp zp
         rmw_z(0xc6, Op{s}.dec(s.bus.d));        // dec zp
         rmw_z(0xc7, Op{s}.ud_dcp());            // dcp zp
            sb(0xc8, Op{s}.inc(s.y));            // iny
          ie_i(0xc9, Op{s}.cmp(s.a));            // cmp imm
            sb(0xca, Op{s}.dec(s.x));            // dex
          ie_i(0xcb, Op{s}.ud_axs());            // axs imm
          ie_a(0xcc, Op{s}.cmp(s.y));            // cpy abs
          ie_a(0xcd, Op{s}.cmp(s.a));            // cmp abs
         rmw_a(0xce, Op{s}.dec(s.bus.d));        // dec abs
         rmw_a(0xcf, Op{s}.ud_dcp());            // dcp abs
         m_brc(0xd0, Flag::Z);                   // bne
        ie_izy(0xd1, Op{s}.cmp(s.a));            // cmp izy
         m_hlt(0xd2);                            // hlt
        ud_izy(0xd3, Op{s}.ud_dcp());            // dcp izy
         ie_zi(0xd4, s.x, );                     // nop zpx
         ie_zi(0xd5, s.x, Op{s}.cmp(s.a));       // cmp zpx
        rmw_zx(0xd6, Op{s}.dec(s.bus.d));        // dec zpx
        rmw_zx(0xd7, Op{s}.ud_dcp());            // dcp zpx
            sb(0xd8, s.clr(Flag::D));            // cld
         ie_ai(0xd9, s.y, Op{s}.cmp(s.a));       // cmp absy
            sb(0xda, );                          // nop
        rmw_ai(0xdb, s.y, Op{s}.ud_dcp());       // dcp absy
         ie_ai(0xdc, s.x, );                     // nop absx
         ie_ai(0xdd, s.x, Op{s}.cmp(s.a));       // cmp absx
        rmw_ai(0xde, s.x, Op{s}.dec(s.bus.d));   // dec absx
        rmw_ai(0xdf, s.x, Op{s}.ud_dcp());       // dcp absx
          ie_i(0xe0, Op{s}.cmp(s.x));            // cpx imm
        ie_izx(0xe1, Op{s}.sbc());               // sbc izx
          ie_i(0xe2, );                          // nop imm
        ud_izx(0xe3, Op{s}.ud_isc());            // isc izx
          ie_z(0xe4, Op{s}.cmp(s.x));            // cpx zp
          ie_z(0xe5, Op{s}.sbc());               // sbc zp
         rmw_z(0xe6, Op{s}.inc(s.bus.d));        // inc zp
         rmw_z(0xe7, Op{s}.ud_isc());            // isc zp
            sb(0xe8, Op{s}.inc(s.x));            // inx
          ie_i(0xe9, Op{s}.sbc());               // sbc imm
            sb(0xea, );                          // nop
          ie_i(0xeb, Op{s}.sbc());               // sbc imm
          ie_a(0xec, Op{s}.cmp(s.x));            // cpx abs
          ie_a(0xed, Op{s}.sbc());               // sbc abs
         rmw_a(0xee, Op{s}.inc(s.bus.d));        // inc abs
         rmw_a(0xef, Op{s}.ud_isc());            // isc abs
         m_brs(0xf0, Flag::Z);                   // beq
        ie_izy(0xf1, Op{s}.sbc());               // sbc izy
         m_hlt(0xf2);                            // hlt
        ud_izy(0xf3, Op{s}.ud_isc());            // isc izy
         ie_zi(0xf4, s.x, );                     // nop zpx
         ie_zi(0xf5, s.x, Op{s}.sbc());          // sbc zpx
        rmw_zx(0xf6, Op{s}.inc(s.bus.d));        // inc zpx
        rmw_zx(0xf7, Op{s}.ud_isc());            // isc zpx
            sb(0xf8, s.set(Flag::D));            // sed
         ie_ai(0xf9, s.y, Op{s}.sbc());          // sbc absy
            sb(0xfa, );                          // nop
        rmw_ai(0xfb, s.y, Op{s}.ud_isc());       // isc absy
         ie_ai(0xfc, s.x, );                     // nop absx
         ie_ai(0xfd, s.x, Op{s}.sbc());          // sbc absx
        rmw_ai(0xfe, s.x, Op{s}.inc(s.bus.d));   // inc absx
        rmw_ai(0xff, s.x, Op{s}.ud_isc());       // isc absx

        // ********************************************************************

        case mc(OPC::bra, 0): // bra, no page cross (hold ints)
            if (s.nmi_timer == 0x02) s.nmi_timer = 0x01;
            if (s.irq_timer & 0b11) s.irq_timer = 0b01;
            s.bus.a = s.pc;
            Op{s}.schedule(OPC::dispatch);
            break;
        case mc(OPC::bra, 1): // bra, page cross
            s.bus.a = s.aux;
            break;
        case mc(OPC::bra, 2):
            s.bus.a = s.pc;
            Op{s}.schedule(OPC::dispatch);
            break;

        // --------------------------------------------------------------------

        /* cli&sei feature: change not be visible at next T0 (dispatch)
            (http://visual6502.org/wiki/index.php?title=6502_Timing_of_Interrupt_Handling)
        */
        case mc(OPC::dispatch_post_cli): 
            Op{s}.check_irq(); // irq taken on 'old' i-flag value
            s.clr(Flag::I);
            goto check_nmi;
        case mc(OPC::dispatch_post_sei):
            Op{s}.check_irq(); // irq taken on 'old' i-flag value
            s.set(Flag::I);
            goto check_nmi;
        case mc(OPC::dispatch): // 'normal' T0 case (all interrupts taken normally)
            Op{s}.check_irq();

            check_nmi:
            if (s.nmi_act) {
                s.brk_srcs |= Brk_src::nmi;
                s.nmi_act = false;
                if (s.nmi_timer) s.nmi_timer = nmi_timer_handled;
            }
            // fall through
        case mc(OPC::dispatch_post_brk):
            if (s.brk_srcs) {
                s.pc = s.bus.a;
                s.p &= ~Flag::B;
                Op{s}.schedule(OPC::brk);
            } else {
                s.bus.a += 1; // inc pc
                if (s.bus.d == OPC::brk) {
                    s.pc = s.bus.a + 1;
                    s.p |= Flag::B;
                    // s.brk_srcs = Brk_src::sw; // redundant...
                }
                Op{s}.schedule(OPC(s.bus.d));
            }
            break;

        // --------------------------------------------------------------------

        case mc(OPC::halt, 0):
            s.bus.a = 0xfffe;
            break;
        case mc(OPC::halt, 1):
            break;
        case mc(OPC::halt, 2):
            s.bus.a = 0xffff;
            sig_halt(s.aux & 0xff, s.aux >> 8);
            break;
        case mc(OPC::halt, 3):
            s.mcc--; // stuck (until resume())
            break;

        // --------------------------------------------------------------------

        case mc(OPC::reset, 0): s.bus.a += 1; break;
        case mc(OPC::reset, 1): s.bus.a = 0x0100; break;
        case mc(OPC::reset, 2): s.bus.a = 0x01ff; break;
        case mc(OPC::reset, 3): s.bus.a = 0x01fe; break;
        case mc(OPC::reset, 4): s.bus.a = Vec::rst; s.sp = 0x01fd; break;
        case mc(OPC::reset, 5):
            s.aux = s.bus.d;
            s.bus.a += 1;
            break;
        case mc(OPC::reset, 6):
            s.bus.a = s.aux | (s.bus.d << 8);
            s.set(Flag::I);
            Op{s}.schedule(OPC::dispatch_post_brk);
            break;
    }
}


void MOS6502::Core::resume() {
    if (halted()) {
        s.bus.a = s.pc;
        Op{s}.schedule(OPC::dispatch);
    }
}
