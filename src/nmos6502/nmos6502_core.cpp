
#include "nmos6502_core.h"


NMOS6502::Core::Core(Sig& sig_halt_) :
    r8((Reg8*)r16),
    pc(r16[R16::pc]), spf(r16[R16::spf]), zpaf(r16[R16::zpaf]),
    a1(r16[R16::a1]), a2(r16[R16::a2]), a3(r16[R16::a3]), a4(r16[R16::a4]),
    pcl(r8[R8::pcl]), pch(r8[R8::pch]), sp(r8[R8::sp]), p(r8[R8::p]), a(r8[R8::a]), x(r8[R8::x]), y(r8[R8::y]),
    d(r8[R8::d]), ir(r8[R8::ir]), zpa(r8[R8::zpa]), a1l(r8[R8::a1l]), a1h(r8[R8::a1h]),
    sig_halt(sig_halt_)
{
    reset_cold();
}


void NMOS6502::Core::reset_warm() {
    brk_src = 0x00;
    nmi_req = irq_req = 0x00;
    nmi_bit = irq_bit = 0x00;
    mcp = MC::OPC_MC[0x100];
    a1 = spf; --sp; a2 = spf; --sp; a3 = spf; --sp;
    a4 = Vec::rst;
}


void NMOS6502::Core::reset_cold() {
    zpaf = 0x0000;
    r8[R8::sph] = 0x01;
    reset_warm();
}


void NMOS6502::Core::exec(const u8 mop) {
    using namespace NMOS6502::MC;

    switch (mop) {
        case abs_x: a2 = a1 + x; a1l += x; return;
        case inc_zpa: ++zpa; return;
        case abs_y: a2 = a1 + y; a1l += y; return;
        case rm_zp_x: zpa += x; return;
        case rm_zp_y: zpa += y; return;
        case rm_x:
            a1l += x;
            if (a1l < x) { a2 = a1 + 0x0100; mcp += 2; }
            return;
        case rm_y:
            a1l += y;
            if (a1l < y) { a2 = a1 + 0x0100; mcp += 2; }
            return;
        case rm_idx_ind: zpa += x; a2 = (u8)(zpa + 0x01); return;
        case a_nz: set_nz(a); return;
        case do_op: switch (ir) {
            // case 0x00: case 0x02: case 0x04: case 0x08: case 0x0c: case 0x10: case 0x12: case 0x14: case 0x1a: case 0x1c: case 0x20: case 0x22: case 0x28: case 0x29: case 0x30: case 0x32: case 0x34: case 0x3a: case 0x3c: case 0x40: case 0x42: case 0x44: case 0x48: case 0x4c: case 0x50: case 0x52: case 0x54: case 0x58: case 0x5a: case 0x5c: case 0x60: case 0x62: case 0x64: case 0x68: case 0x6c: case 0x70: case 0x72: case 0x74: case 0x78: case 0x7a: case 0x7c: case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85: case 0x86: case 0x87: case 0x89: case 0x8c: case 0x8d: case 0x8e: case 0x8f: case 0x90: case 0x91: case 0x92: case 0x94: case 0x95: case 0x96: case 0x97: case 0x99: case 0x9d: case 0xa5: case 0xad: case 0xb0: case 0xb2: case 0xbd: case 0xc2: case 0xd0: case 0xd2: case 0xd4: case 0xda: case 0xdc: case 0xe2: case 0xea: case 0xf0: case 0xf2: case 0xf4: case 0xfa: case 0xfc:  return; /* nop */
            case 0x01: case 0x05: case 0x09: case 0x0d: case 0x11: case 0x15:
            case 0x19: case 0x1d: set_nz(a|=d); return; /* rm_ora */
            case 0x03: case 0x07: case 0x0f: case 0x13: case 0x17: case 0x1b:
            case 0x1f: do_ud_slo(); return; /* ud_slo */
            case 0x06: case 0x0e: case 0x16: case 0x1e: do_asl(d); return; /* rmw_asl */
            case 0x0a: do_asl(a); return; /* sb_asl */
            case 0x0b: case 0x2b: do_ud_anc(); return; /* ud_anc */
            case 0x18: clr(Flag::C); return; /* sb_clc */
            case 0x21: case 0x25: case 0x29: case 0x2d: case 0x31: case 0x35:
            case 0x39: case 0x3d: set_nz(a&=d); return; /* rm_and */
            case 0x23: case 0x27: case 0x2f: case 0x33: case 0x37: case 0x3b:
            case 0x3f: do_ud_rla(); return; /* ud_rla */
            case 0x24: case 0x2c: do_bit(); return; /* rm_bit */
            case 0x26: case 0x2e: case 0x36: case 0x3e: do_rol(d); return; /* rmw_rol */
            case 0x2a: do_rol(a); return; /* sb_rol */
            case 0x38: set(Flag::C); return; /* sb_sec */
            case 0x41: case 0x45: case 0x49: case 0x4d: case 0x51: case 0x55:
            case 0x59: case 0x5d: set_nz(a^=d); return; /* rm_eor */
            case 0x43: case 0x47: case 0x4f: case 0x53: case 0x57: case 0x5b:
            case 0x5f: do_ud_sre(); return; /* ud_sre */
            case 0x46: case 0x4e: case 0x56: case 0x5e: do_lsr(d); return; /* rmw_lsr */
            case 0x4a: do_lsr(a); return; /* sb_lsr */
            case 0x4b: a&=d;do_lsr(a); return; /* ud_alr */
            case 0x61: case 0x65: case 0x69: case 0x6d: case 0x71: case 0x75:
            case 0x79: case 0x7d: do_adc(); return; /* rm_adc */
            case 0x63: case 0x67: case 0x6f: case 0x73: case 0x77: case 0x7b:
            case 0x7f: do_ud_rra(); return; /* ud_rra */
            case 0x66: case 0x6e: case 0x76: case 0x7e: do_ror(d); return; /* rmw_ror */
            case 0x6a: do_ror(a); return; /* sb_ror */
            case 0x6b: do_ud_arr(); return; /* ud_arr */
            case 0x88: set_nz(--y); return; /* sb_dey */
            case 0x8a: set_nz(a=x); return; /* sb_txa */
            case 0x8b: do_ud_xaa(); return; /* ud_xaa */
            case 0x93: case 0x9f: d=a&x&(a1h+1); return; /* ud_ahx */
            case 0x98: set_nz(a=y); return; /* sb_tya */
            case 0x9a: sp=x; return; /* sb_txs */
            case 0x9b: do_ud_tas(); return; /* ud_tas */
            case 0x9c: d=y&(a1h+1); return; /* ud_shy */
            case 0x9e: d=x&(a1h+1); return; /* ud_shx */
            case 0xa0: case 0xa4: case 0xac: case 0xb4: case 0xbc: set_nz(y=d); return; /* rm_ldy */
            case 0xa1: case 0xa5: case 0xa9: case 0xad: case 0xb1: case 0xb5:
            case 0xb9: case 0xbd: set_nz(a=d); return; /* rm_lda */
            case 0xa2: case 0xa6: case 0xae: case 0xb6: case 0xbe: set_nz(x=d); return; /* rm_ldx */
            case 0xa3: case 0xa7: case 0xaf: case 0xb3: case 0xb7: case 0xbf:
                set_nz(a=x=d); return; /* ud_lax */
            case 0xa8: set_nz(y=a); return; /* sb_tay */
            case 0xaa: set_nz(x=a); return; /* sb_tax */
            case 0xab: do_ud_lxa(); return; /* ud_lxa */
            case 0xb8: clr(Flag::V); return; /* sb_clv */
            case 0xba: set_nz(x=sp); return; /* sb_tsx */
            case 0xbb: do_ud_las(); return; /* ud_las */
            case 0xc0: case 0xc4: case 0xcc: do_cmp(y); return; /* rm_cpy */
            case 0xc1: case 0xc5: case 0xc9: case 0xcd: case 0xd1: case 0xd5:
            case 0xd9: case 0xdd: do_cmp(a); return; /* rm_cmp */
            case 0xc3: case 0xc7: case 0xcf: case 0xd3: case 0xd7: case 0xdb:
            case 0xdf: d--;do_cmp(a); return; /* ud_dcp */
            case 0xc6: case 0xce: case 0xd6: case 0xde: set_nz(--d); return; /* rmw_dec */
            case 0xc8: set_nz(++y); return; /* sb_iny */
            case 0xca: set_nz(--x); return; /* sb_dex */
            case 0xcb: do_ud_axs(); return; /* ud_axs */
            case 0xd8: clr(Flag::D); return; /* sb_cld */
            case 0xe0: case 0xe4: case 0xec: do_cmp(x); return; /* rm_cpx */
            case 0xe1: case 0xe5: case 0xe9: case 0xeb: case 0xed: case 0xf1:
            case 0xf5: case 0xf9: case 0xfd: do_sbc(); return; /* rm_sbc */
            case 0xe3: case 0xe7: case 0xef: case 0xf3: case 0xf7: case 0xfb:
            case 0xff: d++;do_sbc(); return; /* ud_isc */
            case 0xe6: case 0xee: case 0xf6: case 0xfe: set_nz(++d); return; /* rmw_inc */
            case 0xe8: set_nz(++x); return; /* sb_inx */
            case 0xf8: set(Flag::D); return; /* sb_sed */
        } return;
        case st_zp_x: zpa += x; st_reg_sel(); return;
        case st_zp_y: zpa += y; st_reg_sel(); return;
        case st_idx_ind: zpa += x; a2 = (u8)(zpa + 0x01); // fall through
        case st_reg: st_reg_sel(); return;
        case jmp_ind: ++a1l; return;
        case bpl: if (is_set(Flag::N)) { mcp += 7; /*T2 (no bra)*/ return; } else goto bra_take;
        case bmi: if (is_clr(Flag::N)) { mcp += 6; /*T2 (no bra)*/ return; } else goto bra_take;
        case bvc: if (is_set(Flag::V)) { mcp += 5; /*T2 (no bra)*/ return; } else goto bra_take;
        case bvs: if (is_clr(Flag::V)) { mcp += 4; /*T2 (no bra)*/ return; } else goto bra_take;
        case bcc: if (is_set(Flag::C)) { mcp += 3; /*T2 (no bra)*/ return; } else goto bra_take;
        case bcs: if (is_clr(Flag::C)) { mcp += 2; /*T2 (no bra)*/ return; } else goto bra_take;
        case beq: if (is_clr(Flag::Z)) { mcp += 1; /*T2 (no bra)*/ return; } else goto bra_take;
        case bne: if (is_set(Flag::Z)) /*T2 (no bra)*/ return;
            // fall through
        bra_take:
            mcp = MC::OPC_MC[OPC_bne] + 2; // T2 (no page cross)
            a2 = pc;
            pc += (i8)d;
            if ((a2 ^ pc) & 0xff00) {
                mcp += 2; // T2 (page cross)
                a1 = a2;
                a1l += d;
            }
            return;
        case hold_ints:
            if (nmi_req == 0x02) nmi_req = 0x01;
            if (irq_req && (irq_req < 0x04)) irq_req = 0x01;
            return;
        case php: p |= (Flag::B | Flag::u); // fall through
        case pha: a1 = spf; --sp; return;
        case jsr: a3 = spf; --sp; a4 = spf; --sp; a2 = pc; // fall through
        case jmp_abs: pc = a1; return;
        case rti: ++sp; a1 = spf; // fall through
        case rts: ++sp; a2 = spf; // fall through
        case inc_sp: ++sp; return;
        case brk:
            // brk_src |= (irq_bit & ~p); // no effect (the same vector used anyway)
            if (nmi_req & 0x3) { // Potential hijacking by nmi
                brk_src |= NMI_BIT;
                nmi_req = NMI_taken;
            }
            a2 = brk_ctrl[brk_src].vec;
            a3 = a2 + 0x0001;
            brk_src = 0x00;
            set(Flag::I);
            return;
        case dispatch_cli: // post cli dispatch
            brk_src |= (irq_bit & ~p);       // bit 2
            clr(Flag::I);
            goto dispatch_post_cli_sei;
        case dispatch_sei: // post sei dispatch
            brk_src |= (irq_bit & ~p);       // bit 2
            set(Flag::I);
            goto dispatch_post_cli_sei;
        case dispatch: // 'normal' T0 case (all interrupts taken normally)
            /* Interrupt hijacking:
               - happens if an interrupt of a higher priority is signalled before
                 the 'brk' execution of the lower priority reaches T5 (=T4 ph2 at latest).
               - T0&T1 PC, and pushed b-flag according to the former (lower prio)
               - vector address according to the latter (higher prio)
            */
            brk_src |= (irq_bit & ~p);   // bit 2
        dispatch_post_cli_sei: // irq has been taken based on 'old' i-flag value (still can hijack a sw brk!)
            if (nmi_bit) {
                brk_src |= nmi_bit;      // bit 1
                nmi_bit = 0x00;
                if (nmi_req) nmi_req = NMI_taken;
            }
            // fall through
        case dispatch_brk: // hw-ints not taken on the instruction following a brk (sw or hw)
            brk_src |= (ir == OPC_brk); // bit 0

            if (brk_src) {
                ir = 0x00;
                p = (p | (Flag::B | Flag::u)) & brk_ctrl[brk_src].p_mask;
                pc -= brk_ctrl[brk_src].pc_t0;
                a1 = pc + brk_ctrl[brk_src].pc_t1;
                a2 = spf; --sp;
                a3 = spf; --sp;
                a4 = spf; --sp;
            }

            mcp = MC::OPC_MC[ir];

            return;
        case sig_hlt: sig_halt(); return;
        case hlt: --mcp; return; // stuck
        case reset: a1 = Vec::rst + 0x0001; set(Flag::I); return;
    }
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
        set(Flag::Z, ((a + d + C()) & 0xff) == 0x00); // Z set based on binary result
        u8 n1; u8 c1; u8 n2; u8 c2;
        n1 = adc_dec_nib((a & 0xf) + (d & 0xf) + C(), c1);
        n2 = (a >> 4) + (d >> 4) + c1;
        u8 r = (n2 << 4) | n1; // high nib not adjusted yet (N&V set based on this)
        set(Flag::N, r & 0x80);
        set(Flag::V, ((a ^ r) & (d ^ r) & 0x80));
        n2 = adc_dec_nib(n2, c2);
        set(Flag::C, c2); // decimal carry
        a = (n2 << 4) | n1;
    } else {
        u16 r = a + d + C();
        set(Flag::C, r & 0x100);
        set(Flag::V, ((a ^ r) & (d ^ r) & 0x80));
        set_nz(a = r);
    }
}


void NMOS6502::Core::do_sbc() {
    if (is_set(Flag::D)) {
        auto sbc_dec_nib = [](u8 res, u8& co) -> u8 {
            if (res > 0xf) { co = 0x1; return (res - 0x6) & 0xf; }
            else { co = 0x0; return res; }
        };
        set(Flag::Z, ((a - d - B()) & 0xff) == 0x00);
        u8 n1; u8 c1; u8 n2; u8 c2;
        n1 = sbc_dec_nib((a & 0xf) - (d & 0xf) - B(), c1);
        n2 = (a >> 4) - (d >> 4) - c1;
        u8 r = (n2 << 4) | n1;
        set(Flag::N, r & 0x80);
        set(Flag::V, ((a ^ d) & (a ^ r)) & 0x80);
        n2 = sbc_dec_nib(n2, c2);
        set(Flag::C, !c2);
        a = (n2 << 4) | n1;
    } else {
        u16 r = a - d - B();
        set(Flag::C, r < 0x100);
        set(Flag::V, ((a ^ d) & (a ^ r)) & 0x80);
        set_nz(a = r);
    }
}


void NMOS6502::Core::do_ud_arr() {
    /* Totally based on c64doc.txt by John West & Marko Mäkelä.
        (http://www.6502.org/users/andre/petindex/local/64doc.txt)
    */
    if (is_set(Flag::D)) {
        d &= a;

        u8 ah = d >> 4;
        u8 al = d & 0xf;

        set(Flag::N, is_set(Flag::C));
        a = (d >> 1) | (p << 7);
        set(Flag::Z, a == 0x00);
        set(Flag::V, (d ^ a) & 0x40);

        if (al + (al & 0x1) > 0x5) a = (a & 0xf0) | ((a + 0x6) & 0xf);
        if ((ah + (ah & 0x1)) > 0x5) { set(Flag::C); a += 0x60; }
        else clr(Flag::C);
    } else {
        a &= d;
        a >>= 1;
        a |= (p << 7);
        set_nz(a);
        set(Flag::C, a & 0x40);
        set(Flag::V, (a ^ (a << 1)) & 0x40);
    }
}

