
#include "nmos6502_core.h"


using RW = NMOS6502::Core::State::Bus::RW;


namespace NMOS6502 {
    enum Brk_src : u8 { sw = 0b001, nmi = 0b010, irq = 0b100, };

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

        //void bit() { s.p = (s.p & 0x3f) | (s.d & 0xc0); s.set(Flag::Z, (s.a & s.d) == 0x00); }
        void cmp(const u8& r) { s.set(Flag::C, r >= s.bus.d); set_nz(r - s.bus.d); }
        void adc();
        void sbc();

        /*void ud_anc() { set_nz(s.a &= s.d); s.set(Flag::C, s.a & 0x80); }
        void ud_arr();
        void ud_axs() { s.x &= s.a; s.set(Flag::C, s.x >= s.d); set_nz(s.x -= s.d); }
        void ud_las() { set_nz(s.a = s.x = (s.sp & s.d)); s.set_sp(s.a); }
        void ud_lxa() { set_nz(s.a = s.x = (s.a | 0xee) & s.d); }
        void ud_rla() { rol(s.d); set_nz(s.a &= s.d); }
        void ud_rra() { ror(s.d); adc(); }
        void ud_slo() { s.set(Flag::C, s.d & 0x80); set_nz(s.a |= (s.d <<= 1)); }
        void ud_sre() { s.set(Flag::C, s.d & 0x01); set_nz(s.a ^= (s.d >>= 1)); }
        void ud_tas() { s.set_sp(s.a & s.x); s.d = s.sp & (s.a1h + 0x01); }
        void ud_xaa() { set_nz(s.a = (s.a | 0xee) & s.x & s.d); }

        void st_reg_sel() {
            switch (s.ir & 0x3) {
                case 0x0: s.d = s.y; return;
                case 0x1: s.d = s.a; return;
                case 0x2: s.d = s.x; return;
                case 0x3: s.d = s.a & s.x; return; // sax
            }
        }
        */
        void halt() {
            s.aux = s.bus.d;
            s.pc = s.bus.a + 1;
            s.bus.a = 0xffff;
            schedule(OPC::halt);
        }

        void op() {
            //switch (s.ir) {
                // case 0x00: case 0x02: case 0x04: case 0x08: case 0x0c: case 0x10: case 0x12: case 0x14: case 0x1a: case 0x1c: case 0x20: case 0x22: case 0x28: case 0x29: case 0x30: case 0x32: case 0x34: case 0x3a: case 0x3c: case 0x40: case 0x42: case 0x44: case 0x48: case 0x4c: case 0x50: case 0x52: case 0x54: case 0x58: case 0x5a: case 0x5c: case 0x60: case 0x62: case 0x64: case 0x68: case 0x6c: case 0x70: case 0x72: case 0x74: case 0x78: case 0x7a: case 0x7c: case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85: case 0x86: case 0x87: case 0x89: case 0x8c: case 0x8d: case 0x8e: case 0x8f: case 0x90: case 0x91: case 0x92: case 0x94: case 0x95: case 0x96: case 0x97: case 0x99: case 0x9d: case 0xa5: case 0xad: case 0xb0: case 0xb2: case 0xbd: case 0xc2: case 0xd0: case 0xd2: case 0xd4: case 0xda: case 0xdc: case 0xe2: case 0xea: case 0xf0: case 0xf2: case 0xf4: case 0xfa: case 0xfc:  return; /* nop */
                //case 0x01: case 0x05: case 0x09: case 0x0d: case 0x11: case 0x15:
                //case 0x19: case 0x1d: set_nz(s.a|=s.d); return; /* rm_ora */
                //case 0x03: case 0x07: case 0x0f: case 0x13: case 0x17: case 0x1b:
                //case 0x1f: ud_slo(); return; /* ud_slo */
                //case 0x06: case 0x0e: case 0x16: case 0x1e: asl(s.d); return; /* rmw_asl */
                //case 0x0a: asl(s.a); return; /* sb_asl */
                //case 0x0b: case 0x2b: ud_anc(); return; /* ud_anc */
                //case 0x18: s.clr(Flag::C); return; /* sb_clc */
                //case 0x21: case 0x25: case 0x29: case 0x2d: case 0x31: case 0x35:
                //case 0x39: case 0x3d: set_nz(s.a&=s.d); return; /* rm_and */
                //case 0x23: case 0x27: case 0x2f: case 0x33: case 0x37: case 0x3b:
                //case 0x3f: ud_rla(); return; /* ud_rla */
                //case 0x24: case 0x2c: bit(); return; /* rm_bit */
                //case 0x26: case 0x2e: case 0x36: case 0x3e: rol(s.d); return; /* rmw_rol */
                //case 0x2a: rol(s.a); return; /* sb_rol */
                //case 0x38: s.set(Flag::C); return; /* sb_sec */
                //case 0x41: case 0x45: case 0x49: case 0x4d: case 0x51: case 0x55:
                //case 0x59: case 0x5d: set_nz(s.a^=s.d); return; /* rm_eor */
                //case 0x43: case 0x47: case 0x4f: case 0x53: case 0x57: case 0x5b:
                //case 0x5f: ud_sre(); return; /* ud_sre */
                //case 0x46: case 0x4e: case 0x56: case 0x5e: lsr(s.d); return; /* rmw_lsr */
                //case 0x4a: lsr(s.a); return; /* sb_lsr */
                //case 0x4b: s.a&=s.d; lsr(s.a); return; /* ud_alr */
                //case 0x61: case 0x65: case 0x69: case 0x6d: case 0x71: case 0x75:
                //case 0x79: case 0x7d: adc(); return; /* rm_adc */
                //case 0x63: case 0x67: case 0x6f: case 0x73: case 0x77: case 0x7b:
                //case 0x7f: ud_rra(); return; /* ud_rra */
                //case 0x66: case 0x6e: case 0x76: case 0x7e: ror(s.d); return; /* rmw_ror */
                //case 0x6a: ror(s.a); return; /* sb_ror */
                //case 0x6b: ud_arr(); return; /* ud_arr */
                //case 0x88: set_nz(--s.y); return; /* sb_dey */
                //case 0x8a: set_nz(s.a=s.x); return; /* sb_txa */
                //case 0x8b: ud_xaa(); return; /* ud_xaa */
                //case 0x93: case 0x9f: s.d=s.a&s.x&(s.a1h+1); return; /* ud_ahx */
                //case 0x98: set_nz(s.a=s.y); return; /* sb_tya */
                //case 0x9a: s.set_sp(s.x); return; /* sb_txs */
                //case 0x9b: ud_tas(); return; /* ud_tas */
                //case 0x9c: s.d=s.y&(s.a1h+1); return; /* ud_shy */
                //case 0x9e: s.d=s.x&(s.a1h+1); return; /* ud_shx */
                //case 0xa0: case 0xa4: case 0xac: case 0xb4: case 0xbc: set_nz(s.y=s.d); return; /* rm_ldy */
                //case 0xa1: case 0xa5: case 0xa9: case 0xad: case 0xb1: case 0xb5:
                //case 0xb9: case 0xbd: set_nz(s.a=s.d); return; /* rm_lda */
                //case 0xa2: case 0xa6: case 0xae: case 0xb6: case 0xbe: set_nz(s.x=s.d); return; /* rm_ldx */
                //case 0xa3: case 0xa7: case 0xaf: case 0xb3: case 0xb7: case 0xbf:
                //    set_nz(s.a=s.x=s.d); return; /* ud_lax */
                //case 0xa8: set_nz(s.y=s.a); return; /* sb_tay */
                //case 0xaa: set_nz(s.x=s.a); return; /* sb_tax */
                //case 0xab: ud_lxa(); return; /* ud_lxa */
                //case 0xb8: s.clr(Flag::V); return; /* sb_clv */
                //case 0xba: set_nz(s.x=s.sp); return; /* sb_tsx */
                //case 0xbb: ud_las(); return; /* ud_las */
                //case 0xc0: case 0xc4: case 0xcc: cmp(s.y); return; /* rm_cpy */
                //case 0xc1: case 0xc5: case 0xc9: case 0xcd: case 0xd1: case 0xd5:
                //case 0xd9: case 0xdd: cmp(s.a); return; /* rm_cmp */
                //case 0xc3: case 0xc7: case 0xcf: case 0xd3: case 0xd7: case 0xdb:
                //case 0xdf: s.d--; cmp(s.a); return; /* ud_dcp */
                //case 0xc6: case 0xce: case 0xd6: case 0xde: set_nz(--s.d); return; /* rmw_dec */
                //case 0xc8: set_nz(++s.y); return; /* sb_iny */
                //case 0xca: set_nz(--s.x); return; /* sb_dex */
                //case 0xcb: ud_axs(); return; /* ud_axs */
                //case 0xd8: s.clr(Flag::D); return; /* sb_cld */
                //case 0xe0: case 0xe4: case 0xec: cmp(s.x); return; /* rm_cpx */
                //case 0xe1: case 0xe5: case 0xe9: case 0xeb: case 0xed: case 0xf1:
                //case 0xf5: case 0xf9: case 0xfd: sbc(); return; /* rm_sbc */
                //case 0xe3: case 0xe7: case 0xef: case 0xf3: case 0xf7: case 0xfb:
                //case 0xff: s.d++; sbc(); return; /* ud_isc */
                //case 0xe6: case 0xee: case 0xf6: case 0xfe: set_nz(++s.d); return; /* rmw_inc */
                //case 0xe8: set_nz(++s.x); return; /* sb_inx */
                //case 0xf8: s.set(Flag::D); return; /* sb_sed */
            //}
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

    //void S::ud_arr() {
        /* Totally based on c64doc.txt by John West & Marko Mäkelä.
            (http://www.6502.org/users/andre/petindex/local/64doc.txt)
        */
        /*
        if (s.is_set(Flag::D)) {
            s.d &= s.a;

            u8 ah = s.d >> 4;
            u8 al = s.d & 0xf;

            s.set(Flag::N, s.is_set(Flag::C));
            s.a = (s.d >> 1) | (s.p << 7);
            s.set(Flag::Z, s.a == 0x00);
            s.set(Flag::V, (s.d ^ s.a) & 0x40);

            if (al + (al & 0x1) > 0x5) s.a = (s.a & 0xf0) | ((s.a + 0x6) & 0xf);
            if ((ah + (ah & 0x1)) > 0x5) { s.set(Flag::C); s.a += 0x60; }
            else s.clr(Flag::C);
        } else {
            s.a &= s.d;
            s.a >>= 1;
            s.a |= (s.p << 7);
            set_nz(s.a);
            s.set(Flag::C, s.a & 0x40);
            s.set(Flag::V, (s.a ^ (s.a << 1)) & 0x40);
        }
        */
    //}
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

    #define set_flag(opc, flag) { \
        case mc(opc, 0): \
            s.set(flag); \
            schedule(OPC::dispatch); \
            break; \
    }

    #define clr_flag(opc, flag) { \
        case mc(opc, 0): \
            s.clr(flag); \
            schedule(OPC::dispatch); \
            break; \
    }

    #define bra_if_set(opc, flag) { \
        case mc(opc, 0): \
            Op{s}.bra(s.is_set(flag)); \
            break; \
    }

    #define bra_if_clr(opc, flag) { \
        case mc(opc, 0): \
            Op{s}.bra(s.is_clr(flag)); \
            break; \
    }

    #define halt(opc) { \
        case mc(opc, 0): \
            Op{s}.halt(); \
            break; \
    }

    // Read & Modify -operations (internal exec on mem data)
    #define rm_imm(opc, op) { \
        case mc(opc, 0): \
            op; \
            s.bus.a += 1; \
            schedule(OPC::dispatch); \
            break; \
    }

    #define rm_zp(opc, op) { \
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

    // Store Operations
    // Read & Modify & Write -operations

    switch (s.mcc++) {

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

        halt(0x02);

        case mc(0x08, 0): // php
            s.pc = s.bus.a;
            s.bus(s.sp, s.p | (Flag::B | Flag::u), RW::w);
            s.sp = sp(s.sp - 1);
            break;
        case mc(0x08, 1):
            s.bus(s.pc, RW::r);
            schedule(OPC::dispatch);
            break;

        rm_imm(0x09, set_nz(s.a |= s.bus.d)); // ora imm

        case mc(0x0a, 0): // asl
            Op{s}.asl(s.a);
            schedule(OPC::dispatch);
            break;

        bra_if_clr(0x10, Flag::N); // bpl

        halt(0x12);

        clr_flag(0x18, Flag::C); // clc

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

        halt(0x22);

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

        rm_imm(0x29, set_nz(s.a &= s.bus.d)); // and imm

        case mc(0x2a, 0): // rol
            Op{s}.rol(s.a);
            schedule(OPC::dispatch);
            break;

        bra_if_set(0x30, Flag::N); // bmi

        halt(0x32);

        set_flag(0x38, Flag::C); // sec

        case mc(0x3a, 0): // nop
            schedule(OPC::dispatch);
            break;

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

        halt(0x42);

        case mc(0x48, 0): // pha
            s.pc = s.bus.a;
            s.bus(s.sp, s.a, RW::w);
            s.sp = sp(s.sp - 1);
            break;
        case mc(0x48, 1):
            s.bus(s.pc, RW::r);
            schedule(OPC::dispatch);
            break;

        rm_imm(0x49, set_nz(s.a ^= s.bus.d)); // eor imm

        case mc(0x4a, 0): // lsr
            Op{s}.lsr(s.a);
            schedule(OPC::dispatch);
            break;

        case mc(0x4c, 0): // jmp abs
            read_pcl();
            break;
        case mc(0x4c, 1):
            read_pch();
            schedule(OPC::dispatch);
            break;

        bra_if_clr(0x50, Flag::V); // bvc

        halt(0x52);

        case mc(0x58, 0): // cli
            schedule(OPC::dispatch_post_cli);
            break;

        case mc(0x5a, 0): // nop
            schedule(OPC::dispatch);
            break;

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

        halt(0x62);

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

        rm_imm(0x69, Op{s}.adc()); // adc imm

        case mc(0x6a, 0): // ror
            Op{s}.ror(s.a);
            schedule(OPC::dispatch);
            break;

        case mc(0x6c, 0): // jmp ind
            read_pcl();
            break;
        case mc(0x6c, 1):
            read_pch();
            break;
        case mc(0x6c, 2):
            read_pcl();
            break;
        case mc(0x6c, 3):
            read_pch();
            schedule(OPC::dispatch);
            break;

        bra_if_set(0x70, Flag::V); // bvs

        halt(0x72);

        case mc(0x78, 0): // sei
            schedule(OPC::dispatch_post_sei);
            break;

        case mc(0x88, 0): // dey
            set_nz(--s.y);
            schedule(OPC::dispatch);
            break;

        case mc(0x8a, 0): // txa
            set_nz(s.a = s.x);
            schedule(OPC::dispatch);
            break;

        case mc(0x8d, 0): // todo
            break;
        case mc(0x8d, 1): // todo
            break;
        case mc(0x8d, 2): // todo
            s.bus.a += 2;
            schedule(OPC::dispatch);
            break;

        bra_if_clr(0x90, Flag::C); // bcc

        halt(0x92);

        case mc(0x98, 0): // tya
            set_nz(s.a = s.y);
            schedule(OPC::dispatch);
            break;

        case mc(0x9a, 0): // txs
            s.sp = sp(s.x);
            schedule(OPC::dispatch);
            break;

        rm_imm(0xa2, set_nz(s.x = s.bus.d));// ldx imm

        rm_zp(0xa4, set_nz(s.y = s.bus.d)); // ldy zp

        rm_zp(0xa5, set_nz(s.a = s.bus.d)); // lda zp

        rm_zp(0xa6, set_nz(s.x = s.bus.d)); // ldx zp

        case mc(0xa8, 0): // tay
            set_nz(s.y = s.a);
            schedule(OPC::dispatch);
            break;

        rm_imm(0xa9, set_nz(s.a = s.bus.d)); // lda imm

        case mc(0xaa, 0): // tax
            set_nz(s.x = s.a);
            schedule(OPC::dispatch);
            break;

        bra_if_set(0xb0, Flag::C); // bcs

        halt(0xb2);

        clr_flag(0xb8, Flag::V); // clv

        case mc(0xba, 0): // tsx
            set_nz(s.x = u8(s.sp));
            schedule(OPC::dispatch);
            break;

        rm_imm(0xc0, Op{s}.cmp(s.y)); // cpy imm

        case mc(0xc8, 0): // iny
            set_nz(++s.y);
            schedule(OPC::dispatch);
            break;

        rm_imm(0xc9, Op{s}.cmp(s.a)); // cmp imm

        case mc(0xca, 0): // dex
            set_nz(--s.x);
            schedule(OPC::dispatch);
            break;

        bra_if_clr(0xd0, Flag::Z); // bne

        halt(0xd2);

        clr_flag(0xd8, Flag::D); // cld

        rm_imm(0xe0, Op{s}.cmp(s.x)); // cpx imm

        case mc(0xe8, 0): // inx
            set_nz(++s.x);
            schedule(OPC::dispatch);
            break;

        rm_imm(0xe9, Op{s}.sbc()); // sbc imm

        case mc(0xea, 0): // nop
            schedule(OPC::dispatch);
            break;

        bra_if_set(0xf0, Flag::Z); // beq

        halt(0xf2);

        set_flag(0xf8, Flag::D); // sed

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

        /* Interrupt hijacking:
            - happens if an interrupt of a higher priority is signalled before
                the 'brk' execution of the lower priority reaches T5 (=T4 ph2 at latest).
            - T0&T1 PC, and pushed b-flag according to the former (lower prio)
            - vector address according to the latter (higher prio)
        */
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

    #undef set_flag
    #undef clr_flag
    #undef bra_if_set
    #undef bra_if_clr
    #undef halt
    #undef rm_imm
    #undef rm_zp
}


void NMOS6502::Core::resume() {
    if (halted()) {
        s.bus.a = s.pc;
        Op{s}.schedule(OPC::dispatch);
    }
}


/*
void NMOS6502::Core::exec(const u8 mopc) {
    using namespace NMOS6502::MC;

    static constexpr u8 nmi_timer_handled = 0b10000000;

    const auto a1 = [&]() -> Reg16& { return s.r16(Ri16::a1); };

    switch (mopc) {
        case abs_x: s.a2 = a1() + s.x; s.a1l += s.x; return;
        case inc_zp: ++s.zp; return;
        case abs_y: s.a2 = a1() + s.y; s.a1l += s.y; return;
        case rm_zp_x: s.zp += s.x; return;
        case rm_zp_y: s.zp += s.y; return;
        case rm_x:
            s.a1l += s.x;
            if (s.a1l < s.x) { s.a2 = a1() + 0x0100; s.mcc += 2; }
            return;
        case rm_y:
            s.a1l += s.y;
            if (s.a1l < s.y) { s.a2 = a1() + 0x0100; s.mcc += 2; }
            return;
        case rm_idx_ind: s.zp += s.x; s.a2 = (u8)(s.zp + 0x01); return;
        case a_nz: Op{s}.set_nz(s.a); return;
        case do_op: Op{s}.op(); return;
        case st_zp_x: s.zp += s.x; Op{s}.st_reg_sel(); return;
        case st_zp_y: s.zp += s.y; Op{s}.st_reg_sel(); return;
        case st_idx_ind: s.zp += s.x; s.a2 = (u8)(s.zp + 0x01); // fall through
        case st_reg: Op{s}.st_reg_sel(); return;
        case jmp_ind: ++s.a1l; return;
        case bra: {
            // mapping: condition code -> flag bit position
            static constexpr u8 cc_flag_pos[8] = { 7, 7, 6, 6, 0, 0, 1, 1 };

            const int cc = s.ir >> 5;
            const bool no_bra = ((s.p >> cc_flag_pos[cc]) ^ cc) & 0b1;

            if (!no_bra) {
                s.mcc += 1; // T2 (no page cross)
                s.a2 = s.pc;
                s.pc += (i8)s.d;
                if ((s.a2 ^ s.pc) & 0xff00) {
                    s.mcc += 2; // T2 (page cross)
                    a1() = s.a2;
                    s.a1l += s.d;
                }
            }
            return;
        }
        case hold_ints:
            if (s.nmi_timer == 0x02) s.nmi_timer = 0x01;
            if (s.irq_timer == 0b10) s.irq_timer = 0b01;
            return;
        case php: s.p |= (Flag::B | Flag::u); // fall through
        case pha: a1() = s.sp; s.dec_sp(); return;
        case jsr: s.a3 = s.sp; s.dec_sp(); s.a4 = s.sp; s.dec_sp(); s.a2 = s.pc; // fall through
        case jmp_abs: s.pc = a1(); return;
        case rti: s.inc_sp(); a1() = s.sp; // fall through
        case MOPC::rts: s.inc_sp(); s.a2 = s.sp; // fall through
        case inc_sp: s.inc_sp(); return;

        case dispatch_cli: // post cli dispatch
            Op{s}.check_irq();
            s.clr(Flag::I);
            goto dispatch_post_cli_sei;
        case dispatch_sei: // post sei dispatch
            Op{s}.check_irq();
            s.set(Flag::I);
            goto dispatch_post_cli_sei;
        case sig_hlt: sig_halt(); return;
        case hlt: --s.mcc; return; // stuck
            return;
    }
}
*/