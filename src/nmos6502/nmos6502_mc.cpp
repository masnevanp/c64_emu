
#include "nmos6502_mc.h"
//#include <map>
//#include <vector>


const std::string NMOS6502::PC_inc_str[] = { ", ", ", inc(pc), " };

const std::string NMOS6502::MOPC_str[] = {
    "nmop", "dispatch", "dispatch_brk", "dispatch_cli", "dispatch_sei", "do_op",
    "idx_ind", "abs_x", "abs_y", "zp_x", "zp_y", "inc_zpa", "ind_idx",
    "st_reg", "st_abs_x", "st_abs_y", "st_idx_ind", "st_zp_x", "st_zp_y", "st_ind_y",
    "abs_x_rmw",
    "inc_sp", "php", "pha", "pla", "jsr", "brk1", "reset", "rti", "jmp_abs", "jmp_ind", "rts", "bra", "hold_ints",
    "abs_y_rmw", "sig_hlt", "hlt", //"__end",
};


// instruction (opc) to micro-code sub-operation mapping
const NMOS6502::u8 NMOS6502::MC::OPC_MSOPC[] = {
    // 00
    MSOPC::err, MSOPC::rm_ora, MSOPC::err, MSOPC::ud_slo, MSOPC::nop, MSOPC::rm_ora, MSOPC::rmw_asl, MSOPC::ud_slo,
    MSOPC::err, MSOPC::rm_ora, MSOPC::sb_asl, MSOPC::ud_anc, MSOPC::nop, MSOPC::rm_ora, MSOPC::rmw_asl, MSOPC::ud_slo,
    // 10
    MSOPC::err, MSOPC::rm_ora, MSOPC::err, MSOPC::ud_slo, MSOPC::nop, MSOPC::rm_ora, MSOPC::rmw_asl, MSOPC::ud_slo,
    MSOPC::sb_clc, MSOPC::rm_ora, MSOPC::nop, MSOPC::ud_slo, MSOPC::nop, MSOPC::rm_ora, MSOPC::rmw_asl, MSOPC::ud_slo,
    // 20
    MSOPC::err, MSOPC::rm_and, MSOPC::err, MSOPC::ud_rla, MSOPC::rm_bit, MSOPC::rm_and, MSOPC::rmw_rol, MSOPC::ud_rla,
    MSOPC::err, MSOPC::rm_and, MSOPC::sb_rol, MSOPC::ud_anc, MSOPC::rm_bit, MSOPC::rm_and, MSOPC::rmw_rol, MSOPC::ud_rla,
    // 30
    MSOPC::err, MSOPC::rm_and, MSOPC::err, MSOPC::ud_rla, MSOPC::nop, MSOPC::rm_and, MSOPC::rmw_rol, MSOPC::ud_rla,
    MSOPC::sb_sec, MSOPC::rm_and, MSOPC::nop, MSOPC::ud_rla, MSOPC::nop, MSOPC::rm_and, MSOPC::rmw_rol, MSOPC::ud_rla,
    // 40
    MSOPC::err, MSOPC::rm_eor, MSOPC::err, MSOPC::ud_sre, MSOPC::nop, MSOPC::rm_eor, MSOPC::rmw_lsr, MSOPC::ud_sre,
    MSOPC::err, MSOPC::rm_eor, MSOPC::sb_lsr, MSOPC::ud_alr, MSOPC::err, MSOPC::rm_eor, MSOPC::rmw_lsr, MSOPC::ud_sre,
    // 50
    MSOPC::err, MSOPC::rm_eor, MSOPC::err, MSOPC::ud_sre, MSOPC::nop, MSOPC::rm_eor, MSOPC::rmw_lsr, MSOPC::ud_sre,
    MSOPC::err, MSOPC::rm_eor, MSOPC::nop, MSOPC::ud_sre, MSOPC::nop, MSOPC::rm_eor, MSOPC::rmw_lsr, MSOPC::ud_sre,
    // 60
    MSOPC::err, MSOPC::rm_adc, MSOPC::err, MSOPC::ud_rra, MSOPC::nop, MSOPC::rm_adc, MSOPC::rmw_ror, MSOPC::ud_rra,
    MSOPC::err, MSOPC::rm_adc, MSOPC::sb_ror, MSOPC::ud_arr, MSOPC::err, MSOPC::rm_adc, MSOPC::rmw_ror, MSOPC::ud_rra,
    // 70
    MSOPC::err, MSOPC::rm_adc, MSOPC::err, MSOPC::ud_rra, MSOPC::nop, MSOPC::rm_adc, MSOPC::rmw_ror, MSOPC::ud_rra,
    MSOPC::err, MSOPC::rm_adc, MSOPC::nop, MSOPC::ud_rra, MSOPC::nop, MSOPC::rm_adc, MSOPC::rmw_ror, MSOPC::ud_rra,
    // 80
    MSOPC::nop, MSOPC::err, MSOPC::nop, MSOPC::err, MSOPC::err, MSOPC::err, MSOPC::err, MSOPC::err,
    MSOPC::sb_dey, MSOPC::nop, MSOPC::sb_txa, MSOPC::ud_xaa, MSOPC::err, MSOPC::err, MSOPC::err, MSOPC::err,
    // 90
    MSOPC::err, MSOPC::err, MSOPC::err, MSOPC::ud_ahx, MSOPC::err, MSOPC::err, MSOPC::err, MSOPC::err,
    MSOPC::sb_tya, MSOPC::err, MSOPC::sb_txs, MSOPC::ud_tas, MSOPC::ud_shy, MSOPC::err, MSOPC::ud_shx, MSOPC::ud_ahx,
    // a0
    MSOPC::rm_ldy, MSOPC::rm_lda, MSOPC::rm_ldx, MSOPC::ud_lax, MSOPC::rm_ldy, MSOPC::rm_lda, MSOPC::rm_ldx, MSOPC::ud_lax,
    MSOPC::sb_tay, MSOPC::rm_lda, MSOPC::sb_tax, MSOPC::ud_lxa, MSOPC::rm_ldy, MSOPC::rm_lda, MSOPC::rm_ldx, MSOPC::ud_lax,
    // b0
    MSOPC::err, MSOPC::rm_lda, MSOPC::err, MSOPC::ud_lax, MSOPC::rm_ldy, MSOPC::rm_lda, MSOPC::rm_ldx, MSOPC::ud_lax,
    MSOPC::sb_clv, MSOPC::rm_lda, MSOPC::sb_tsx, MSOPC::ud_las, MSOPC::rm_ldy, MSOPC::rm_lda, MSOPC::rm_ldx, MSOPC::ud_lax,
    // c0
    MSOPC::rm_cpy, MSOPC::rm_cmp, MSOPC::nop, MSOPC::ud_dcp, MSOPC::rm_cpy, MSOPC::rm_cmp, MSOPC::rmw_dec, MSOPC::ud_dcp,
    MSOPC::sb_iny, MSOPC::rm_cmp, MSOPC::sb_dex, MSOPC::ud_axs, MSOPC::rm_cpy, MSOPC::rm_cmp, MSOPC::rmw_dec, MSOPC::ud_dcp,
    // d0
    MSOPC::err, MSOPC::rm_cmp, MSOPC::err, MSOPC::ud_dcp, MSOPC::nop, MSOPC::rm_cmp, MSOPC::rmw_dec, MSOPC::ud_dcp,
    MSOPC::sb_cld, MSOPC::rm_cmp, MSOPC::nop, MSOPC::ud_dcp, MSOPC::nop, MSOPC::rm_cmp, MSOPC::rmw_dec, MSOPC::ud_dcp,
    // e0
    MSOPC::rm_cpx, MSOPC::rm_sbc, MSOPC::nop, MSOPC::ud_isc, MSOPC::rm_cpx, MSOPC::rm_sbc, MSOPC::rmw_inc, MSOPC::ud_isc,
    MSOPC::sb_inx, MSOPC::rm_sbc, MSOPC::nop, MSOPC::rm_sbc, MSOPC::rm_cpx, MSOPC::rm_sbc, MSOPC::rmw_inc, MSOPC::ud_isc,
    // f0
    MSOPC::err, MSOPC::rm_sbc, MSOPC::err, MSOPC::ud_isc, MSOPC::nop, MSOPC::rm_sbc, MSOPC::rmw_inc, MSOPC::ud_isc,
    MSOPC::sb_sed, MSOPC::rm_sbc, MSOPC::nop, MSOPC::ud_isc, MSOPC::nop, MSOPC::rm_sbc, MSOPC::rmw_inc, MSOPC::ud_isc,
};


namespace _MC { // micro-code: 1..n micro-ops/instr (1 micro-op/1 cycle)
    using NMOS6502::MOP;
    using NMOS6502::R16;
    using NMOS6502::R8;
    using NMOS6502::RW;
    using NMOS6502::MOPC;

    using NMOS6502::PC_inc;
    namespace SB {
        static const MOP sb[] = {
            MOP(R16::pc,   R8::d,    RW::r, PC_inc::n, MOPC::do_op    ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
            // MOP(0,         0,        0,     0,         MOPC::__end ),
        };
        /* cli&sei feature: change not be visible at next T0 (dispatch)
            (http://visual6502.org/wiki/index.php?title=6502_Timing_of_Interrupt_Handling)
        */
        static const MOP sb_cli[] = {
            MOP(R16::pc,   R8::d,    RW::r, PC_inc::n, MOPC::nmop         ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch_cli ),
            // MOP(0,         0,        0,     0,         MOPC::__end     ),
        };
        static const MOP sb_sei[] = {
            MOP(R16::pc,   R8::d,    RW::r, PC_inc::n, MOPC::nmop         ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch_sei ),
            // MOP(0,         0,        0,     0,         MOPC::__end     ),
        };
    }
    namespace RM {
        static const MOP imm[] = {
            MOP(R16::pc,   R8::d,    RW::r, PC_inc::y, MOPC::do_op    ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
            // MOP(0,         0,        0,     0,         MOPC::__end ),
        };
        static const MOP zp[] = {
            MOP(R16::pc,   R8::zpa,  RW::r, PC_inc::y, MOPC::nmop     ),
            MOP(R16::zpaf, R8::d,    RW::r, PC_inc::n, MOPC::do_op    ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
            // MOP(0,         0,        0,     0,         MOPC::__end ),
        };
        static const MOP abs[] = {
            MOP(R16::pc,   R8::a1l,  RW::r, PC_inc::y, MOPC::nmop     ),
            MOP(R16::pc,   R8::a1h,  RW::r, PC_inc::y, MOPC::nmop     ),
            MOP(R16::a1,   R8::d,    RW::r, PC_inc::n, MOPC::do_op    ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
            // MOP(0,         0,        0,     0,         MOPC::__end ),
        };
        static const MOP idx_ind[] = {
            MOP(R16::pc,   R8::zpa,  RW::r, PC_inc::y, MOPC::nmop     ),
            MOP(R16::zpaf, R8::d,    RW::r, PC_inc::n, MOPC::idx_ind  ),
            MOP(R16::zpaf, R8::a1l,  RW::r, PC_inc::n, MOPC::nmop     ),
            MOP(R16::a2,   R8::a1h,  RW::r, PC_inc::n, MOPC::nmop     ),
            MOP(R16::a1,   R8::d,    RW::r, PC_inc::n, MOPC::do_op    ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
            // MOP(0,         0,        0,     0,         MOPC::__end ),
        };
        static const MOP abs_x[] = {
            MOP(R16::pc,   R8::a1l,  RW::r, PC_inc::y, MOPC::nmop     ),
            MOP(R16::pc,   R8::a1h,  RW::r, PC_inc::y, MOPC::abs_x    ),
            MOP(R16::a1,   R8::d,    RW::r, PC_inc::n, MOPC::do_op    ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
            MOP(R16::a1,   R8::d,    RW::r, PC_inc::n, MOPC::nmop     ),
            MOP(R16::a2,   R8::d,    RW::r, PC_inc::n, MOPC::do_op    ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
            // MOP(0,         0,        0,     0,         MOPC::__end ),
        };
        static const MOP abs_y[] = {
            MOP(R16::pc,   R8::a1l,  RW::r, PC_inc::y, MOPC::nmop     ),
            MOP(R16::pc,   R8::a1h,  RW::r, PC_inc::y, MOPC::abs_y    ),
            MOP(R16::a1,   R8::d,    RW::r, PC_inc::n, MOPC::do_op    ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
            MOP(R16::a1,   R8::d,    RW::r, PC_inc::n, MOPC::nmop     ),
            MOP(R16::a2,   R8::d,    RW::r, PC_inc::n, MOPC::do_op    ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
            // MOP(0,         0,        0,     0,         MOPC::__end ),
        };
        static const MOP zp_x[] = {
            MOP(R16::pc,   R8::zpa,  RW::r, PC_inc::y, MOPC::nmop     ),
            MOP(R16::zpaf, R8::d,    RW::r, PC_inc::n, MOPC::zp_x     ),
            MOP(R16::zpaf, R8::d,    RW::r, PC_inc::n, MOPC::do_op    ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
            // MOP(0,         0,        0,     0,         MOPC::__end ),
        };
        static const MOP zp_y[] = {
            MOP(R16::pc,   R8::zpa,  RW::r, PC_inc::y, MOPC::nmop     ),
            MOP(R16::zpaf, R8::d,    RW::r, PC_inc::n, MOPC::zp_y     ),
            MOP(R16::zpaf, R8::d,    RW::r, PC_inc::n, MOPC::do_op    ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
            // MOP(0,         0,        0,     0,         MOPC::__end ),
        };
        static const MOP ind_idx[] = {
            MOP(R16::pc,   R8::zpa,  RW::r, PC_inc::y, MOPC::nmop     ),
            MOP(R16::zpaf, R8::a1l,  RW::r, PC_inc::n, MOPC::inc_zpa  ),
            MOP(R16::zpaf, R8::a1h,  RW::r, PC_inc::n, MOPC::ind_idx  ),
            MOP(R16::a1,   R8::d,    RW::r, PC_inc::n, MOPC::do_op    ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
            MOP(R16::a1,   R8::d,    RW::r, PC_inc::n, MOPC::nmop     ),
            MOP(R16::a2,   R8::d,    RW::r, PC_inc::n, MOPC::do_op    ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
            // MOP(0,         0,        0,     0,         MOPC::__end ),
        };
    }
    namespace ST {
        static const MOP zp[] = {
            MOP(R16::pc,   R8::zpa,  RW::r, PC_inc::y, MOPC::st_reg   ),
            MOP(R16::zpaf, R8::d,    RW::w, PC_inc::n, MOPC::nmop     ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
            // MOP(0,         0,        0,     0,         MOPC::__end ),
        };
        static const MOP abs[] = {
            MOP(R16::pc,   R8::a1l,  RW::r, PC_inc::y, MOPC::nmop     ),
            MOP(R16::pc,   R8::a1h,  RW::r, PC_inc::y, MOPC::st_reg   ),
            MOP(R16::a1,   R8::d,    RW::w, PC_inc::n, MOPC::nmop     ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
            // MOP(0,         0,        0,     0,         MOPC::__end ),
        };
        static const MOP idx_ind[] = {
            MOP(R16::pc,   R8::zpa,  RW::r, PC_inc::y, MOPC::nmop     ),
            MOP(R16::zpaf, R8::d,    RW::r, PC_inc::n, MOPC::st_idx_ind),
            MOP(R16::zpaf, R8::a1l,  RW::r, PC_inc::n, MOPC::nmop     ),
            MOP(R16::a2,   R8::a1h,  RW::r, PC_inc::n, MOPC::nmop     ),
            MOP(R16::a1,   R8::d,    RW::w, PC_inc::n, MOPC::nmop     ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
            // MOP(0,         0,        0,     0,         MOPC::__end ),
        };
        static const MOP abs_x[] = {
            MOP(R16::pc,   R8::a1l,  RW::r, PC_inc::y, MOPC::nmop     ),
            MOP(R16::pc,   R8::a1h,  RW::r, PC_inc::y, MOPC::st_abs_x ),
            MOP(R16::a1,   R8::d,    RW::r, PC_inc::n, MOPC::nmop     ),
            MOP(R16::a2,   R8::a,    RW::w, PC_inc::n, MOPC::nmop     ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
            // MOP(0,         0,        0,     0,         MOPC::__end ),
        };
        static const MOP abs_y[] = {
            MOP(R16::pc,   R8::a1l,  RW::r, PC_inc::y, MOPC::nmop     ),
            MOP(R16::pc,   R8::a1h,  RW::r, PC_inc::y, MOPC::st_abs_y ),
            MOP(R16::a1,   R8::d,    RW::r, PC_inc::n, MOPC::nmop     ),
            MOP(R16::a2,   R8::a,    RW::w, PC_inc::n, MOPC::nmop     ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
            // MOP(0,         0,        0,     0,         MOPC::__end ),
        };
        static const MOP zp_x[] = {
            MOP(R16::pc,   R8::zpa,  RW::r, PC_inc::y, MOPC::nmop     ),
            MOP(R16::zpaf, R8::d,    RW::r, PC_inc::n, MOPC::st_zp_x  ),
            MOP(R16::zpaf, R8::d,    RW::w, PC_inc::n, MOPC::nmop     ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
            // MOP(0,         0,        0,     0,         MOPC::__end ),
        };
        static const MOP zp_y[] = {
            MOP(R16::pc,   R8::zpa,  RW::r, PC_inc::y, MOPC::nmop     ),
            MOP(R16::zpaf, R8::d,    RW::r, PC_inc::n, MOPC::st_zp_y  ),
            MOP(R16::zpaf, R8::d,    RW::w, PC_inc::n, MOPC::nmop     ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
            // MOP(0,         0,        0,     0,         MOPC::__end ),
        };
        static const MOP ind_y[] = {
            MOP(R16::pc,   R8::zpa,  RW::r, PC_inc::y, MOPC::nmop     ),
            MOP(R16::zpaf, R8::a1l,  RW::r, PC_inc::n, MOPC::inc_zpa  ),
            MOP(R16::zpaf, R8::a1h,  RW::r, PC_inc::n, MOPC::st_ind_y ),
            MOP(R16::a1,   R8::d,    RW::r, PC_inc::n, MOPC::nmop     ),
            MOP(R16::a2,   R8::a,    RW::w, PC_inc::n, MOPC::nmop     ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
            // MOP(0,         0,        0,     0,         MOPC::__end ),
        };
    }
    namespace RMW {
        static const MOP zp[] = {
            MOP(R16::pc,   R8::zpa,  RW::r, PC_inc::y, MOPC::nmop     ),
            MOP(R16::zpaf, R8::d,    RW::r, PC_inc::n, MOPC::nmop     ),
            MOP(R16::zpaf, R8::d,    RW::w, PC_inc::n, MOPC::do_op    ),
            MOP(R16::zpaf, R8::d,    RW::w, PC_inc::n, MOPC::nmop     ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
            // MOP(0,         0,        0,     0,         MOPC::__end ),
        };
        static const MOP abs[] = {
            MOP(R16::pc,   R8::a1l,  RW::r, PC_inc::y, MOPC::nmop     ),
            MOP(R16::pc,   R8::a1h,  RW::r, PC_inc::y, MOPC::nmop     ),
            MOP(R16::a1,   R8::d,    RW::r, PC_inc::n, MOPC::nmop     ),
            MOP(R16::a1,   R8::d,    RW::w, PC_inc::n, MOPC::do_op    ),
            MOP(R16::a1,   R8::d,    RW::w, PC_inc::n, MOPC::nmop     ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
            // MOP(0,         0,        0,     0,         MOPC::__end ),
        };
        static const MOP zp_x[] = {
            MOP(R16::pc,   R8::zpa,  RW::r, PC_inc::y, MOPC::nmop     ),
            MOP(R16::zpaf, R8::d,    RW::r, PC_inc::n, MOPC::zp_x     ),
            MOP(R16::zpaf, R8::d,    RW::r, PC_inc::n, MOPC::nmop     ),
            MOP(R16::zpaf, R8::d,    RW::w, PC_inc::n, MOPC::do_op    ),
            MOP(R16::zpaf, R8::d,    RW::w, PC_inc::n, MOPC::nmop     ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
            // MOP(0,         0,        0,     0,         MOPC::__end ),
        };
        static const MOP abs_x[] = {
            MOP(R16::pc,   R8::a1l,  RW::r, PC_inc::y, MOPC::nmop     ),
            MOP(R16::pc,   R8::a1h,  RW::r, PC_inc::y, MOPC::abs_x_rmw),
            MOP(R16::a1,   R8::d,    RW::r, PC_inc::n, MOPC::nmop     ),
            MOP(R16::a2,   R8::d,    RW::r, PC_inc::n, MOPC::nmop     ),
            MOP(R16::a2,   R8::d,    RW::w, PC_inc::n, MOPC::do_op    ),
            MOP(R16::a2,   R8::d,    RW::w, PC_inc::n, MOPC::nmop     ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
            // MOP(0,         0,        0,     0,         MOPC::__end ),
        };
    }
    namespace FC {
        static const MOP pha[] = {
            MOP(R16::pc,   R8::d,    RW::r, PC_inc::n, MOPC::pha      ),
            MOP(R16::a1,   R8::a,    RW::w, PC_inc::n, MOPC::nmop     ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
            // MOP(0,         0,        0,     0,         MOPC::__end ),
        };
        static const MOP php[] = {
            MOP(R16::pc,   R8::d,    RW::r, PC_inc::n, MOPC::php      ),
            MOP(R16::a1,   R8::p,    RW::w, PC_inc::n, MOPC::nmop     ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
            // MOP(0,         0,        0,     0,         MOPC::__end ),
        };
        static const MOP pla[] = {
            MOP(R16::pc,   R8::d,    RW::r, PC_inc::n, MOPC::nmop     ),
            MOP(R16::spf,  R8::d,    RW::r, PC_inc::n, MOPC::inc_sp   ),
            MOP(R16::spf,  R8::a,    RW::r, PC_inc::n, MOPC::pla      ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
            // MOP(0,         0,        0,     0,         MOPC::__end ),
        };
        static const MOP plp[] = {
            MOP(R16::pc,   R8::d,    RW::r, PC_inc::n, MOPC::nmop     ),
            MOP(R16::spf,  R8::d,    RW::r, PC_inc::n, MOPC::inc_sp   ),
            MOP(R16::spf,  R8::p,    RW::r, PC_inc::n, MOPC::nmop     ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
            // MOP(0,         0,        0,     0,         MOPC::__end ),
        };
        static const MOP jsr[] = {
            MOP(R16::pc,   R8::a1l,  RW::r, PC_inc::y, MOPC::nmop     ),
            MOP(R16::spf,  R8::d,    RW::r, PC_inc::n, MOPC::jsr      ),
            MOP(R16::a3,   R8::a2h,  RW::w, PC_inc::n, MOPC::nmop     ),
            MOP(R16::a4,   R8::a2l,  RW::w, PC_inc::n, MOPC::nmop     ),
            MOP(R16::a2,   R8::pch,  RW::r, PC_inc::n, MOPC::nmop     ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
            // MOP(0,         0,        0,     0,         MOPC::__end ),
        };
        static const MOP brk[] = {
            MOP(R16::pc,   R8::d,    RW::r, PC_inc::n, MOPC::nmop         ),
            MOP(R16::a2,   R8::a1h,  RW::w, PC_inc::n, MOPC::nmop         ),
            MOP(R16::a3,   R8::a1l,  RW::w, PC_inc::n, MOPC::nmop         ),
            MOP(R16::a4,   R8::p,    RW::w, PC_inc::n, MOPC::brk1         ),
            MOP(R16::a2,   R8::pcl,  RW::r, PC_inc::n, MOPC::nmop         ),
            MOP(R16::a3,   R8::pch,  RW::r, PC_inc::n, MOPC::nmop         ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch_brk ),
            // MOP(0,         0,        0,     0,         MOPC::__end ),
        };
        static const MOP reset[] = {
            MOP(R16::pc,   R8::d,    RW::r, PC_inc::y, MOPC::nmop         ),
            MOP(R16::pc,   R8::d,    RW::r, PC_inc::y, MOPC::nmop         ),
            MOP(R16::a1,   R8::d,    RW::r, PC_inc::n, MOPC::nmop         ),
            MOP(R16::a2,   R8::d,    RW::r, PC_inc::n, MOPC::nmop         ),
            MOP(R16::a3,   R8::d,    RW::r, PC_inc::n, MOPC::nmop         ),
            MOP(R16::a4,   R8::pcl,  RW::r, PC_inc::n, MOPC::reset        ),
            MOP(R16::a1,   R8::pch,  RW::r, PC_inc::n, MOPC::nmop         ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch_brk ),
            // MOP(0,         0,        0,     0,         MOPC::__end ),
        };
        static const MOP rti[] = {
            MOP(R16::pc,   R8::d,    RW::r, PC_inc::n, MOPC::nmop     ),
            MOP(R16::spf,  R8::d,    RW::r, PC_inc::n, MOPC::rti      ),
            MOP(R16::a1,   R8::p,    RW::r, PC_inc::n, MOPC::nmop     ),
            MOP(R16::a2,   R8::pcl,  RW::r, PC_inc::n, MOPC::nmop     ),
            MOP(R16::spf,  R8::pch,  RW::r, PC_inc::n, MOPC::nmop     ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
            // MOP(0,         0,        0,     0,         MOPC::__end ),
        };
        static const MOP jmp_abs[] = {
            MOP(R16::pc,   R8::a1l,  RW::r, PC_inc::y, MOPC::nmop     ),
            MOP(R16::pc,   R8::a1h,  RW::r, PC_inc::n, MOPC::jmp_abs  ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
            // MOP(0,         0,        0,     0,         MOPC::__end ),
        };
        static const MOP jmp_ind[] = {
            MOP(R16::pc,   R8::a1l,  RW::r, PC_inc::y, MOPC::nmop     ),
            MOP(R16::pc,   R8::a1h,  RW::r, PC_inc::y, MOPC::nmop     ),
            MOP(R16::a1,   R8::pcl,  RW::r, PC_inc::n, MOPC::jmp_ind  ),
            MOP(R16::a1,   R8::pch,  RW::r, PC_inc::n, MOPC::nmop     ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
            // MOP(0,         0,        0,     0,         MOPC::__end ),
        };
        static const MOP rts[] = {
            MOP(R16::pc,   R8::d,    RW::r, PC_inc::n, MOPC::nmop     ),
            MOP(R16::spf,  R8::d,    RW::r, PC_inc::n, MOPC::rts      ),
            MOP(R16::a1,   R8::pcl,  RW::r, PC_inc::n, MOPC::nmop     ),
            MOP(R16::spf,  R8::pch,  RW::r, PC_inc::n, MOPC::nmop     ),
            MOP(R16::pc,   R8::d,    RW::r, PC_inc::y, MOPC::nmop     ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
            // MOP(0,         0,        0,     0,         MOPC::__end ),
        };
        static const MOP bra[] = {
            /* NOTE: branch taken, no page crossing -> request has to come earlier
                       http://visual6502.org/wiki/index.php?title=6502_State_Machine
            */
            MOP(R16::pc,   R8::d,    RW::r, PC_inc::y, MOPC::bra         ),    // T1
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch    ),    // T2 (no branch)
            MOP(R16::a2,   R8::d,    RW::r, PC_inc::n, MOPC::hold_ints   ),    // T2 (no pg.crs)
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch    ),    // T3
            MOP(R16::a2,   R8::d,    RW::r, PC_inc::n, MOPC::nmop        ),    // T2 (pg.crs)
            MOP(R16::a1,   R8::d,    RW::r, PC_inc::n, MOPC::nmop        ),    // T3
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch    ),    // T0
            // MOP(0,         0,        0,     0,         MOPC::__end    ),
        };
    }
    namespace UD {
        static const MOP rmw_abs_y[] = {
            MOP(R16::pc,   R8::a1l,  RW::r, PC_inc::y, MOPC::nmop     ),
            MOP(R16::pc,   R8::a1h,  RW::r, PC_inc::y, MOPC::abs_y_rmw),
            MOP(R16::a1,   R8::d,    RW::r, PC_inc::n, MOPC::nmop     ),
            MOP(R16::a2,   R8::d,    RW::r, PC_inc::n, MOPC::nmop     ),
            MOP(R16::a2,   R8::d,    RW::w, PC_inc::n, MOPC::do_op    ),
            MOP(R16::a2,   R8::d,    RW::w, PC_inc::n, MOPC::nmop     ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
            // MOP(0,         0,        0,     0,         MOPC::__end ),
        };
        static const MOP rmw_idx_ind[] = {
            MOP(R16::pc,   R8::zpa,  RW::r, PC_inc::y, MOPC::nmop     ),
            MOP(R16::zpaf, R8::d,    RW::r, PC_inc::n, MOPC::idx_ind  ),
            MOP(R16::zpaf, R8::a1l,  RW::r, PC_inc::n, MOPC::nmop     ),
            MOP(R16::a2,   R8::a1h,  RW::r, PC_inc::n, MOPC::nmop     ),
            MOP(R16::a1,   R8::d,    RW::r, PC_inc::n, MOPC::nmop     ),
            MOP(R16::a1,   R8::d,    RW::w, PC_inc::n, MOPC::do_op    ),
            MOP(R16::a1,   R8::d,    RW::w, PC_inc::n, MOPC::nmop     ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
            // MOP(0,         0,        0,     0,         MOPC::__end ),
        };
        static const MOP rmw_ind_idx[] = {
            MOP(R16::pc,   R8::zpa,  RW::r, PC_inc::y, MOPC::nmop     ),
            MOP(R16::zpaf, R8::a1l,  RW::r, PC_inc::n, MOPC::inc_zpa  ),
            MOP(R16::zpaf, R8::a1h,  RW::r, PC_inc::n, MOPC::st_ind_y ),
            MOP(R16::a1,   R8::d,    RW::r, PC_inc::n, MOPC::nmop     ),
            MOP(R16::a2,   R8::d,    RW::r, PC_inc::n, MOPC::nmop     ),
            MOP(R16::a2,   R8::d,    RW::w, PC_inc::n, MOPC::do_op    ),
            MOP(R16::a2,   R8::d,    RW::w, PC_inc::n, MOPC::nmop     ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
            // MOP(0,         0,        0,     0,         MOPC::__end ),
        };
        static const MOP st_abs_x[] = {
            MOP(R16::pc,   R8::a1l,  RW::r, PC_inc::y, MOPC::nmop     ),
            MOP(R16::pc,   R8::a1h,  RW::r, PC_inc::y, MOPC::st_abs_x ),
            MOP(R16::a1,   R8::d,    RW::r, PC_inc::n, MOPC::do_op    ),
            MOP(R16::a2,   R8::d,    RW::w, PC_inc::n, MOPC::nmop     ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
            // MOP(0,         0,        0,     0,         MOPC::__end ),
        };
        static const MOP st_abs_y[] = {
            MOP(R16::pc,   R8::a1l,  RW::r, PC_inc::y, MOPC::nmop     ),
            MOP(R16::pc,   R8::a1h,  RW::r, PC_inc::y, MOPC::st_abs_y ),
            MOP(R16::a1,   R8::d,    RW::r, PC_inc::n, MOPC::do_op    ),
            MOP(R16::a2,   R8::d,    RW::w, PC_inc::n, MOPC::nmop     ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
            // MOP(0,         0,        0,     0,         MOPC::__end ),
        };
        static const MOP st_ind_y[] = {
            MOP(R16::pc,   R8::zpa,  RW::r, PC_inc::y, MOPC::nmop     ),
            MOP(R16::zpaf, R8::a1l,  RW::r, PC_inc::n, MOPC::inc_zpa  ),
            MOP(R16::zpaf, R8::a1h,  RW::r, PC_inc::n, MOPC::st_ind_y ),
            MOP(R16::a1,   R8::d,    RW::r, PC_inc::n, MOPC::do_op    ),
            MOP(R16::a2,   R8::d,    RW::w, PC_inc::n, MOPC::nmop     ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
            // MOP(0,         0,        0,     0,         MOPC::__end ),
        };
        static const MOP hlt[] = {
            MOP(R16::pc,   R8::d,    RW::r, PC_inc::n, MOPC::nmop     ),
            MOP(R16::pc,   R8::d,    RW::r, PC_inc::n, MOPC::nmop     ), // TODO:
            MOP(R16::pc,   R8::d,    RW::r, PC_inc::n, MOPC::nmop     ), // correct addr
            MOP(R16::pc,   R8::d,    RW::r, PC_inc::n, MOPC::sig_hlt  ), // for these
            MOP(R16::pc,   R8::d,    RW::r, PC_inc::n, MOPC::hlt      ), // 4 cycles
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
            // MOP(0,         0,        0,     0,         MOPC::__end ),
        };
    }

    // instruction (opc) to micro-code (mc) mapping
    static const MOP* OPC_MC[] = {
        // 0x00
        FC::brk, RM::idx_ind, UD::hlt, UD::rmw_idx_ind, RM::zp, RM::zp, RMW::zp, RMW::zp,
        FC::php, RM::imm, SB::sb, RM::imm, RM::abs, RM::abs, RMW::abs, RMW::abs,
        // 0x10
        FC::bra, RM::ind_idx, UD::hlt, UD::rmw_ind_idx, RM::zp_x, RM::zp_x, RMW::zp_x, RMW::zp_x,
        SB::sb, RM::abs_y, SB::sb, UD::rmw_abs_y, RM::abs_x, RM::abs_x, RMW::abs_x, RMW::abs_x,
        // 0x20
        FC::jsr, RM::idx_ind, UD::hlt, UD::rmw_idx_ind, RM::zp, RM::zp, RMW::zp, RMW::zp,
        FC::plp, RM::imm, SB::sb, RM::imm, RM::abs, RM::abs, RMW::abs, RMW::abs,
        // 0x30
        FC::bra, RM::ind_idx, UD::hlt, UD::rmw_ind_idx, RM::zp_x, RM::zp_x, RMW::zp_x, RMW::zp_x,
        SB::sb, RM::abs_y, SB::sb, UD::rmw_abs_y, RM::abs_x, RM::abs_x, RMW::abs_x, RMW::abs_x,
        // 0x40
        FC::rti, RM::idx_ind, UD::hlt, UD::rmw_idx_ind, RM::zp, RM::zp, RMW::zp, RMW::zp,
        FC::pha, RM::imm, SB::sb, RM::imm, FC::jmp_abs, RM::abs, RMW::abs, RMW::abs,
        // 0x50
        FC::bra, RM::ind_idx, UD::hlt, UD::rmw_ind_idx, RM::zp_x, RM::zp_x, RMW::zp_x, RMW::zp_x,
        SB::sb_cli, RM::abs_y, SB::sb, UD::rmw_abs_y, RM::abs_x, RM::abs_x, RMW::abs_x, RMW::abs_x,
        // 0x60
        FC::rts, RM::idx_ind, UD::hlt, UD::rmw_idx_ind, RM::zp, RM::zp, RMW::zp, RMW::zp,
        FC::pla, RM::imm, SB::sb, RM::imm, FC::jmp_ind, RM::abs, RMW::abs, RMW::abs,
        // 0x70
        FC::bra, RM::ind_idx, UD::hlt, UD::rmw_ind_idx, RM::zp_x, RM::zp_x, RMW::zp_x, RMW::zp_x,
        SB::sb_sei, RM::abs_y, SB::sb, UD::rmw_abs_y, RM::abs_x, RM::abs_x, RMW::abs_x, RMW::abs_x,
        // 0x80
        RM::imm, ST::idx_ind, RM::imm, ST::idx_ind, ST::zp, ST::zp, ST::zp, ST::zp,
        SB::sb, RM::imm, SB::sb, RM::imm, ST::abs, ST::abs, ST::abs, ST::abs,
        // 0x90
        FC::bra, ST::ind_y, UD::hlt, UD::st_ind_y, ST::zp_x, ST::zp_x, ST::zp_y, ST::zp_y,
        SB::sb, ST::abs_y, SB::sb, UD::st_abs_y, UD::st_abs_x, ST::abs_x, UD::st_abs_y, UD::st_abs_y,
        // 0xa0
        RM::imm, RM::idx_ind, RM::imm, RM::idx_ind, RM::zp, RM::zp, RM::zp, RM::zp,
        SB::sb, RM::imm, SB::sb, RM::imm, RM::abs, RM::abs, RM::abs, RM::abs,
        // 0xb0
        FC::bra, RM::ind_idx, UD::hlt, RM::ind_idx, RM::zp_x, RM::zp_x, RM::zp_y, RM::zp_y,
        SB::sb, RM::abs_y, SB::sb, RM::abs_y, RM::abs_x, RM::abs_x, RM::abs_y, RM::abs_y,
        // 0xc0
        RM::imm, RM::idx_ind, RM::imm, UD::rmw_idx_ind, RM::zp, RM::zp, RMW::zp, RMW::zp,
        SB::sb, RM::imm, SB::sb, RM::imm, RM::abs, RM::abs, RMW::abs, RMW::abs,
        // 0xd0
        FC::bra, RM::ind_idx, UD::hlt, UD::rmw_ind_idx, RM::zp_x, RM::zp_x, RMW::zp_x, RMW::zp_x,
        SB::sb, RM::abs_y, SB::sb, UD::rmw_abs_y, RM::abs_x, RM::abs_x, RMW::abs_x, RMW::abs_x,
        // 0xe0
        RM::imm, RM::idx_ind, RM::imm, UD::rmw_idx_ind, RM::zp, RM::zp, RMW::zp, RMW::zp,
        SB::sb, RM::imm, SB::sb, RM::imm, RM::abs, RM::abs, RMW::abs, RMW::abs,
        // 0xf0
        FC::bra, RM::ind_idx, UD::hlt, UD::rmw_ind_idx, RM::zp_x, RM::zp_x, RMW::zp_x, RMW::zp_x,
        SB::sb, RM::abs_y, SB::sb, UD::rmw_abs_y, RM::abs_x, RM::abs_x, RMW::abs_x, RMW::abs_x,
    };
} // namespace _MC

/*
static bool mc_init = false;

static NMOS6502::MOP* mc;
static NMOS6502::u8*  opc_mc_ptr;

void init_mc() {
    if (mc_init) return;

    std::vector<NMOS6502::MOP> MC;
    std::vector<NMOS6502::u8> OPC_MC_PTR;
    std::map<const NMOS6502::MOP*, NMOS6502::u8> MC_PTR;

    auto add_mc = [&](const NMOS6502::MOP* mc) {
        NMOS6502::u8 mp = MC.size();
        const NMOS6502::MOP* _mc = mc;
        while (_mc->mopc != NMOS6502::__end) {
            MC.push_back(*_mc);
            ++_mc;
        }
        MC_PTR[mc] = mp;
        return mp;
    };

    auto get_mc_ptr = [&](const NMOS6502::MOP* mc) {
        auto mp = MC_PTR.find(mc);
        if (mp != MC_PTR.end()) return mp->second;
        else return add_mc(mc);
    };

    add_mc(_MC::FC::reset);

    for (int opc = 0; opc < 0x100; ++opc) {
        OPC_MC_PTR.push_back(get_mc_ptr(_MC::OPC_MC[opc]));
    }

    mc = new NMOS6502::MOP[MC.size()];
    opc_mc_ptr = new NMOS6502::u8[OPC_MC_PTR.size()];

    std::copy(MC.begin(), MC.end(), mc);
    std::copy(OPC_MC_PTR.begin(), OPC_MC_PTR.end(), opc_mc_ptr);

    mc_init = true;
}

const NMOS6502::MOP* NMOS6502::MC::get_mc() {
    init_mc();
    return mc;
}
*/

//const NMOS6502::u8* NMOS6502::MC::get_opc_mc_ptr() {
const NMOS6502::MOP** NMOS6502::MC::get_opc_mc_ptr() {
    //init_mc();
    //return opc_mc_ptr;
    return _MC::OPC_MC;
}

const NMOS6502::MOP* NMOS6502::MC::get_reset_mc() { return _MC::FC::reset; }
