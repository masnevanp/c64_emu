
#include "nmos6502_core.h"


using RW = NMOS6502::Core::State::Bus::RW;


namespace NMOS6502 {
    enum Brk_src : u8 { sw = 0b001, nmi = 0b010, irq = 0b100, };

    static u16 zp(u16 a) { return a & 0x00ff; }
    static u16 sp(u16 a) { return (a & 0xff) | 0x0100; }

    struct Op {
        Core::State& s;

        void schedule(u16 opc) { s.mcc = opc << 3; } // bits 0-2 encode the step (0..7)

        void set_nz(const u8& res) { s.set(Flag::N, res & 0x80); s.set(Flag::Z, res == 0x00); }

        u8 C() const { return s.p & Flag::C; } // carry
        u8 B() const { return C() ^ 1; } // borrow

        void bra(bool cond) {
            s.bus.a += 1;

            if (cond) {
                s.pc = s.bus.a + i8(s.bus.d);
                schedule(OPC::bra);
                if ((s.pc ^ s.bus.a) & 0xff00) { // page cross?
                    s.aux = s.bus.a + s.bus.d;
                    s.mcc += 1; // schedule 'page cross' step
                }
            } else {
                schedule(OPC::dispatch);
            }
        }
        void asl(u8& d) { s.set(Flag::C, d & 0x80); set_nz(d <<= 1); }
        void dec(u8& d) { set_nz(--d); }
        void inc(u8& d) { set_nz(++d); }
        void lsr(u8& d) { s.clr(Flag::N); s.set(Flag::C, d & 0x01); s.set(Flag::Z, !(d >>= 1)); }
        void rol(u8& d) {
            u8 old_c = s.p & Flag::C;
            s.set(Flag::C, d & 0x80);
            d <<= 1;
            set_nz(d |= old_c);
        }
        void ror(u8& d) {
            u8 old_c = s.p << 7;
            s.set(Flag::C, d & 0x01);
            d >>= 1;
            set_nz(d |= old_c);
        }

        // TODO: check if we can use s.bus.d directly everywhere?
        void bit(u8& d) { s.p = (s.p & 0x3f) | (d & 0xc0); s.set(Flag::Z, (s.a & d) == 0x00); }
        void cmp(const u8& r) { s.set(Flag::C, r >= s.bus.d); set_nz(r - s.bus.d); }
        void adc();
        void sbc();

        void ud_anc(u8& d) { set_nz(s.a &= d); s.set(Flag::C, s.a & 0x80); }
        void ud_arr();
        void ud_axs(u8& d) { s.x &= s.a; s.set(Flag::C, s.x >= d); set_nz(s.x -= d); }
        void ud_dcp(u8& d) { --d; cmp(s.a); }
        void ud_isc(u8& d) { ++d; sbc(); }
        void ud_las(u8& d) { set_nz(s.a = s.x = (s.sp & d)); s.sp = sp(s.a); }
        void ud_lxa(u8& d) { set_nz(s.a = s.x = (s.a | 0xee) & d); }
        void ud_rla(u8& d) { rol(d); set_nz(s.a &= d); }
        void ud_rra(u8& d) { ror(d); adc(); }
        void ud_slo(u8& d) { s.set(Flag::C, d & 0x80); set_nz(s.a |= (d <<= 1)); }
        void ud_sre(u8& d) { s.set(Flag::C, d & 0x01); set_nz(s.a ^= (d >>= 1)); }
        //void ud_tas(u8& d) { s.sp = sp(s.a & s.x); s.bus.d = s.sp & (s.a1h + 0x01); }
        void ud_xaa(u8& d) { set_nz(s.a = (s.a | 0xee) & s.x & d); }

        void halt() {
            s.aux = s.bus.d;
            s.pc = s.bus.a + 1;
            s.bus.a = 0xffff;
            schedule(OPC::halt);
        }
    };

    void Op::adc() {
        if (s.is_set(Flag::D)) {
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
        if (s.is_set(Flag::D)) {
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
        if (s.is_set(Flag::D)) {
            s.bus.d &= s.a;

            u8 ah = s.bus.d >> 4;
            u8 al = s.bus.d & 0xf;

            s.set(Flag::N, s.is_set(Flag::C));
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
}


NMOS6502::Core::Core(State& s_, Sig& sig_halt_) : s(s_), sig_halt(sig_halt_) {}


void NMOS6502::Core::reset() {
    s.brk_srcs = 0;
    s.nmi_timer = s.irq_timer = 0;
    s.nmi_act = s.irq_act = false;
    s.bus(RW::r);
    Op{s}.schedule(OPC::reset);
}


void NMOS6502::Core::set_nmi(bool act) {
    if (act) {
        if (s.nmi_timer == 0x00) s.nmi_timer = 0x01;
    } else {
        if (s.nmi_timer == 0x02) s.nmi_act = true;
        s.nmi_timer = 0x00;
    }
}

void NMOS6502::Core::set_irq(bool act) {
    if (act) { s.irq_timer |= 0b1; }
    else s.irq_timer = s.irq_act = 0x00;
}


static constexpr NMOS6502::u16 mc(NMOS6502::u16 opc, NMOS6502::u8 cn = 0) { // micro-code label
    return (opc << 3) | (cn & 0b111);
}


void NMOS6502::Core::exec_cycle() {
    static constexpr u8 nmi_timer_handled = 0b10000000;

    auto set_nz = [&](u8 res) { Op{s}.set_nz(res); };

    auto check_irq = [&]() { if (s.irq_act && s.is_clr(Flag::I)) s.brk_srcs |= Brk_src::irq; };

    auto read_pcl = [&]() { s.pc = s.bus.d; s.bus.a += 1; };
    auto read_pch = [&]() { s.pc |= (s.bus.d << 8); s.bus.a = s.pc; };

    auto schedule = [&](OPC opc) { Op{s}.schedule(opc); };

    // bra if set
    #define brs(opc, flag) { \
        case mc(opc, 0): \
            Op{s}.bra(s.is_set(flag)); \
            break; \
    }
    // bra if clr
    #define brc(opc, flag) { \
        case mc(opc, 0): \
            Op{s}.bra(s.is_clr(flag)); \
            break; \
    }

    #define hlt(opc) { \
        case mc(opc, 0): \
            Op{s}.halt(); \
            break; \
    }

    // ******** Single Byte ********
    #define sb(opc, op) { \
        case mc(opc, 0): \
            op; \
            schedule(OPC::dispatch); \
            break; \
    }

    // ******** Read & Modify -operations (internal exec on mem data) ********
    #define rm_i(opc, op) { \
        case mc(opc, 0): \
            op; \
            s.bus.a += 1; \
            schedule(OPC::dispatch); \
            break; \
    }

    #define rm_z(opc, op) { \
        case mc(opc, 0): \
            s.pc = s.bus.a + 1; \
            s.bus.a = s.bus.d; \
            break; \
        case mc(opc, 1): \
            op; \
            s.bus.a = s.pc; \
            schedule(OPC::dispatch); \
            break; \
    }

    #define rm_a(opc, op) { \
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
            schedule(OPC::dispatch); \
            break; \
    }

    #define rm_izx(opc, op) { \
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
            schedule(OPC::dispatch); \
            break; \
    }

    #define rm_ai(opc, ireg, op) { \
        case mc(opc, 0): \
            s.aux = s.bus.d; \
            s.pc = s.bus.a + 2; \
            s.bus.a += 1; \
            break; \
        case mc(opc, 1): \
            s.bus.a = (s.aux | (s.bus.d << 8)) + ireg; \
            if ((s.bus.a & 0xff) < ireg) { \
                s.bus.a -= 0x100; \
                s.mcc += 1; \
            } \
            break; \
        case mc(opc, 2): \
            op; \
            s.bus.a = s.pc; \
            schedule(OPC::dispatch); \
            break; \
        case mc(opc, 3): \
            s.bus.a += 0x100; \
            break; \
        case mc(opc, 4): \
            op; \
            s.bus.a = s.pc; \
            schedule(OPC::dispatch); \
            break; \
    }

    // ******** Store Operations ********
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
            schedule(OPC::dispatch); \
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
            schedule(OPC::dispatch); \
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
            schedule(OPC::dispatch); \
            break; \
    }

    #define st_ai(opc, ireg) { \
        case mc(opc, 0): \
            s.aux = s.bus.d; \
            s.pc = s.bus.a + 2; \
            s.bus.a += 1; \
            break; \
        case mc(opc, 1): \
            s.bus.a = (s.aux | (s.bus.d << 8)) + ireg; \
            break; \
        case mc(opc, 2): \
            s.bus.d = s.a; \
            s.bus(RW::w); \
            break; \
        case mc(opc, 3): \
            s.bus.a = s.pc; \
            s.bus(RW::r); \
            schedule(OPC::dispatch); \
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
            schedule(OPC::dispatch); \
            break; \
    }

    #define st_izy(opc) { \
        case mc(opc, 0): \
            s.pc = s.bus.a + 1; \
            s.bus.a = s.bus.d; \
            break; \
        case mc(opc, 1): \
            s.aux = s.bus.d; \
            s.bus.a += 1; \
            break; \
        case mc(opc, 2): \
            s.bus.a = (s.aux | (s.bus.d << 8)) + s.y; \
            break; \
        case mc(opc, 3): \
            s.bus.d = s.a; \
            s.bus(RW::w); \
            break; \
        case mc(opc, 4): \
            s.bus.a = s.pc; \
            s.bus(RW::r); \
            schedule(OPC::dispatch); \
            break; \
    }

    // ******** Read & Modify & Write -operations ********

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
            schedule(OPC::dispatch); \
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
            schedule(OPC::dispatch); \
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
            schedule(OPC::dispatch); \
            break; \
    }

    #define rmw_ax(opc, op) { \
        case mc(opc, 0): \
            s.aux = s.bus.d; \
            s.pc = s.bus.a + 2; \
            s.bus.a += 1; \
            break; \
        case mc(opc, 1): \
            s.bus.a = (s.aux | (s.bus.d << 8)) + s.x; \
            break; \
        case mc(opc, 2): \
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
            schedule(OPC::dispatch); \
            break; \
    }

    switch (s.mcc++) {
        /* Interrupt hijacking:
            - happens if an interrupt of a higher priority is signalled before
                the 'brk' execution of the lower priority reaches T5 (=T4 ph2 at latest).
            - T0&T1 PC, and pushed b-flag according to the former (lower prio)
            - vector address according to the latter (higher prio)
        */
        case mc(OPC::brk, 0):
            s.pc = s.bus.a;
            if (s.brk_srcs == Brk_src::sw) s.pc += 1;
            s.bus(s.sp, (s.pc >> 8), RW::w);
            break;
        case mc(OPC::brk, 1):
            s.bus(sp(s.bus.a - 1), s.pc);
            break;
        case mc(OPC::brk, 2): {
            const auto p = s.p | Flag::u | (s.brk_srcs == Brk_src::sw ? Flag::B : 0x00);
            s.bus(sp(s.bus.a - 1), p);
            s.sp = sp(s.bus.a - 1);
            break; }
        case mc(OPC::brk, 3):
            if (s.nmi_timer & 0b11) { // Potential hijacking by nmi
                s.brk_srcs = Brk_src::nmi;
                s.nmi_timer = nmi_timer_handled;
            }
            s.bus.a = (s.brk_srcs & Brk_src::nmi) ? Vec::nmi : Vec::irq;
            s.brk_srcs = 0;
            s.bus(RW::r);
            break;
        case mc(OPC::brk, 4):
            read_pcl();
            break;
        case mc(OPC::brk, 5):
            read_pch();
            s.set(Flag::I);
            schedule(OPC::dispatch_post_brk);
            break;

        rm_izx(0x01, set_nz(s.a |= s.bus.d)); // ora izx
        hlt(0x02);
        rmw_z(0x06, Op{s}.asl(s.bus.d)); // asl zp
        rmw_z(0x07, Op{s}.ud_slo(s.bus.d)); // slo zp

        case mc(0x08, 0): // php
            s.pc = s.bus.a;
            s.bus(s.sp, s.p | (Flag::B | Flag::u), RW::w);
            s.sp = sp(s.sp - 1);
            break;
        case mc(0x08, 1):
            s.bus(s.pc, RW::r);
            schedule(OPC::dispatch);
            break;

        rm_i(0x09, set_nz(s.a |= s.bus.d)); // ora imm
        sb(0x0a, Op{s}.asl(s.a)); // asl
        rm_a(0x0c, ); // nop abs
        rm_a(0x0d, set_nz(s.a |= s.bus.d)); // ora abs
        rmw_a(0x0e, Op{s}.asl(s.bus.d)); // asl abs
        rmw_a(0x0f, Op{s}.ud_slo(s.bus.d)); // slo abs
        brc(0x10, Flag::N); // bpl
        hlt(0x12);
        rmw_zx(0x16, Op{s}.asl(s.bus.d)); // asl zpx
        rmw_zx(0x17, Op{s}.ud_slo(s.bus.d)); // slo zpx
        sb(0x18, s.clr(Flag::C)); // clc
        rm_ai(0x19, s.y, set_nz(s.a |= s.bus.d)); // ora absy
        rm_ai(0x1c, s.x, ); // nop absx
        rm_ai(0x1d, s.x, set_nz(s.a |= s.bus.d)); // ora absx
        rmw_ax(0x1e, Op{s}.asl(s.bus.d)); // asl absx
        rmw_ax(0x1f, Op{s}.ud_slo(s.bus.d)); // slo absx

        case mc(0x20, 0): // jsr
            s.aux = s.bus.d;
            s.pc = s.bus.a + 1;
            s.bus.a = s.sp;
            s.sp = sp(s.sp - 2);
            break;
        case mc(0x20, 1):
            s.bus.d = (s.pc >> 8);
            s.bus(RW::w);
            break;
        case mc(0x20, 2):
            s.bus(sp(s.bus.a - 1), u8(s.pc));
            break;
        case mc(0x20, 3):
            s.bus.a = s.pc;
            s.bus(RW::r);
            break;
        case mc(0x20, 4):
            s.bus.a = (s.aux | (s.bus.d << 8));
            schedule(OPC::dispatch);
            break;

        rm_izx(0x21, set_nz(s.a &= s.bus.d)); // and izx
        hlt(0x22);
        rmw_z(0x26, Op{s}.rol(s.bus.d)); // rol zp
        rmw_z(0x27, Op{s}.ud_rla(s.bus.d)); // rla zp

        case mc(0x28, 0): // plp
            s.pc = s.bus.a;
            s.bus.a = s.sp;
            break;
        case mc(0x28, 1):
            s.sp = sp(s.sp + 1);
            s.bus.a = s.sp;
            break;
        case mc(0x28, 2):
            s.p = s.bus.d;
            s.bus.a = s.pc;
            schedule(OPC::dispatch);
            break;

        rm_i(0x29, set_nz(s.a &= s.bus.d)); // and imm
        sb(0x2a, Op{s}.rol(s.a)); // rol
        rm_a(0x2c, Op{s}.bit(s.bus.d)); // bit abs
        rm_a(0x2d, set_nz(s.a &= s.bus.d)); // and abs
        rmw_a(0x2e, Op{s}.rol(s.bus.d)); // rol abs
        rmw_a(0x2f, Op{s}.ud_rla(s.bus.d)); // rla abs
        brs(0x30, Flag::N); // bmi
        hlt(0x32);
        rmw_zx(0x36, Op{s}.rol(s.bus.d)); // rol zpx
        rmw_zx(0x37, Op{s}.ud_rla(s.bus.d)); // rla zpx
        sb(0x38, s.set(Flag::C)); // sec
        rm_ai(0x39, s.y, set_nz(s.a &= s.bus.d)); // and absy
        sb(0x3a,); // nop
        rm_ai(0x3c, s.x,); // nop absx
        rm_ai(0x3d, s.x, set_nz(s.a &= s.bus.d)); // and absx
        rmw_ax(0x3e, Op{s}.rol(s.bus.d)); // rol absx
        rmw_ax(0x3f, Op{s}.ud_rla(s.bus.d)); // rla absx

        case mc(0x40, 0): // rti
            s.bus.a = s.sp;
            s.sp = sp(s.sp + 3);
            break;
        case mc(0x40, 1):
            s.bus.a = sp(s.bus.a + 1);
            break;
        case mc(0x40, 2):
            s.p = s.bus.d;
            s.bus.a = sp(s.bus.a + 1);
            break;
        case mc(0x40, 3):
            s.pc = s.bus.d;
            s.bus.a = sp(s.bus.a + 1);
            break;
        case mc(0x40, 4):
            s.bus.a = s.pc | (s.bus.d << 8);
            schedule(OPC::dispatch);
            break;

        rm_izx(0x41, set_nz(s.a ^= s.bus.d)); // eor izx
        hlt(0x42);
        rmw_z(0x46, Op{s}.lsr(s.bus.d)); // lsr zp
        rmw_z(0x47, Op{s}.ud_sre(s.bus.d)); // sre zp

        case mc(0x48, 0): // pha
            s.pc = s.bus.a;
            s.bus(s.sp, s.a, RW::w);
            s.sp = sp(s.sp - 1);
            break;
        case mc(0x48, 1):
            s.bus(s.pc, RW::r);
            schedule(OPC::dispatch);
            break;

        rm_i(0x49, set_nz(s.a ^= s.bus.d)); // eor imm
        sb(0x4a, Op{s}.lsr(s.a)); // lsr
        rm_a(0x4d, set_nz(s.a ^= s.bus.d)); // eor abs
        rmw_a(0x4e, Op{s}.lsr(s.bus.d)); // lsr abs
        rmw_a(0x4f, Op{s}.ud_sre(s.bus.d)); // sre abs

        case mc(0x4c, 0): // jmp abs
            read_pcl();
            break;
        case mc(0x4c, 1):
            read_pch();
            schedule(OPC::dispatch);
            break;

        brc(0x50, Flag::V); // bvc
        hlt(0x52);
        rmw_zx(0x56, Op{s}.lsr(s.bus.d)); // lsr zpx
        rmw_zx(0x57, Op{s}.ud_sre(s.bus.d)); // sre zpx

        case mc(0x58, 0): // cli
            schedule(OPC::dispatch_post_cli);
            break;

        rm_ai(0x59, s.y, set_nz(s.a ^= s.bus.d)); // eor absy
        sb(0x5a,); // nop
        rm_ai(0x5c, s.x,); // nop absx
        rm_ai(0x5d, s.x, set_nz(s.a ^= s.bus.d)); // eor absx
        rmw_ax(0x5e, Op{s}.lsr(s.bus.d)); // lsr absx
        rmw_ax(0x5f, Op{s}.ud_sre(s.bus.d)); // sre absx

        case mc(0x60, 0): // rts
            s.bus.a = s.sp;
            s.sp = sp(s.sp + 2);
            break;
        case mc(0x60, 1):
            s.bus.a = sp(s.bus.a + 1);
            break;
        case mc(0x60, 2):
            s.pc = s.bus.d;
            s.bus.a = sp(s.bus.a + 1);
            break;
        case mc(0x60, 3):
            s.bus.a = s.pc | (s.bus.d << 8);
            break;
        case mc(0x60, 4):
            s.bus.a += 1; // pc + 1
            schedule(OPC::dispatch);
            break;

        rm_izx(0x61, Op{s}.adc()); // adc izx
        hlt(0x62);
        rmw_z(0x66, Op{s}.ror(s.bus.d)); // ror zp
        rmw_z(0x67, Op{s}.ud_rra(s.bus.d)); // rra zp

        case mc(0x68, 0): // pla
            s.pc = s.bus.a;
            s.bus.a = s.sp;
            break;
        case mc(0x68, 1):
            s.sp = sp(s.sp + 1);
            s.bus.a = s.sp;
            break;
        case mc(0x68, 2):
            set_nz(s.a = s.bus.d);
            s.bus.a = s.pc;
            schedule(OPC::dispatch);
            break;

        rm_i(0x69, Op{s}.adc()); // adc imm
        sb(0x6a, Op{s}.ror(s.a)); // ror
        rm_a(0x6d, Op{s}.adc()); // adc abs
        rmw_a(0x6e, Op{s}.ror(s.bus.d)); // ror abs
        rmw_a(0x6f, Op{s}.ud_rra(s.bus.d)); // rra abs

        case mc(0x6c, 0): // jmp ind
            read_pcl();
            break;
        case mc(0x6c, 1):
            read_pch();
            break;
        case mc(0x6c, 2):
            s.pc = s.bus.d;
            s.bus.a = (s.bus.a & 0xff00) | ((s.bus.a + 1) & 0xff); // the 'missing carry propagation' feature
            break;
        case mc(0x6c, 3):
            read_pch();
            schedule(OPC::dispatch);
            break;

        brs(0x70, Flag::V); // bvs
        hlt(0x72);
        rmw_zx(0x76, Op{s}.ror(s.bus.d)); // ror zpx
        rmw_zx(0x77, Op{s}.ud_rra(s.bus.d)); // rra zpx

        case mc(0x78, 0): // sei
            schedule(OPC::dispatch_post_sei);
            break;

        rm_ai(0x79, s.y, Op{s}.adc()); // adc absy
        rm_ai(0x7c, s.x,); // nop absx
        rm_ai(0x7d, s.x, Op{s}.adc()); // adc absx
        rmw_ax(0x7e, Op{s}.ror(s.bus.d)); // ror absx
        rmw_ax(0x7f, Op{s}.ud_rra(s.bus.d)); // rra absx
        st_izx(0x81, s.a); // sta izx
        st_izx(0x83, s.a & s.x); // sax izx
        st_z(0x84, s.y); // sty zp
        st_z(0x85, s.a); // sta zp
        st_z(0x86, s.x); // stx zp
        st_z(0x87, s.a & s.x); // sax zp
        sb(0x88, set_nz(--s.y)); // dey
        sb(0x8a, set_nz(s.a = s.x)); // txa
        st_a(0x8c, s.y); // sty abs
        st_a(0x8d, s.a); // sta abs
        st_a(0x8e, s.x); // stx abs
        st_a(0x8f, s.a & s.x); // sax abs
        brc(0x90, Flag::C); // bcc
        st_izy(0x91); // sta izy
        hlt(0x92);
        st_zi(0x94, s.x, s.y); // sty zpx
        st_zi(0x95, s.x, s.a); // sta zpx
        st_zi(0x96, s.y, s.x); // stx zpy
        st_zi(0x97, s.y, s.a & s.x); // sax zpy
        sb(0x98, set_nz(s.a = s.y)); // tya
        st_ai(0x99, s.y); // sta absy
        sb(0x9a, s.sp = sp(s.x)); // txs
        st_ai(0x9d, s.x); // sta absx
        rm_i(0xa0, set_nz(s.y = s.bus.d)); // ldy imm
        rm_izx(0xa1, set_nz(s.a = s.bus.d)); // lda izx
        rm_i(0xa2, set_nz(s.x = s.bus.d)); // ldx imm
        rm_izx(0xa3, set_nz(s.a = s.x = s.bus.d)); // lax izx
        rm_z(0xa4, set_nz(s.y = s.bus.d)); // ldy zp
        rm_z(0xa5, set_nz(s.a = s.bus.d)); // lda zp
        rm_z(0xa6, set_nz(s.x = s.bus.d)); // ldx zp
        sb(0xa8, set_nz(s.y = s.a)); // tay
        rm_i(0xa9, set_nz(s.a = s.bus.d)); // lda imm
        sb(0xaa, set_nz(s.x = s.a)); // tax
        rm_a(0xac, set_nz(s.y = s.bus.d)); // ldy abs
        rm_a(0xad, set_nz(s.a = s.bus.d)); // lda abs
        rm_a(0xae, set_nz(s.x = s.bus.d)); // ldx abs
        rm_a(0xaf, set_nz(s.a = s.x = s.bus.d)); // lax abs
        brs(0xb0, Flag::C); // bcs
        hlt(0xb2);
        sb(0xb8, s.clr(Flag::V)); // clv
        sb(0xba, set_nz(s.x = u8(s.sp))); // tsx
        rm_ai(0xbb, s.y, Op{s}.ud_las(s.bus.d)); // las absy
        rm_ai(0xbc, s.x, set_nz(s.y = s.bus.d)); // ldy absx
        rm_ai(0xbd, s.x, set_nz(s.a = s.bus.d)); // lda absx
        rm_ai(0xbe, s.y, set_nz(s.x = s.bus.d)); // ldx absy
        rm_ai(0xbf, s.y, set_nz(s.a = s.x = s.bus.d)); // lax absy
        rm_i(0xc0, Op{s}.cmp(s.y)); // cpy imm
        rm_izx(0xc1, Op{s}.cmp(s.a)); // cmp izx
        rmw_z(0xc6, Op{s}.dec(s.bus.d)); // dec zp
        rmw_z(0xc7, Op{s}.ud_dcp(s.bus.d)); // dcp zp
        sb(0xc8, set_nz(++s.y)); // iny
        rm_i(0xc9, Op{s}.cmp(s.a)); // cmp imm
        sb(0xca, set_nz(--s.x)); // dex
        rm_a(0xcc, Op{s}.cmp(s.y)); // cpy abs
        rm_a(0xcd, Op{s}.cmp(s.a)); // cmp abs
        rmw_a(0xce, Op{s}.dec(s.bus.d)); // dec abs
        rmw_a(0xcf, Op{s}.ud_dcp(s.bus.d)); // dcp abs
        brc(0xd0, Flag::Z); // bne
        hlt(0xd2);
        rmw_zx(0xd6, Op{s}.dec(s.bus.d)); // dec zpx
        rmw_zx(0xd7, Op{s}.ud_dcp(s.bus.d)); // dcp zpx
        sb(0xd8, s.clr(Flag::D)); // cld
        rm_ai(0xd9, s.y, Op{s}.cmp(s.a)); // cmp absy
        rm_ai(0xdc, s.x,); // nop absx
        rm_ai(0xdd, s.x, Op{s}.cmp(s.a)); // cmp absx
        rmw_ax(0xde, Op{s}.dec(s.bus.d)); // dec absx
        rmw_ax(0xdf, Op{s}.ud_dcp(s.bus.d)); // dcp absx
        rm_i(0xe0, Op{s}.cmp(s.x)); // cpx imm
        rm_izx(0xe1, Op{s}.sbc()); // sbc izx
        rmw_z(0xe6, Op{s}.inc(s.bus.d)); // inc zp
        rmw_z(0xe7, Op{s}.ud_isc(s.bus.d)); // isc zp
        sb(0xe8, set_nz(++s.x)); // inx
        rm_i(0xe9, Op{s}.sbc()); // sbc imm
        sb(0xea,); // nop
        rm_a(0xec, Op{s}.cmp(s.x)); // cpx abs
        rm_a(0xed, Op{s}.sbc()); // sbc abs
        rmw_a(0xee, Op{s}.inc(s.bus.d)); // inc abs
        rmw_a(0xef, Op{s}.ud_isc(s.bus.d)); // isc abs
        brs(0xf0, Flag::Z); // beq
        hlt(0xf2);
        rmw_zx(0xf6, Op{s}.inc(s.bus.d)); // inc zpx
        rmw_zx(0xf7, Op{s}.ud_isc(s.bus.d)); // isc zpx
        sb(0xf8, s.set(Flag::D)); // sed
        rm_ai(0xf9, s.y, Op{s}.sbc()); // sbc absy
        rm_ai(0xfc, s.x,); // nop absx
        rm_ai(0xfd, s.x, Op{s}.sbc()); // sbc absx
        rmw_ax(0xfe, Op{s}.inc(s.bus.d)); // inc absx
        rmw_ax(0xff, Op{s}.ud_isc(s.bus.d)); // isc absx

        // ********************************************************************

        case mc(OPC::bra, 0): // bra, no page cross (hold ints)
            if (s.nmi_timer == 0x02) s.nmi_timer = 0x01;
            if (s.irq_timer == 0b10) s.irq_timer = 0b01;
            s.bus.a = s.pc;
            schedule(OPC::dispatch);
            break;
        case mc(OPC::bra, 1): // bra, page cross
            s.bus.a = s.aux;
            break;
        case mc(OPC::bra, 2):
            s.bus.a = s.pc;
            schedule(OPC::dispatch);
            break;

        // --------------------------------------------------------------------

        /* cli&sei feature: change not be visible at next T0 (dispatch)
            (http://visual6502.org/wiki/index.php?title=6502_Timing_of_Interrupt_Handling)
        */
        case mc(OPC::dispatch_post_cli): 
            check_irq(); // irq taken on 'old' i-flag value
            s.clr(Flag::I);
            goto IRQ_checked;
        case mc(OPC::dispatch_post_sei):
            check_irq(); // irq taken on 'old' i-flag value
            s.set(Flag::I);
            goto IRQ_checked;
        case mc(OPC::dispatch): // 'normal' T0 case (all interrupts taken normally)
            check_irq();

            IRQ_checked:
            if (s.nmi_act) {
                s.brk_srcs |= Brk_src::nmi;
                s.nmi_act = false;
                if (s.nmi_timer) s.nmi_timer = nmi_timer_handled;
            }
            // fall through
        case mc(OPC::dispatch_post_brk):
            if (s.brk_srcs) {
                schedule(OPC::brk);
            } else {
                s.bus.a += 1; // inc pc
                if (s.bus.d == OPC::brk) s.brk_srcs = Brk_src::sw;
                schedule(OPC(s.bus.d));
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
            sig_halt();
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
            read_pcl();
            break;
        case mc(OPC::reset, 6):
            read_pch();
            s.set(Flag::I);
            schedule(OPC::dispatch_post_brk);
            break;
    }

    // TODO: add missing undefs
    #undef halt
    #undef rm_i
    #undef rm_z
}


void NMOS6502::Core::resume() {
    if (halted()) {
        s.bus.a = s.pc;
        Op{s}.schedule(OPC::dispatch);
    }
}
