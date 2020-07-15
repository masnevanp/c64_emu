#ifndef NMOS6502_MC_H_INCLUDED
#define NMOS6502_MC_H_INCLUDED

#include "nmos6502.h"


// Micro-code

namespace NMOS6502 {

    namespace MC {
        enum MOPC : u8 { // micro-op code
            // General
            nmop = 0, dispatch, dispatch_brk, dispatch_cli, dispatch_sei, do_op,
            // Reg-Mem (RM)
            idx_ind, abs_x, abs_y, zp_x, zp_y, inc_zpa, ind_idx,
            // Store (ST)
            st_reg, st_abs_x, st_abs_y, st_idx_ind, st_zp_x, st_zp_y, st_ind_y,
            // Read-Modify-Write (RMW)
            abs_x_rmw,
            // Flow Control (FC)
            inc_sp, php, pha, pla, jsr, brk, rti, jmp_abs, jmp_ind, rts, bra, hold_ints,
            // Undoc/Undef (UD)
            abs_y_rmw, sig_hlt, hlt,
            //
            reset,
        };
        extern const std::string MOPC_str[];

        enum MSOPC : u8 { // micro-sub-op code
            // single byte instrs
            sb_asl, sb_clc, sb_cld, sb_clv, sb_dex, sb_dey, sb_inx, sb_iny, sb_lsr,
            sb_rol, sb_ror, sb_sec, sb_sed, sb_tax, sb_tay, sb_tsx, sb_txa, sb_txs, sb_tya,
            // reg-mem
            rm_adc, rm_and, rm_bit, rm_cmp, rm_cpx, rm_cpy, rm_eor, rm_lda, rm_ldx, rm_ldy,
            rm_ora, rm_sbc,
            // read-modify-write
            rmw_asl, rmw_dec, rmw_inc, rmw_lsr, rmw_rol, rmw_ror,
            // misc
            nop,
            // undocumented
            ud_ahx, ud_alr, ud_anc, ud_arr, ud_axs, ud_dcp, ud_isc, ud_las, ud_lax, ud_lxa,
            ud_rla, ud_rra, ud_shx, ud_shy, ud_slo, ud_sre, ud_tas, ud_xaa,
            // error
            err,
        };

        struct PC_inc { // due to a clash with R8....
            enum { n = 0, y = 1 };
        };
        extern const std::string PC_inc_str[];

        // mem-op. direction
        enum RW : u8 { w = 0, r = 1 };
        extern const std::string RW_str[];

        struct MOP { // micro-op
            unsigned char ar : 4;
            unsigned char dr : 4;
            unsigned char mopc : 6;
            unsigned char rw : 1;
            unsigned char pc_inc : 1;

            MOP(u8 ar_ = 0, u8 dr_ = 0, u8 rw_ = 0, u8 pc_inc_ = 0, u8 mopc_ = hlt)
            : ar(ar_), dr(dr_), mopc(mopc_), rw(rw_), pc_inc(pc_inc_) {}

            operator std::string() const {
                return "(" + R16_str[ar] + ") " + R8_str[dr] + " " + RW_str[rw]
                        + PC_inc_str[pc_inc] + " " + MOPC_str[mopc];
            }
        };

        extern const MOP** OPC_MC;   // map: opc -> micro-code (reset @ OPC_MC[0x100])
        extern const u8* OPC_MSOPC;  // map: opc -> micro-code sub-op
    } // namespace MC

} // namespace NMOS6502


#endif // NMOS6502_MC_H_INCLUDED
