
#include "nmos6502_mc.h"


using namespace NMOS6502;


const std::string MC::MOPC_str[] = { // TODO
    "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "",
};

const std::string MC::PC_inc_str[] = { ", ", ", inc(pc), " };

const std::string MC::RW_str[] = { "w", "r" };


namespace _MC { // micro-code: 1..n micro-ops/instr (1 micro-op/1 cycle)
    using MC::MOP;
    using MC::MOPC;
    using MC::PC_inc;
    using MC::RW;
    using MC::MSOPC;

    namespace SB {
        static const MOP sb[] = {
            MOP(R16::pc,   R8::d,    RW::r, PC_inc::n, MOPC::do_op    ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
        };
    }
    namespace RM {
        static const MOP zp[] = {
            MOP(R16::pc,   R8::zpa,  RW::r, PC_inc::y, MOPC::nmop     ),
            MOP(R16::zpaf, R8::d,    RW::r, PC_inc::n, MOPC::do_op    ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
        };
        static const MOP imm[] = {
            MOP(R16::pc,   R8::d,    RW::r, PC_inc::y, MOPC::do_op    ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
        };
        static const MOP abs[] = {
            MOP(R16::pc,   R8::a1l,  RW::r, PC_inc::y, MOPC::nmop     ),
            MOP(R16::pc,   R8::a1h,  RW::r, PC_inc::y, MOPC::nmop     ),
            MOP(R16::a1,   R8::d,    RW::r, PC_inc::n, MOPC::do_op    ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
        };
        static const MOP zp_x[] = {
            MOP(R16::pc,   R8::zpa,  RW::r, PC_inc::y, MOPC::nmop     ),
            MOP(R16::zpaf, R8::d,    RW::r, PC_inc::n, MOPC::rm_zp_x  ),
            MOP(R16::zpaf, R8::d,    RW::r, PC_inc::n, MOPC::do_op    ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
        };
        static const MOP zp_y[] = {
            MOP(R16::pc,   R8::zpa,  RW::r, PC_inc::y, MOPC::nmop     ),
            MOP(R16::zpaf, R8::d,    RW::r, PC_inc::n, MOPC::rm_zp_y  ),
            MOP(R16::zpaf, R8::d,    RW::r, PC_inc::n, MOPC::do_op    ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
        };
        static const MOP idx_ind[] = {
            MOP(R16::pc,   R8::zpa,  RW::r, PC_inc::y, MOPC::nmop     ),
            MOP(R16::zpaf, R8::d,    RW::r, PC_inc::n, MOPC::rm_idx_ind),
            MOP(R16::zpaf, R8::a1l,  RW::r, PC_inc::n, MOPC::nmop     ),
            MOP(R16::a2,   R8::a1h,  RW::r, PC_inc::n, MOPC::nmop     ),
            MOP(R16::a1,   R8::d,    RW::r, PC_inc::n, MOPC::do_op    ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
        };
        static const MOP abs_x[] = {
            MOP(R16::pc,   R8::a1l,  RW::r, PC_inc::y, MOPC::nmop     ),
            MOP(R16::pc,   R8::a1h,  RW::r, PC_inc::y, MOPC::rm_x     ),
            MOP(R16::a1,   R8::d,    RW::r, PC_inc::n, MOPC::do_op    ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
            MOP(R16::a1,   R8::d,    RW::r, PC_inc::n, MOPC::nmop     ),
            MOP(R16::a2,   R8::d,    RW::r, PC_inc::n, MOPC::do_op    ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
        };
        static const MOP abs_y[] = {
            MOP(R16::pc,   R8::a1l,  RW::r, PC_inc::y, MOPC::nmop     ),
            MOP(R16::pc,   R8::a1h,  RW::r, PC_inc::y, MOPC::rm_y     ),
            MOP(R16::a1,   R8::d,    RW::r, PC_inc::n, MOPC::do_op    ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
            MOP(R16::a1,   R8::d,    RW::r, PC_inc::n, MOPC::nmop     ),
            MOP(R16::a2,   R8::d,    RW::r, PC_inc::n, MOPC::do_op    ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
        };
        static const MOP ind_idx[] = {
            MOP(R16::pc,   R8::zpa,  RW::r, PC_inc::y, MOPC::nmop     ),
            MOP(R16::zpaf, R8::a1l,  RW::r, PC_inc::n, MOPC::inc_zpa  ),
            MOP(R16::zpaf, R8::a1h,  RW::r, PC_inc::n, MOPC::rm_y     ),
            MOP(R16::a1,   R8::d,    RW::r, PC_inc::n, MOPC::do_op    ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
            MOP(R16::a1,   R8::d,    RW::r, PC_inc::n, MOPC::nmop     ),
            MOP(R16::a2,   R8::d,    RW::r, PC_inc::n, MOPC::do_op    ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
        };
    }
    namespace ST {
        static const MOP zp[] = {
            MOP(R16::pc,   R8::zpa,  RW::r, PC_inc::y, MOPC::st_reg   ),
            MOP(R16::zpaf, R8::d,    RW::w, PC_inc::n, MOPC::nmop     ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
        };
        static const MOP abs[] = {
            MOP(R16::pc,   R8::a1l,  RW::r, PC_inc::y, MOPC::nmop     ),
            MOP(R16::pc,   R8::a1h,  RW::r, PC_inc::y, MOPC::st_reg   ),
            MOP(R16::a1,   R8::d,    RW::w, PC_inc::n, MOPC::nmop     ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
        };
        static const MOP zp_x[] = {
            MOP(R16::pc,   R8::zpa,  RW::r, PC_inc::y, MOPC::nmop     ),
            MOP(R16::zpaf, R8::d,    RW::r, PC_inc::n, MOPC::st_zp_x  ),
            MOP(R16::zpaf, R8::d,    RW::w, PC_inc::n, MOPC::nmop     ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
        };
        static const MOP zp_y[] = {
            MOP(R16::pc,   R8::zpa,  RW::r, PC_inc::y, MOPC::nmop     ),
            MOP(R16::zpaf, R8::d,    RW::r, PC_inc::n, MOPC::st_zp_y  ),
            MOP(R16::zpaf, R8::d,    RW::w, PC_inc::n, MOPC::nmop     ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
        };
        static const MOP idx_ind[] = {
            MOP(R16::pc,   R8::zpa,  RW::r, PC_inc::y, MOPC::nmop     ),
            MOP(R16::zpaf, R8::d,    RW::r, PC_inc::n, MOPC::st_idx_ind),
            MOP(R16::zpaf, R8::a1l,  RW::r, PC_inc::n, MOPC::nmop     ),
            MOP(R16::a2,   R8::a1h,  RW::r, PC_inc::n, MOPC::nmop     ),
            MOP(R16::a1,   R8::d,    RW::w, PC_inc::n, MOPC::nmop     ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
        };
        static const MOP abs_x[] = {
            MOP(R16::pc,   R8::a1l,  RW::r, PC_inc::y, MOPC::nmop     ),
            MOP(R16::pc,   R8::a1h,  RW::r, PC_inc::y, MOPC::abs_x    ),
            MOP(R16::a1,   R8::d,    RW::r, PC_inc::n, MOPC::nmop     ),
            MOP(R16::a2,   R8::a,    RW::w, PC_inc::n, MOPC::nmop     ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
        };
        static const MOP abs_y[] = {
            MOP(R16::pc,   R8::a1l,  RW::r, PC_inc::y, MOPC::nmop     ),
            MOP(R16::pc,   R8::a1h,  RW::r, PC_inc::y, MOPC::abs_y    ),
            MOP(R16::a1,   R8::d,    RW::r, PC_inc::n, MOPC::nmop     ),
            MOP(R16::a2,   R8::a,    RW::w, PC_inc::n, MOPC::nmop     ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
        };
        static const MOP ind_y[] = {
            MOP(R16::pc,   R8::zpa,  RW::r, PC_inc::y, MOPC::nmop     ),
            MOP(R16::zpaf, R8::a1l,  RW::r, PC_inc::n, MOPC::inc_zpa  ),
            MOP(R16::zpaf, R8::a1h,  RW::r, PC_inc::n, MOPC::abs_y    ),
            MOP(R16::a1,   R8::d,    RW::r, PC_inc::n, MOPC::nmop     ),
            MOP(R16::a2,   R8::a,    RW::w, PC_inc::n, MOPC::nmop     ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
        };
    }
    namespace RMW {
        static const MOP zp[] = {
            MOP(R16::pc,   R8::zpa,  RW::r, PC_inc::y, MOPC::nmop     ),
            MOP(R16::zpaf, R8::d,    RW::r, PC_inc::n, MOPC::nmop     ),
            MOP(R16::zpaf, R8::d,    RW::w, PC_inc::n, MOPC::do_op    ),
            MOP(R16::zpaf, R8::d,    RW::w, PC_inc::n, MOPC::nmop     ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
        };
        static const MOP abs[] = {
            MOP(R16::pc,   R8::a1l,  RW::r, PC_inc::y, MOPC::nmop     ),
            MOP(R16::pc,   R8::a1h,  RW::r, PC_inc::y, MOPC::nmop     ),
            MOP(R16::a1,   R8::d,    RW::r, PC_inc::n, MOPC::nmop     ),
            MOP(R16::a1,   R8::d,    RW::w, PC_inc::n, MOPC::do_op    ),
            MOP(R16::a1,   R8::d,    RW::w, PC_inc::n, MOPC::nmop     ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
        };
        static const MOP zp_x[] = {
            MOP(R16::pc,   R8::zpa,  RW::r, PC_inc::y, MOPC::nmop     ),
            MOP(R16::zpaf, R8::d,    RW::r, PC_inc::n, MOPC::rm_zp_x  ),
            MOP(R16::zpaf, R8::d,    RW::r, PC_inc::n, MOPC::nmop     ),
            MOP(R16::zpaf, R8::d,    RW::w, PC_inc::n, MOPC::do_op    ),
            MOP(R16::zpaf, R8::d,    RW::w, PC_inc::n, MOPC::nmop     ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
        };
        static const MOP abs_x[] = {
            MOP(R16::pc,   R8::a1l,  RW::r, PC_inc::y, MOPC::nmop     ),
            MOP(R16::pc,   R8::a1h,  RW::r, PC_inc::y, MOPC::abs_x    ),
            MOP(R16::a1,   R8::d,    RW::r, PC_inc::n, MOPC::nmop     ),
            MOP(R16::a2,   R8::d,    RW::r, PC_inc::n, MOPC::nmop     ),
            MOP(R16::a2,   R8::d,    RW::w, PC_inc::n, MOPC::do_op    ),
            MOP(R16::a2,   R8::d,    RW::w, PC_inc::n, MOPC::nmop     ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
        };
    }
    namespace FC {
        static const MOP pha[] = {
            MOP(R16::pc,   R8::d,    RW::r, PC_inc::n, MOPC::pha      ),
            MOP(R16::a1,   R8::a,    RW::w, PC_inc::n, MOPC::nmop     ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
        };
        static const MOP php[] = {
            MOP(R16::pc,   R8::d,    RW::r, PC_inc::n, MOPC::php      ),
            MOP(R16::a1,   R8::p,    RW::w, PC_inc::n, MOPC::nmop     ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
        };
        static const MOP pla[] = {
            MOP(R16::pc,   R8::d,    RW::r, PC_inc::n, MOPC::nmop     ),
            MOP(R16::spf,  R8::d,    RW::r, PC_inc::n, MOPC::inc_sp   ),
            MOP(R16::spf,  R8::a,    RW::r, PC_inc::n, MOPC::a_nz     ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
        };
        static const MOP plp[] = {
            MOP(R16::pc,   R8::d,    RW::r, PC_inc::n, MOPC::nmop     ),
            MOP(R16::spf,  R8::d,    RW::r, PC_inc::n, MOPC::inc_sp   ),
            MOP(R16::spf,  R8::p,    RW::r, PC_inc::n, MOPC::nmop     ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
        };
        static const MOP jsr[] = {
            MOP(R16::pc,   R8::a1l,  RW::r, PC_inc::y, MOPC::nmop     ),
            MOP(R16::spf,  R8::d,    RW::r, PC_inc::n, MOPC::jsr      ),
            MOP(R16::a3,   R8::a2h,  RW::w, PC_inc::n, MOPC::nmop     ),
            MOP(R16::a4,   R8::a2l,  RW::w, PC_inc::n, MOPC::nmop     ),
            MOP(R16::a2,   R8::pch,  RW::r, PC_inc::n, MOPC::nmop     ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
        };
        static const MOP bra[] = {
            // these entry MOPs evaluate the relevant branching condition,
            // and then jump to the main code below (even if branch not taken)
            MOP(R16::pc,   R8::d,    RW::r, PC_inc::y, MOPC::bpl      ),  // T1
            MOP(R16::pc,   R8::d,    RW::r, PC_inc::y, MOPC::bmi      ),  // T1
            MOP(R16::pc,   R8::d,    RW::r, PC_inc::y, MOPC::bvc      ),  // T1
            MOP(R16::pc,   R8::d,    RW::r, PC_inc::y, MOPC::bvs      ),  // T1
            MOP(R16::pc,   R8::d,    RW::r, PC_inc::y, MOPC::bcc      ),  // T1
            MOP(R16::pc,   R8::d,    RW::r, PC_inc::y, MOPC::bcs      ),  // T1
            MOP(R16::pc,   R8::d,    RW::r, PC_inc::y, MOPC::beq      ),  // T1
            MOP(R16::pc,   R8::d,    RW::r, PC_inc::y, MOPC::bne      ),  // T1
            // NOTE: branch taken, no page crossing -> request has to come earlier
            //       http://visual6502.org/wiki/index.php?title=6502_State_Machine
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),  // T2 (no branch)
            MOP(R16::a2,   R8::d,    RW::r, PC_inc::n, MOPC::hold_ints),  // T2 (no pg.crs)
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),  // T3
            MOP(R16::a2,   R8::d,    RW::r, PC_inc::n, MOPC::nmop     ),  // T2 (pg.crs)
            MOP(R16::a1,   R8::d,    RW::r, PC_inc::n, MOPC::nmop     ),  // T3
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),  // T0
        };
        static const MOP brk[] = {
            MOP(R16::pc,   R8::d,    RW::r, PC_inc::n, MOPC::nmop     ),
            MOP(R16::a2,   R8::a1h,  RW::w, PC_inc::n, MOPC::nmop     ),
            MOP(R16::a3,   R8::a1l,  RW::w, PC_inc::n, MOPC::nmop     ),
            MOP(R16::a4,   R8::p,    RW::w, PC_inc::n, MOPC::brk      ),
            MOP(R16::a2,   R8::pcl,  RW::r, PC_inc::n, MOPC::nmop     ),
            MOP(R16::a3,   R8::pch,  RW::r, PC_inc::n, MOPC::nmop     ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch_brk),
        };
        static const MOP rti[] = {
            MOP(R16::pc,   R8::d,    RW::r, PC_inc::n, MOPC::nmop     ),
            MOP(R16::spf,  R8::d,    RW::r, PC_inc::n, MOPC::rti      ),
            MOP(R16::a1,   R8::p,    RW::r, PC_inc::n, MOPC::nmop     ),
            MOP(R16::a2,   R8::pcl,  RW::r, PC_inc::n, MOPC::nmop     ),
            MOP(R16::spf,  R8::pch,  RW::r, PC_inc::n, MOPC::nmop     ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
        };
        static const MOP jmp_abs[] = {
            MOP(R16::pc,   R8::a1l,  RW::r, PC_inc::y, MOPC::nmop     ),
            MOP(R16::pc,   R8::a1h,  RW::r, PC_inc::n, MOPC::jmp_abs  ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
        };
        static const MOP jmp_ind[] = {
            MOP(R16::pc,   R8::a1l,  RW::r, PC_inc::y, MOPC::nmop     ),
            MOP(R16::pc,   R8::a1h,  RW::r, PC_inc::y, MOPC::nmop     ),
            MOP(R16::a1,   R8::pcl,  RW::r, PC_inc::n, MOPC::jmp_ind  ),
            MOP(R16::a1,   R8::pch,  RW::r, PC_inc::n, MOPC::nmop     ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
        };
        static const MOP rts[] = {
            MOP(R16::pc,   R8::d,    RW::r, PC_inc::n, MOPC::nmop     ),
            MOP(R16::spf,  R8::d,    RW::r, PC_inc::n, MOPC::rts      ),
            MOP(R16::a2,   R8::pcl,  RW::r, PC_inc::n, MOPC::nmop     ),
            MOP(R16::spf,  R8::pch,  RW::r, PC_inc::n, MOPC::nmop     ),
            MOP(R16::pc,   R8::d,    RW::r, PC_inc::y, MOPC::nmop     ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
        };
    }
    namespace SB {
        /* cli&sei feature: change not be visible at next T0 (dispatch)
            (http://visual6502.org/wiki/index.php?title=6502_Timing_of_Interrupt_Handling)
        */
        static const MOP sb_cli[] = {
            MOP(R16::pc,   R8::d,    RW::r, PC_inc::n, MOPC::nmop         ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch_cli ),
        };
        static const MOP sb_sei[] = {
            MOP(R16::pc,   R8::d,    RW::r, PC_inc::n, MOPC::nmop         ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch_sei ),
        };
    }
    namespace UD {
        static const MOP rmw_abs_y[] = {
            MOP(R16::pc,   R8::a1l,  RW::r, PC_inc::y, MOPC::nmop     ),
            MOP(R16::pc,   R8::a1h,  RW::r, PC_inc::y, MOPC::abs_y    ),
            MOP(R16::a1,   R8::d,    RW::r, PC_inc::n, MOPC::nmop     ),
            MOP(R16::a2,   R8::d,    RW::r, PC_inc::n, MOPC::nmop     ),
            MOP(R16::a2,   R8::d,    RW::w, PC_inc::n, MOPC::do_op    ),
            MOP(R16::a2,   R8::d,    RW::w, PC_inc::n, MOPC::nmop     ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
        };
        static const MOP rmw_idx_ind[] = {
            MOP(R16::pc,   R8::zpa,  RW::r, PC_inc::y, MOPC::nmop     ),
            MOP(R16::zpaf, R8::d,    RW::r, PC_inc::n, MOPC::rm_idx_ind),
            MOP(R16::zpaf, R8::a1l,  RW::r, PC_inc::n, MOPC::nmop     ),
            MOP(R16::a2,   R8::a1h,  RW::r, PC_inc::n, MOPC::nmop     ),
            MOP(R16::a1,   R8::d,    RW::r, PC_inc::n, MOPC::nmop     ),
            MOP(R16::a1,   R8::d,    RW::w, PC_inc::n, MOPC::do_op    ),
            MOP(R16::a1,   R8::d,    RW::w, PC_inc::n, MOPC::nmop     ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
        };
        static const MOP rmw_ind_idx[] = {
            MOP(R16::pc,   R8::zpa,  RW::r, PC_inc::y, MOPC::nmop     ),
            MOP(R16::zpaf, R8::a1l,  RW::r, PC_inc::n, MOPC::inc_zpa  ),
            MOP(R16::zpaf, R8::a1h,  RW::r, PC_inc::n, MOPC::abs_y    ),
            MOP(R16::a1,   R8::d,    RW::r, PC_inc::n, MOPC::nmop     ),
            MOP(R16::a2,   R8::d,    RW::r, PC_inc::n, MOPC::nmop     ),
            MOP(R16::a2,   R8::d,    RW::w, PC_inc::n, MOPC::do_op    ),
            MOP(R16::a2,   R8::d,    RW::w, PC_inc::n, MOPC::nmop     ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
        };
        static const MOP st_abs_x[] = {
            MOP(R16::pc,   R8::a1l,  RW::r, PC_inc::y, MOPC::nmop     ),
            MOP(R16::pc,   R8::a1h,  RW::r, PC_inc::y, MOPC::abs_x    ),
            MOP(R16::a1,   R8::d,    RW::r, PC_inc::n, MOPC::do_op    ),
            MOP(R16::a2,   R8::d,    RW::w, PC_inc::n, MOPC::nmop     ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
        };
        static const MOP st_abs_y[] = {
            MOP(R16::pc,   R8::a1l,  RW::r, PC_inc::y, MOPC::nmop     ),
            MOP(R16::pc,   R8::a1h,  RW::r, PC_inc::y, MOPC::abs_y    ),
            MOP(R16::a1,   R8::d,    RW::r, PC_inc::n, MOPC::do_op    ),
            MOP(R16::a2,   R8::d,    RW::w, PC_inc::n, MOPC::nmop     ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
        };
        static const MOP st_ind_y[] = {
            MOP(R16::pc,   R8::zpa,  RW::r, PC_inc::y, MOPC::nmop     ),
            MOP(R16::zpaf, R8::a1l,  RW::r, PC_inc::n, MOPC::inc_zpa  ),
            MOP(R16::zpaf, R8::a1h,  RW::r, PC_inc::n, MOPC::abs_y    ),
            MOP(R16::a1,   R8::d,    RW::r, PC_inc::n, MOPC::do_op    ),
            MOP(R16::a2,   R8::d,    RW::w, PC_inc::n, MOPC::nmop     ),
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
        };
        static const MOP hlt[] = {
            MOP(R16::pc,   R8::d,    RW::r, PC_inc::y, MOPC::nmop     ),
            MOP(R16::pc,   R8::a1l,  RW::r, PC_inc::n, MOPC::nmop     ), // TODO:
            MOP(R16::pc,   R8::a1h,  RW::r, PC_inc::n, MOPC::nmop     ), // correct addr
            MOP(R16::pc,   R8::a1l,  RW::r, PC_inc::n, MOPC::sig_hlt  ), // for these
            MOP(R16::pc,   R8::a1h,  RW::r, PC_inc::n, MOPC::hlt      ), // 4 cycles
            MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch ),
        };
    }

    static const MOP reset[] = {
        MOP(R16::pc,   R8::d,    RW::r, PC_inc::y, MOPC::nmop         ),
        MOP(R16::pc,   R8::d,    RW::r, PC_inc::y, MOPC::nmop         ),
        MOP(R16::a1,   R8::d,    RW::r, PC_inc::n, MOPC::nmop         ),
        MOP(R16::a2,   R8::d,    RW::r, PC_inc::n, MOPC::nmop         ),
        MOP(R16::a3,   R8::d,    RW::r, PC_inc::n, MOPC::nmop         ),
        MOP(R16::a4,   R8::pcl,  RW::r, PC_inc::n, MOPC::reset        ),
        MOP(R16::a1,   R8::pch,  RW::r, PC_inc::n, MOPC::nmop         ),
        MOP(R16::pc,   R8::ir,   RW::r, PC_inc::y, MOPC::dispatch_brk ),
    };

    // instruction (opc) to micro-code (mc) mapping
    static const MOP* OPC_MC[] = {
        // 0x00
        FC::brk, RM::idx_ind, UD::hlt, UD::rmw_idx_ind, RM::zp, RM::zp, RMW::zp, RMW::zp,
        FC::php, RM::imm, SB::sb, RM::imm, RM::abs, RM::abs, RMW::abs, RMW::abs,
        // 0x10
        FC::bra + 0, RM::ind_idx, UD::hlt, UD::rmw_ind_idx, RM::zp_x, RM::zp_x, RMW::zp_x, RMW::zp_x,
        SB::sb, RM::abs_y, SB::sb, UD::rmw_abs_y, RM::abs_x, RM::abs_x, RMW::abs_x, RMW::abs_x,
        // 0x20
        FC::jsr, RM::idx_ind, UD::hlt, UD::rmw_idx_ind, RM::zp, RM::zp, RMW::zp, RMW::zp,
        FC::plp, RM::imm, SB::sb, RM::imm, RM::abs, RM::abs, RMW::abs, RMW::abs,
        // 0x30
        FC::bra + 1, RM::ind_idx, UD::hlt, UD::rmw_ind_idx, RM::zp_x, RM::zp_x, RMW::zp_x, RMW::zp_x,
        SB::sb, RM::abs_y, SB::sb, UD::rmw_abs_y, RM::abs_x, RM::abs_x, RMW::abs_x, RMW::abs_x,
        // 0x40
        FC::rti, RM::idx_ind, UD::hlt, UD::rmw_idx_ind, RM::zp, RM::zp, RMW::zp, RMW::zp,
        FC::pha, RM::imm, SB::sb, RM::imm, FC::jmp_abs, RM::abs, RMW::abs, RMW::abs,
        // 0x50
        FC::bra + 2, RM::ind_idx, UD::hlt, UD::rmw_ind_idx, RM::zp_x, RM::zp_x, RMW::zp_x, RMW::zp_x,
        SB::sb_cli, RM::abs_y, SB::sb, UD::rmw_abs_y, RM::abs_x, RM::abs_x, RMW::abs_x, RMW::abs_x,
        // 0x60
        FC::rts, RM::idx_ind, UD::hlt, UD::rmw_idx_ind, RM::zp, RM::zp, RMW::zp, RMW::zp,
        FC::pla, RM::imm, SB::sb, RM::imm, FC::jmp_ind, RM::abs, RMW::abs, RMW::abs,
        // 0x70
        FC::bra + 3, RM::ind_idx, UD::hlt, UD::rmw_ind_idx, RM::zp_x, RM::zp_x, RMW::zp_x, RMW::zp_x,
        SB::sb_sei, RM::abs_y, SB::sb, UD::rmw_abs_y, RM::abs_x, RM::abs_x, RMW::abs_x, RMW::abs_x,
        // 0x80
        RM::imm, ST::idx_ind, RM::imm, ST::idx_ind, ST::zp, ST::zp, ST::zp, ST::zp,
        SB::sb, RM::imm, SB::sb, RM::imm, ST::abs, ST::abs, ST::abs, ST::abs,
        // 0x90
        FC::bra + 4, ST::ind_y, UD::hlt, UD::st_ind_y, ST::zp_x, ST::zp_x, ST::zp_y, ST::zp_y,
        SB::sb, ST::abs_y, SB::sb, UD::st_abs_y, UD::st_abs_x, ST::abs_x, UD::st_abs_y, UD::st_abs_y,
        // 0xa0
        RM::imm, RM::idx_ind, RM::imm, RM::idx_ind, RM::zp, RM::zp, RM::zp, RM::zp,
        SB::sb, RM::imm, SB::sb, RM::imm, RM::abs, RM::abs, RM::abs, RM::abs,
        // 0xb0
        FC::bra + 5, RM::ind_idx, UD::hlt, RM::ind_idx, RM::zp_x, RM::zp_x, RM::zp_y, RM::zp_y,
        SB::sb, RM::abs_y, SB::sb, RM::abs_y, RM::abs_x, RM::abs_x, RM::abs_y, RM::abs_y,
        // 0xc0
        RM::imm, RM::idx_ind, RM::imm, UD::rmw_idx_ind, RM::zp, RM::zp, RMW::zp, RMW::zp,
        SB::sb, RM::imm, SB::sb, RM::imm, RM::abs, RM::abs, RMW::abs, RMW::abs,
        // 0xd0
        FC::bra + 7, RM::ind_idx, UD::hlt, UD::rmw_ind_idx, RM::zp_x, RM::zp_x, RMW::zp_x, RMW::zp_x,
        SB::sb, RM::abs_y, SB::sb, UD::rmw_abs_y, RM::abs_x, RM::abs_x, RMW::abs_x, RMW::abs_x,
        // 0xe0
        RM::imm, RM::idx_ind, RM::imm, UD::rmw_idx_ind, RM::zp, RM::zp, RMW::zp, RMW::zp,
        SB::sb, RM::imm, SB::sb, RM::imm, RM::abs, RM::abs, RMW::abs, RMW::abs,
        // 0xf0
        FC::bra + 6, RM::ind_idx, UD::hlt, UD::rmw_ind_idx, RM::zp_x, RM::zp_x, RMW::zp_x, RMW::zp_x,
        SB::sb, RM::abs_y, SB::sb, UD::rmw_abs_y, RM::abs_x, RM::abs_x, RMW::abs_x, RMW::abs_x,

        // 0x100
        reset
    };

    // map: instruction (opc) -> micro-code sub-op
    /*static const u8 OPC_MSOPC[] = {
        // 00
        MSOPC::nop, MSOPC::rm_ora, MSOPC::nop, MSOPC::ud_slo, MSOPC::nop, MSOPC::rm_ora, MSOPC::rmw_asl, MSOPC::ud_slo,
        MSOPC::nop, MSOPC::rm_ora, MSOPC::sb_asl, MSOPC::ud_anc, MSOPC::nop, MSOPC::rm_ora, MSOPC::rmw_asl, MSOPC::ud_slo,
        // 10
        MSOPC::nop, MSOPC::rm_ora, MSOPC::nop, MSOPC::ud_slo, MSOPC::nop, MSOPC::rm_ora, MSOPC::rmw_asl, MSOPC::ud_slo,
        MSOPC::sb_clc, MSOPC::rm_ora, MSOPC::nop, MSOPC::ud_slo, MSOPC::nop, MSOPC::rm_ora, MSOPC::rmw_asl, MSOPC::ud_slo,
        // 20
        MSOPC::nop, MSOPC::rm_and, MSOPC::nop, MSOPC::ud_rla, MSOPC::rm_bit, MSOPC::rm_and, MSOPC::rmw_rol, MSOPC::ud_rla,
        MSOPC::nop, MSOPC::nop, MSOPC::sb_rol, MSOPC::ud_anc, MSOPC::rm_bit, MSOPC::rm_and, MSOPC::rmw_rol, MSOPC::ud_rla,
        // 30
        MSOPC::nop, MSOPC::rm_and, MSOPC::nop, MSOPC::ud_rla, MSOPC::nop, MSOPC::rm_and, MSOPC::rmw_rol, MSOPC::ud_rla,
        MSOPC::sb_sec, MSOPC::rm_and, MSOPC::nop, MSOPC::ud_rla, MSOPC::nop, MSOPC::rm_and, MSOPC::rmw_rol, MSOPC::ud_rla,
        // 40
        MSOPC::nop, MSOPC::rm_eor, MSOPC::nop, MSOPC::ud_sre, MSOPC::nop, MSOPC::rm_eor, MSOPC::rmw_lsr, MSOPC::ud_sre,
        MSOPC::nop, MSOPC::rm_eor, MSOPC::sb_lsr, MSOPC::ud_alr, MSOPC::nop, MSOPC::rm_eor, MSOPC::rmw_lsr, MSOPC::ud_sre,
        // 50
        MSOPC::nop, MSOPC::rm_eor, MSOPC::nop, MSOPC::ud_sre, MSOPC::nop, MSOPC::rm_eor, MSOPC::rmw_lsr, MSOPC::ud_sre,
        MSOPC::nop, MSOPC::rm_eor, MSOPC::nop, MSOPC::ud_sre, MSOPC::nop, MSOPC::rm_eor, MSOPC::rmw_lsr, MSOPC::ud_sre,
        // 60
        MSOPC::nop, MSOPC::rm_adc, MSOPC::nop, MSOPC::ud_rra, MSOPC::nop, MSOPC::rm_adc, MSOPC::rmw_ror, MSOPC::ud_rra,
        MSOPC::nop, MSOPC::rm_adc, MSOPC::sb_ror, MSOPC::ud_arr, MSOPC::nop, MSOPC::rm_adc, MSOPC::rmw_ror, MSOPC::ud_rra,
        // 70
        MSOPC::nop, MSOPC::rm_adc, MSOPC::nop, MSOPC::ud_rra, MSOPC::nop, MSOPC::rm_adc, MSOPC::rmw_ror, MSOPC::ud_rra,
        MSOPC::nop, MSOPC::rm_adc, MSOPC::nop, MSOPC::ud_rra, MSOPC::nop, MSOPC::rm_adc, MSOPC::rmw_ror, MSOPC::ud_rra,
        // 80
        MSOPC::nop, MSOPC::nop, MSOPC::nop, MSOPC::nop, MSOPC::nop, MSOPC::nop, MSOPC::nop, MSOPC::nop,
        MSOPC::sb_dey, MSOPC::nop, MSOPC::sb_txa, MSOPC::ud_xaa, MSOPC::nop, MSOPC::nop, MSOPC::nop, MSOPC::nop,
        // 90
        MSOPC::nop, MSOPC::nop, MSOPC::nop, MSOPC::ud_ahx, MSOPC::nop, MSOPC::nop, MSOPC::nop, MSOPC::nop,
        MSOPC::sb_tya, MSOPC::nop, MSOPC::sb_txs, MSOPC::ud_tas, MSOPC::ud_shy, MSOPC::nop, MSOPC::ud_shx, MSOPC::ud_ahx,
        // a0
        MSOPC::rm_ldy, MSOPC::rm_lda, MSOPC::rm_ldx, MSOPC::ud_lax, MSOPC::rm_ldy, MSOPC::nop, MSOPC::rm_ldx, MSOPC::ud_lax,
        MSOPC::sb_tay, MSOPC::rm_lda, MSOPC::sb_tax, MSOPC::ud_lxa, MSOPC::rm_ldy, MSOPC::nop, MSOPC::rm_ldx, MSOPC::ud_lax,
        // b0
        MSOPC::nop, MSOPC::rm_lda, MSOPC::nop, MSOPC::ud_lax, MSOPC::rm_ldy, MSOPC::rm_lda, MSOPC::rm_ldx, MSOPC::ud_lax,
        MSOPC::sb_clv, MSOPC::rm_lda, MSOPC::sb_tsx, MSOPC::ud_las, MSOPC::rm_ldy, MSOPC::nop, MSOPC::rm_ldx, MSOPC::ud_lax,
        // c0
        MSOPC::rm_cpy, MSOPC::rm_cmp, MSOPC::nop, MSOPC::ud_dcp, MSOPC::rm_cpy, MSOPC::rm_cmp, MSOPC::rmw_dec, MSOPC::ud_dcp,
        MSOPC::sb_iny, MSOPC::rm_cmp, MSOPC::sb_dex, MSOPC::ud_axs, MSOPC::rm_cpy, MSOPC::rm_cmp, MSOPC::rmw_dec, MSOPC::ud_dcp,
        // d0
        MSOPC::nop, MSOPC::rm_cmp, MSOPC::nop, MSOPC::ud_dcp, MSOPC::nop, MSOPC::rm_cmp, MSOPC::rmw_dec, MSOPC::ud_dcp,
        MSOPC::sb_cld, MSOPC::rm_cmp, MSOPC::nop, MSOPC::ud_dcp, MSOPC::nop, MSOPC::rm_cmp, MSOPC::rmw_dec, MSOPC::ud_dcp,
        // e0
        MSOPC::rm_cpx, MSOPC::rm_sbc, MSOPC::nop, MSOPC::ud_isc, MSOPC::rm_cpx, MSOPC::rm_sbc, MSOPC::rmw_inc, MSOPC::ud_isc,
        MSOPC::sb_inx, MSOPC::rm_sbc, MSOPC::nop, MSOPC::rm_sbc, MSOPC::rm_cpx, MSOPC::rm_sbc, MSOPC::rmw_inc, MSOPC::ud_isc,
        // f0
        MSOPC::nop, MSOPC::rm_sbc, MSOPC::nop, MSOPC::ud_isc, MSOPC::nop, MSOPC::rm_sbc, MSOPC::rmw_inc, MSOPC::ud_isc,
        MSOPC::sb_sed, MSOPC::rm_sbc, MSOPC::nop, MSOPC::ud_isc, MSOPC::nop, MSOPC::rm_sbc, MSOPC::rmw_inc, MSOPC::ud_isc,
    };*/

} // namespace _MC


const MC::MOP** MC::OPC_MC = _MC::OPC_MC;
//const u8* MC::OPC_MSOPC = _MC::OPC_MSOPC;
