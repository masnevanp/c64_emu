#ifndef NMOS6502_MC_H_INCLUDED
#define NMOS6502_MC_H_INCLUDED

#include "nmos6502.h"


// Micro-code

namespace NMOS6502 {
    namespace MC {
        enum MOPC : u8 { // micro-op code
            nmop = 0, abs_x, inc_zpa, abs_y, rm_zp_x, rm_zp_y, rm_x, rm_y,
            rm_idx_ind, a_nz, do_op, st_zp_x, st_zp_y, st_idx_ind, st_reg,
            jmp_ind, bpl, bmi, bvc, bvs, bcc, bcs, beq, bne,
            hold_ints, php, pha, jsr, jmp_abs, rti, rts, inc_sp, brk,
            dispatch_cli, dispatch_sei, dispatch, dispatch_brk,
            sig_hlt, hlt, reset
        };
        extern const std::string MOPC_str[];

        using PC_inc = u8;
        extern const std::string PC_inc_str[];

        // mem-op. direction
        enum RW : u8 { w = 0, r = 1 };
        extern const std::string RW_str[];

        struct MOP { // micro-op
            Ri16 ar;
            Ri8 dr;
            RW rw;
            PC_inc pc_inc;
            MOPC mopc;

            MOP(Ri16 ar_, Ri8 dr_, RW rw_, PC_inc pc_inc_, MOPC mopc_)
                : ar(ar_), dr(dr_), rw(rw_), pc_inc(pc_inc_), mopc(mopc_) {}

            operator std::string() const {
                return "(" + Ri16_str[ar] + ") " + Ri8_str[dr] + " " + RW_str[rw]
                        + PC_inc_str[pc_inc] + " " + MOPC_str[mopc];
            }
        };

        extern const MOP** OPC_MC;   // map: opc -> micro-code (reset @ OPC_MC[0x100])
    } // namespace MC

} // namespace NMOS6502


#endif // NMOS6502_MC_H_INCLUDED
