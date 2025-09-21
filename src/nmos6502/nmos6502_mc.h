#ifndef NMOS6502_MC_H_INCLUDED
#define NMOS6502_MC_H_INCLUDED

#include "nmos6502.h"


// Micro-code

namespace NMOS6502::MC {

    enum MOPC : u8 { // micro-op code
        abs_x = 0, inc_zp, abs_y, rm_zp_x, rm_zp_y, rm_x, rm_y,
        rm_idx_ind, a_nz, do_op, st_zp_x, st_zp_y, st_idx_ind, st_reg,
        jmp_ind, bra, hold_ints, php, pha, jsr, jmp_abs, rti, rts, inc_sp, brk,
        dispatch_cli, dispatch_sei, dispatch, dispatch_brk,
        sig_hlt, hlt, reset, nmop
    };
    extern const std::string MOPC_str[];

    using PC_inc = u8;
    extern const std::string PC_inc_str[];

    // mem-op. direction
    enum RW : u8 { w = 0, r = 1 };
    extern const std::string RW_str[];

    struct MOP { // micro-op
        const Ri16 ar;
        const Ri8 dr;
        const RW rw;
        const PC_inc pc_inc;
        const MOPC mopc;

        MOP(Ri16 a, Ri8 d, RW r, PC_inc p, MOPC m) : ar(a), dr(d), rw(r), pc_inc(p), mopc(m) {}

        operator std::string() const {
            return "(" + Ri16_str[ar] + ") " + Ri8_str[dr] + " " + RW_str[rw]
                    + PC_inc_str[pc_inc] + " " + MOPC_str[mopc];
        }
    };

    extern const MOP code[];
    extern const u8 opc_addr[]; // offset in code[]

    inline const MOP* opc_mc_start(u16 opc) { return &code[opc_addr[opc]]; }

} // namespace NMOS6502::MC


#endif // NMOS6502_MC_H_INCLUDED
