
#include "nmos6502_core.h"


NMOS6502::Core::Core() :
    r8((Reg8*)r16),
    pc(r16[R16::pc]), spf(r16[R16::spf]), zpaf(r16[R16::zpaf]),
    a1(r16[R16::a1]), a2(r16[R16::a2]), a3(r16[R16::a3]), a4(r16[R16::a4]),
    pcl(r8[R8::pcl]), pch(r8[R8::pch]), sp(r8[R8::sp]), p(r8[R8::p]), a(r8[R8::a]), x(r8[R8::x]), y(r8[R8::y]),
    d(r8[R8::d]), ir(r8[R8::ir]), zpa(r8[R8::zpa]), a1l(r8[R8::a1l]), a1h(r8[R8::a1h])
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


void NMOS6502::Core::exec_cycle() {
    using namespace NMOS6502::MC;

    pc += mcp->pc_inc;

    switch (mcp->mopc) {
        case nmop: break;

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

        case dispatch_cli: // post cli dispatch
            brk_src |= (irq_bit & ~p);       // bit 2
            clr(Flag::I);
            goto dispatch_post_cli_sei;
        case dispatch_sei: // post sei dispatch
            brk_src |= (irq_bit & ~p);       // bit 2
            set(Flag::I);
            goto dispatch_post_cli_sei;
        case do_op:
            switch (MC::OPC_MSOPC[ir]) {
                case sb_asl:  do_asl(a);      break;   case sb_clc:  clr(Flag::C);   break;
                case sb_cld:  clr(Flag::D);   break;   case sb_clv:  clr(Flag::V);   break;
                case sb_dex:  set_nz(--x);    break;   case sb_dey:  set_nz(--y);    break;
                case sb_inx:  set_nz(++x);    break;   case sb_iny:  set_nz(++y);    break;
                case sb_lsr:  do_lsr(a);      break;   case sb_rol:  do_rol(a);      break;
                case sb_ror:  do_ror(a);      break;   case sb_sec:  set(Flag::C);   break;
                case sb_sed:  set(Flag::D);   break;   case sb_tax:  set_nz(x = a);  break;
                case sb_tay:  set_nz(y = a);  break;   case sb_tsx:  set_nz(x = sp); break;
                case sb_txa:  set_nz(a = x);  break;   case sb_txs:  sp = x;         break;
                case sb_tya:  set_nz(a = y);  break;   case rm_adc:  do_adc();       break;
                case rm_and:  set_nz(a &= d); break;   case rm_bit:  do_bit();       break;
                case rm_cmp:  do_cmp(a);      break;   case rm_cpx:  do_cmp(x);      break;
                case rm_cpy:  do_cmp(y);      break;   case rm_eor:  set_nz(a ^= d); break;
                case rm_lda:  set_nz(a = d);  break;   case rm_ldx:  set_nz(x = d);  break;
                case rm_ldy:  set_nz(y = d);  break;   case rm_ora:  set_nz(a |= d); break;
                case rm_sbc:  do_sbc();       break;   case rmw_asl: do_asl(d);      break;
                case rmw_dec: set_nz(--d);    break;   case rmw_inc: set_nz(++d);    break;
                case rmw_lsr: do_lsr(d);      break;   case rmw_rol: do_rol(d);      break;
                case rmw_ror: do_ror(d);      break;   case nop:                     break;
                case ud_ahx:  d=a&x&(a1h+1);  break;   case ud_alr:  a&=d;do_lsr(a); break;
                case ud_anc:  do_ud_anc();    break;   case ud_arr:  do_ud_arr();    break;
                case ud_axs:  do_ud_axs();    break;   case ud_dcp:  d--; do_cmp(a); break;
                case ud_isc:  d++; do_sbc();  break;   case ud_las:  do_ud_las();    break;
                case ud_lax:  set_nz(a=x=d);  break;   case ud_lxa:  do_ud_lxa();    break;
                case ud_rla:  do_ud_rla();    break;   case ud_rra:  do_ud_rra();    break;
                case ud_shx:  d = x&(a1h+1);  break;   case ud_shy:  d = y&(a1h+1);  break;
                case ud_slo:  do_ud_slo();    break;   case ud_sre:  do_ud_sre();    break;
                case ud_tas:  do_ud_tas();    break;   case ud_xaa:  do_ud_xaa();    break;
                // case err: abort(); return;  // Needed? Tests don't cover halts...
            }
            break;
        case idx_ind: zpa += x; a2 = (u8)(zpa + 0x01); break;
        case abs_x:
            a1l += x;
            if (a1l < x) { a2 = a1 + 0x0100; mcp += 3; return; }
            break;
        case abs_y:
            a1l += y;
            if (a1l < y) { a2 = a1 + 0x0100; mcp += 3; return; }
            break;
        case zp_x: zpa += x; break;
        case zp_y: zpa += y; break;
        case inc_zpa: ++zpa; break;
        case ind_idx:
            a1l += y;
            if (a1l < y) { a2 = a1 + 0x0100; mcp += 3; return; }
            break;
        case st_reg: st_reg_sel(); break;
        case st_abs_x: a2 = a1 + x; a1l += x; break;
        case st_abs_y: a2 = a1 + y; a1l += y; break;
        case st_idx_ind: zpa += x; a2 = (u8)(zpa + 0x01); st_reg_sel(); break;
        case st_zp_x: zpa += x; st_reg_sel(); break;
        case st_zp_y: zpa += y; st_reg_sel(); break;
        case st_ind_y: a2 = a1 + y; a1l += y; break;
        case abs_x_rmw: a2 = a1 + x; a1l += x; break;
        case inc_sp: ++sp; break;
        case php: p |= (Flag::B | Flag::u); // fall through
        case pha: a1 = spf; --sp; break;
        case pla: set_nz(a); break;
        case jsr: a2 = pc; pc = a1; a3 = spf; --sp; a4 = spf; --sp; break;
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
            break;
        case rti: ++sp; a1 = spf; ++sp; a2 = spf; ++sp; break;
        case jmp_abs: pc = a1; break;
        case jmp_ind: ++a1l; break;
        case rts: ++sp; a1 = spf; ++sp; break;
        case bra: do_bra(); return;
        case hold_ints:
            if (nmi_req == 0x02) nmi_req = 0x01;
            if (irq_req && (irq_req < 0x04)) irq_req = 0x01;
            break;
        case abs_y_rmw: a2 = a1 + y; a1l += y; break;
        case sig_hlt: sig_halt(); break;
        case hlt: return; // stuck
        case reset: a1 = Vec::rst + 1; set(Flag::I); break;
    }
    ++mcp;
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

