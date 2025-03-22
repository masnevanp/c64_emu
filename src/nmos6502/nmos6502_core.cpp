
#include "nmos6502_core.h"


void NMOS6502::Core::reset_warm() {
    s.brk_src = 0x00;
    s.nmi_req = s.irq_req = 0x00;
    s.nmi_bit = s.irq_bit = 0x00;
    s.mcc = MC::opc_addr[OPC::reset];
    s.a1 = s.sp; --s.sp.l;
    s.a2 = s.sp; --s.sp.l;
    s.a3 = s.sp; --s.sp.l;
    s.a4 = Vec::rst;
}


void NMOS6502::Core::reset_cold() {
    s.zpa = 0x0000;
    s.sp.h = 0x01;
    reset_warm();
}


void NMOS6502::Core::exec(const MC::MOPC mopc) {
    using namespace NMOS6502::MC;

    static constexpr u8 NMI_taken = 0x80;

    // silence compiler (we do handle all the cases)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wswitch"

    switch (mopc) {
        case abs_x: s.a2 = s.a1 + s.x; s.a1.l += s.x; return;
        case inc_zpa: ++s.zpa.l; return;
        case abs_y: s.a2 = s.a1 + s.y; s.a1.l += s.y; return;
        case rm_zp_x: s.zpa.l += s.x; return;
        case rm_zp_y: s.zpa.l += s.y; return;
        case rm_x:
            s.a1.l += s.x;
            if (s.a1.l < s.x) { s.a2 = s.a1 + u16(0x0100); s.mcc += 2; }
            return;
        case rm_y:
            s.a1.l += s.y;
            if (s.a1.l < s.y) { s.a2 = s.a1 + u16(0x0100); s.mcc += 2; }
            return;
        case rm_idx_ind: s.zpa.l += s.x; s.a2 = (s.zpa.l + 0x01) & 0xff; return;
        case a_nz: set_nz(s.a); return;
        case do_op: switch (s.ir) {
            // case 0x00: case 0x02: case 0x04: case 0x08: case 0x0c: case 0x10: case 0x12: case 0x14: case 0x1a: case 0x1c: case 0x20: case 0x22: case 0x28: case 0x29: case 0x30: case 0x32: case 0x34: case 0x3a: case 0x3c: case 0x40: case 0x42: case 0x44: case 0x48: case 0x4c: case 0x50: case 0x52: case 0x54: case 0x58: case 0x5a: case 0x5c: case 0x60: case 0x62: case 0x64: case 0x68: case 0x6c: case 0x70: case 0x72: case 0x74: case 0x78: case 0x7a: case 0x7c: case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85: case 0x86: case 0x87: case 0x89: case 0x8c: case 0x8d: case 0x8e: case 0x8f: case 0x90: case 0x91: case 0x92: case 0x94: case 0x95: case 0x96: case 0x97: case 0x99: case 0x9d: case 0xa5: case 0xad: case 0xb0: case 0xb2: case 0xbd: case 0xc2: case 0xd0: case 0xd2: case 0xd4: case 0xda: case 0xdc: case 0xe2: case 0xea: case 0xf0: case 0xf2: case 0xf4: case 0xfa: case 0xfc:  return; /* nop */
            case 0x01: case 0x05: case 0x09: case 0x0d: case 0x11: case 0x15:
            case 0x19: case 0x1d: set_nz(s.a|=s.d); return; /* rm_ora */
            case 0x03: case 0x07: case 0x0f: case 0x13: case 0x17: case 0x1b:
            case 0x1f: do_ud_slo(); return; /* ud_slo */
            case 0x06: case 0x0e: case 0x16: case 0x1e: do_asl(s.d); return; /* rmw_asl */
            case 0x0a: do_asl(s.a); return; /* sb_asl */
            case 0x0b: case 0x2b: do_ud_anc(); return; /* ud_anc */
            case 0x18: clr(Flag::C); return; /* sb_clc */
            case 0x21: case 0x25: case 0x29: case 0x2d: case 0x31: case 0x35:
            case 0x39: case 0x3d: set_nz(s.a&=s.d); return; /* rm_and */
            case 0x23: case 0x27: case 0x2f: case 0x33: case 0x37: case 0x3b:
            case 0x3f: do_ud_rla(); return; /* ud_rla */
            case 0x24: case 0x2c: do_bit(); return; /* rm_bit */
            case 0x26: case 0x2e: case 0x36: case 0x3e: do_rol(s.d); return; /* rmw_rol */
            case 0x2a: do_rol(s.a); return; /* sb_rol */
            case 0x38: set(Flag::C); return; /* sb_sec */
            case 0x41: case 0x45: case 0x49: case 0x4d: case 0x51: case 0x55:
            case 0x59: case 0x5d: set_nz(s.a^=s.d); return; /* rm_eor */
            case 0x43: case 0x47: case 0x4f: case 0x53: case 0x57: case 0x5b:
            case 0x5f: do_ud_sre(); return; /* ud_sre */
            case 0x46: case 0x4e: case 0x56: case 0x5e: do_lsr(s.d); return; /* rmw_lsr */
            case 0x4a: do_lsr(s.a); return; /* sb_lsr */
            case 0x4b: s.a&=s.d;do_lsr(s.a); return; /* ud_alr */
            case 0x61: case 0x65: case 0x69: case 0x6d: case 0x71: case 0x75:
            case 0x79: case 0x7d: do_adc(); return; /* rm_adc */
            case 0x63: case 0x67: case 0x6f: case 0x73: case 0x77: case 0x7b:
            case 0x7f: do_ud_rra(); return; /* ud_rra */
            case 0x66: case 0x6e: case 0x76: case 0x7e: do_ror(s.d); return; /* rmw_ror */
            case 0x6a: do_ror(s.a); return; /* sb_ror */
            case 0x6b: do_ud_arr(); return; /* ud_arr */
            case 0x88: set_nz(--s.y); return; /* sb_dey */
            case 0x8a: set_nz(s.a=s.x); return; /* sb_txa */
            case 0x8b: do_ud_xaa(); return; /* ud_xaa */
            case 0x93: case 0x9f: s.d=s.a&s.x&(s.a1.h+1); return; /* ud_ahx */
            case 0x98: set_nz(s.a=s.y); return; /* sb_tya */
            case 0x9a: s.sp.l=s.x; return; /* sb_txs */
            case 0x9b: do_ud_tas(); return; /* ud_tas */
            case 0x9c: s.d=s.y&(s.a1.h+1); return; /* ud_shy */
            case 0x9e: s.d=s.x&(s.a1.h+1); return; /* ud_shx */
            case 0xa0: case 0xa4: case 0xac: case 0xb4: case 0xbc: set_nz(s.y=s.d); return; /* rm_ldy */
            case 0xa1: case 0xa5: case 0xa9: case 0xad: case 0xb1: case 0xb5:
            case 0xb9: case 0xbd: set_nz(s.a=s.d); return; /* rm_lda */
            case 0xa2: case 0xa6: case 0xae: case 0xb6: case 0xbe: set_nz(s.x=s.d); return; /* rm_ldx */
            case 0xa3: case 0xa7: case 0xaf: case 0xb3: case 0xb7: case 0xbf:
                set_nz(s.a=s.x=s.d); return; /* ud_lax */
            case 0xa8: set_nz(s.y=s.a); return; /* sb_tay */
            case 0xaa: set_nz(s.x=s.a); return; /* sb_tax */
            case 0xab: do_ud_lxa(); return; /* ud_lxa */
            case 0xb8: clr(Flag::V); return; /* sb_clv */
            case 0xba: set_nz(s.x=s.sp.l); return; /* sb_tsx */
            case 0xbb: do_ud_las(); return; /* ud_las */
            case 0xc0: case 0xc4: case 0xcc: do_cmp(s.y); return; /* rm_cpy */
            case 0xc1: case 0xc5: case 0xc9: case 0xcd: case 0xd1: case 0xd5:
            case 0xd9: case 0xdd: do_cmp(s.a); return; /* rm_cmp */
            case 0xc3: case 0xc7: case 0xcf: case 0xd3: case 0xd7: case 0xdb:
            case 0xdf: s.d--;do_cmp(s.a); return; /* ud_dcp */
            case 0xc6: case 0xce: case 0xd6: case 0xde: set_nz(--s.d); return; /* rmw_dec */
            case 0xc8: set_nz(++s.y); return; /* sb_iny */
            case 0xca: set_nz(--s.x); return; /* sb_dex */
            case 0xcb: do_ud_axs(); return; /* ud_axs */
            case 0xd8: clr(Flag::D); return; /* sb_cld */
            case 0xe0: case 0xe4: case 0xec: do_cmp(s.x); return; /* rm_cpx */
            case 0xe1: case 0xe5: case 0xe9: case 0xeb: case 0xed: case 0xf1:
            case 0xf5: case 0xf9: case 0xfd: do_sbc(); return; /* rm_sbc */
            case 0xe3: case 0xe7: case 0xef: case 0xf3: case 0xf7: case 0xfb:
            case 0xff: s.d++;do_sbc(); return; /* ud_isc */
            case 0xe6: case 0xee: case 0xf6: case 0xfe: set_nz(++s.d); return; /* rmw_inc */
            case 0xe8: set_nz(++s.x); return; /* sb_inx */
            case 0xf8: set(Flag::D); return; /* sb_sed */
        } return;
        case st_zp_x: s.zpa.l += s.x; st_reg_sel(); return;
        case st_zp_y: s.zpa.l += s.y; st_reg_sel(); return;
        case st_idx_ind: s.zpa.l += s.x; s.a2 = (s.zpa.l + 0x01) & 0xff; // fall through
        case st_reg: st_reg_sel(); return;
        case jmp_ind: ++s.a1.l; return;
        case bpl: if (is_set(Flag::N)) { s.mcc += 7; /*T2 (no bra)*/ return; } else goto bra_take;
        case bmi: if (is_clr(Flag::N)) { s.mcc += 6; /*T2 (no bra)*/ return; } else goto bra_take;
        case bvc: if (is_set(Flag::V)) { s.mcc += 5; /*T2 (no bra)*/ return; } else goto bra_take;
        case bvs: if (is_clr(Flag::V)) { s.mcc += 4; /*T2 (no bra)*/ return; } else goto bra_take;
        case bcc: if (is_set(Flag::C)) { s.mcc += 3; /*T2 (no bra)*/ return; } else goto bra_take;
        case bcs: if (is_clr(Flag::C)) { s.mcc += 2; /*T2 (no bra)*/ return; } else goto bra_take;
        case beq: if (is_clr(Flag::Z)) { s.mcc += 1; /*T2 (no bra)*/ return; } else goto bra_take;
        case MOPC::bne:
                  if (is_set(Flag::Z)) /*T2 (no bra)*/ return;
            // fall through
        bra_take:
            s.mcc = MC::opc_addr[OPC::bne] + 2; // T2 (no page cross)
            s.a2 = s.pc;
            s.pc = u16(u16(s.pc) + i8(s.d));
            if ((s.a2 ^ s.pc) & 0xff00) {
                s.mcc += 2; // T2 (page cross)
                s.a1 = s.a2;
                s.a1.l += s.d;
            }
            return;
        case hold_ints:
            if (s.nmi_req == 0x02) s.nmi_req = 0x01;
            if (s.irq_req && (s.irq_req < 0x04)) s.irq_req = 0x01;
            return;
        case php: s.p |= (Flag::B | Flag::u); // fall through
        case pha: s.a1 = s.sp; --s.sp.l; return;
        case jsr: s.a3 = s.sp; --s.sp.l; s.a4 = s.sp; --s.sp.l; s.a2 = s.pc; // fall through
        case jmp_abs: s.pc = s.a1; return;
        case rti: ++s.sp.l; s.a1 = s.sp; // fall through
        case rts: ++s.sp.l; s.a2 = s.sp; // fall through
        case inc_sp: ++s.sp.l; return;
        case MOPC::brk:
            // brk_src |= (irq_bit & ~p); // no effect (the same vector used anyway)
            if (s.nmi_req & 0x3) { // Potential hijacking by nmi
                s.brk_src |= NMI_BIT;
                s.nmi_req = NMI_taken;
            }
            s.a2 = brk_ctrl[s.brk_src].vec;
            s.a3 = s.a2 + u16(0x0001);
            s.brk_src = 0x00;
            set(Flag::I);
            return;
        case dispatch_cli: // post cli dispatch
            s.brk_src |= (s.irq_bit & ~s.p);       // bit 2
            clr(Flag::I);
            goto dispatch_post_cli_sei;
        case dispatch_sei: // post sei dispatch
            s.brk_src |= (s.irq_bit & ~s.p);       // bit 2
            set(Flag::I);
            goto dispatch_post_cli_sei;
        case dispatch: // 'normal' T0 case (all interrupts taken normally)
            /* Interrupt hijacking:
               - happens if an interrupt of a higher priority is signalled before
                 the 'brk' execution of the lower priority reaches T5 (=T4 ph2 at latest).
               - T0&T1 PC, and pushed b-flag according to the former (lower prio)
               - vector address according to the latter (higher prio)
            */
            s.brk_src |= (s.irq_bit & ~s.p);   // bit 2
        dispatch_post_cli_sei: // irq has been taken based on 'old' i-flag value (still can hijack a sw brk!)
            if (s.nmi_bit) {
                s.brk_src |= s.nmi_bit;      // bit 1
                s.nmi_bit = 0x00;
                if (s.nmi_req) s.nmi_req = NMI_taken;
            }
            // fall through
        case dispatch_brk: // hw-ints not taken on the instruction following a brk (sw or hw)
            s.brk_src |= (s.ir == OPC::brk); // bit 0

            if (s.brk_src) {
                s.ir = 0x00;
                s.p = (s.p | (Flag::B | Flag::u)) & brk_ctrl[s.brk_src].p_mask;
                s.pc = s.pc - brk_ctrl[s.brk_src].pc_t0;
                s.a1 = s.pc + brk_ctrl[s.brk_src].pc_t1;
                s.a2 = s.sp; --s.sp.l;
                s.a3 = s.sp; --s.sp.l;
                s.a4 = s.sp; --s.sp.l;
            }

            s.mcc = MC::opc_addr[s.ir];

            return;
        case sig_hlt: sig_halt(); return;
        case hlt: --s.mcc; return; // stuck
        case MOPC::reset: s.a1 = u16(Vec::rst + 0x0001); set(Flag::I); return;
    }

    #pragma GCC diagnostic push
}


void NMOS6502::Core::do_adc() {
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
        set(Flag::Z, ((s.a + s.d + C()) & 0xff) == 0x00); // Z set based on binary result
        u8 n1; u8 c1; u8 n2; u8 c2;
        n1 = adc_dec_nib((s.a & 0xf) + (s.d & 0xf) + C(), c1);
        n2 = (s.a >> 4) + (s.d >> 4) + c1;
        u8 r = (n2 << 4) | n1; // high nib not adjusted yet (N&V set based on this)
        set(Flag::N, r & 0x80);
        set(Flag::V, ((s.a ^ r) & (s.d ^ r) & 0x80));
        n2 = adc_dec_nib(n2, c2);
        set(Flag::C, c2); // decimal carry
        s.a = (n2 << 4) | n1;
    } else {
        const u16 r = s.a + s.d + C();
        set(Flag::C, r & 0x100);
        set(Flag::V, ((s.a ^ r) & (s.d ^ r) & 0x80));
        set_nz(s.a = r);
    }
}


void NMOS6502::Core::do_sbc() {
    if (is_set(Flag::D)) {
        auto sbc_dec_nib = [](u8 res, u8& co) -> u8 {
            if (res > 0xf) { co = 0x1; return (res - 0x6) & 0xf; }
            else { co = 0x0; return res; }
        };
        set(Flag::Z, ((s.a - s.d - B()) & 0xff) == 0x00);
        u8 n1; u8 c1; u8 n2; u8 c2;
        n1 = sbc_dec_nib((s.a & 0xf) - (s.d & 0xf) - B(), c1);
        n2 = (s.a >> 4) - (s.d >> 4) - c1;
        u8 r = (n2 << 4) | n1;
        set(Flag::N, r & 0x80);
        set(Flag::V, ((s.a ^ s.d) & (s.a ^ r)) & 0x80);
        n2 = sbc_dec_nib(n2, c2);
        set(Flag::C, !c2);
        s.a = (n2 << 4) | n1;
    } else {
        u16 r = s.a - s.d - B();
        set(Flag::C, r < 0x100);
        set(Flag::V, ((s.a ^ s.d) & (s.a ^ r)) & 0x80);
        set_nz(s.a = r);
    }
}


void NMOS6502::Core::do_ud_arr() {
    /* Totally based on c64doc.txt by John West & Marko Mäkelä.
        (http://www.6502.org/users/andre/petindex/local/64doc.txt)
    */
    if (is_set(Flag::D)) {
        s.d &= s.a;

        u8 ah = s.d >> 4;
        u8 al = s.d & 0xf;

        set(Flag::N, is_set(Flag::C));
        s.a = (s.d >> 1) | (s.p << 7);
        set(Flag::Z, s.a == 0x00);
        set(Flag::V, (s.d ^ s.a) & 0x40);

        if (al + (al & 0x1) > 0x5) s.a = (s.a & 0xf0) | ((s.a + 0x6) & 0xf);
        if ((ah + (ah & 0x1)) > 0x5) { set(Flag::C); s.a += 0x60; }
        else clr(Flag::C);
    } else {
        s.a &= s.d;
        s.a >>= 1;
        s.a |= (s.p << 7);
        set_nz(s.a);
        set(Flag::C, s.a & 0x40);
        set(Flag::V, (s.a ^ (s.a << 1)) & 0x40);
    }
}

