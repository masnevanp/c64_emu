#ifndef NMOS6502_MC_H_INCLUDED
#define NMOS6502_MC_H_INCLUDED

#include "nmos6502.h"


// Micro-code

namespace NMOS6502 {
    namespace MC {
        // mem-op. direction
        enum RW : u8 { w = 0, r = 1, none = 0xff };
        extern const std::string RW_str[];

        using PC_inc = u8;
        extern const std::string PC_inc_str[];

        enum MOPC : u8 { // micro-op code
            nmop = 0, abs_x, inc_zpa, abs_y, rm_zp_x, rm_zp_y, rm_x, rm_y,
            rm_idx_ind, a_nz, do_op, st_zp_x, st_zp_y, st_idx_ind, st_reg,
            jmp_ind, bpl, bmi, bvc, bvs, bcc, bcs, beq, bne,
            hold_ints, php, pha, jsr, jmp_abs, rti, rts, inc_sp, brk,
            dispatch_cli, dispatch_sei, dispatch, dispatch_brk,
            sig_hlt, hlt, reset,
            _end
        };
        extern const std::string MOPC_str[];

        struct MOP { // micro-op
            // TODO: ar & dr as direct pointers to regs?
            Ri16 ar;
            Ri8 dr;
            RW rw;
            PC_inc pc_inc;
            MOPC mopc;

            MOP(Ri16 ar_ = Ri16::ri16_none, Ri8 dr_ = Ri8::ri8_none, RW rw_ = RW::none,
                    PC_inc pc_inc_ = 0, MOPC mopc_ = hlt)
                : ar(ar_), dr(dr_), rw(rw_), pc_inc(pc_inc_), mopc(mopc_) {}

            operator std::string() const {
                return "(" + Ri16_str[ar] + ") " + Ri8_str[dr] + " " + RW_str[rw]
                        + PC_inc_str[pc_inc] + " " + MOPC_str[mopc];
            }
        };

        extern const MOP* code;
        extern const int* opc_addr; // code offset

    } // namespace MC

} // namespace NMOS6502


#endif // NMOS6502_MC_H_INCLUDED
