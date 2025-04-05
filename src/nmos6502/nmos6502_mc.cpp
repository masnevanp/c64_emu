
#include "nmos6502_mc.h"


using namespace NMOS6502;


const std::string MC::MOPC_str[] = {
    "nmop", "abs_x", "inc_zp", "abs_y", "rm_zp_x", "rm_zp_y", "rm_x", "rm_y",
    "rm_idx_ind", "a_nz", "do_op", "st_zp_x", "st_zp_y", "st_idx_ind", "st_reg",
    "jmp_ind", "bra", "hold_ints", "php", "pha", "jsr", "jmp_abs", "rti", "rts", "inc_sp", "brk",
    "dispatch_cli", "dispatch_sei", "dispatch", "dispatch_brk",
    "sig_hlt", "hlt", "reset",
};

const std::string MC::PC_inc_str[] = { "0", "1" };

const std::string MC::RW_str[] = { "w", "r" };


using MC::MOP;
using MC::MOPC;
using MC::PC_inc;
using MC::RW;
using MC::PC_inc;

const MOP MC::code[] = {
    MOP(Ri16::pc,   Ri8::d,   RW::r, PC_inc(0), MOPC::nmop        ), // 0
    MOP(Ri16::a2,   Ri8::a1h, RW::w, PC_inc(0), MOPC::nmop        ), // 1
    MOP(Ri16::a3,   Ri8::a1l, RW::w, PC_inc(0), MOPC::nmop        ), // 2
    MOP(Ri16::a4,   Ri8::p,   RW::w, PC_inc(0), MOPC::brk         ), // 3
    MOP(Ri16::a2,   Ri8::pcl, RW::r, PC_inc(0), MOPC::nmop        ), // 4
    MOP(Ri16::a3,   Ri8::pch, RW::r, PC_inc(0), MOPC::nmop        ), // 5
    MOP(Ri16::pc,   Ri8::ir,  RW::r, PC_inc(1), MOPC::dispatch_brk), // 6
    MOP(Ri16::pc,   Ri8::zp,  RW::r, PC_inc(1), MOPC::nmop        ), // 7
    MOP(Ri16::zp16, Ri8::d,   RW::r, PC_inc(0), MOPC::rm_idx_ind  ), // 8
    MOP(Ri16::zp16, Ri8::a1l, RW::r, PC_inc(0), MOPC::nmop        ), // 9
    MOP(Ri16::a2,   Ri8::a1h, RW::r, PC_inc(0), MOPC::nmop        ), // 10
    MOP(Ri16::a1,   Ri8::d,   RW::r, PC_inc(0), MOPC::do_op       ), // 11
    MOP(Ri16::pc,   Ri8::ir,  RW::r, PC_inc(1), MOPC::dispatch    ), // 12
    MOP(Ri16::pc,   Ri8::d,   RW::r, PC_inc(1), MOPC::nmop        ), // 13
    MOP(Ri16::pc,   Ri8::a1l, RW::r, PC_inc(0), MOPC::nmop        ), // 14
    MOP(Ri16::pc,   Ri8::a1h, RW::r, PC_inc(0), MOPC::nmop        ), // 15
    MOP(Ri16::pc,   Ri8::a1l, RW::r, PC_inc(0), MOPC::sig_hlt     ), // 16
    MOP(Ri16::pc,   Ri8::a1h, RW::r, PC_inc(0), MOPC::hlt         ), // 17
    MOP(Ri16::pc,   Ri8::ir,  RW::r, PC_inc(1), MOPC::dispatch    ), // 18
    MOP(Ri16::pc,   Ri8::zp,  RW::r, PC_inc(1), MOPC::nmop        ), // 19
    MOP(Ri16::zp16, Ri8::d,   RW::r, PC_inc(0), MOPC::rm_idx_ind  ), // 20
    MOP(Ri16::zp16, Ri8::a1l, RW::r, PC_inc(0), MOPC::nmop        ), // 21
    MOP(Ri16::a2,   Ri8::a1h, RW::r, PC_inc(0), MOPC::nmop        ), // 22
    MOP(Ri16::a1,   Ri8::d,   RW::r, PC_inc(0), MOPC::nmop        ), // 23
    MOP(Ri16::a1,   Ri8::d,   RW::w, PC_inc(0), MOPC::do_op       ), // 24
    MOP(Ri16::a1,   Ri8::d,   RW::w, PC_inc(0), MOPC::nmop        ), // 25
    MOP(Ri16::pc,   Ri8::ir,  RW::r, PC_inc(1), MOPC::dispatch    ), // 26
    MOP(Ri16::pc,   Ri8::zp,  RW::r, PC_inc(1), MOPC::nmop        ), // 27
    MOP(Ri16::zp16, Ri8::d,   RW::r, PC_inc(0), MOPC::do_op       ), // 28
    MOP(Ri16::pc,   Ri8::ir,  RW::r, PC_inc(1), MOPC::dispatch    ), // 29
    MOP(Ri16::pc,   Ri8::zp,  RW::r, PC_inc(1), MOPC::nmop        ), // 30
    MOP(Ri16::zp16, Ri8::d,   RW::r, PC_inc(0), MOPC::nmop        ), // 31
    MOP(Ri16::zp16, Ri8::d,   RW::w, PC_inc(0), MOPC::do_op       ), // 32
    MOP(Ri16::zp16, Ri8::d,   RW::w, PC_inc(0), MOPC::nmop        ), // 33
    MOP(Ri16::pc,   Ri8::ir,  RW::r, PC_inc(1), MOPC::dispatch    ), // 34
    MOP(Ri16::pc,   Ri8::d,   RW::r, PC_inc(0), MOPC::php         ), // 35
    MOP(Ri16::a1,   Ri8::p,   RW::w, PC_inc(0), MOPC::nmop        ), // 36
    MOP(Ri16::pc,   Ri8::ir,  RW::r, PC_inc(1), MOPC::dispatch    ), // 37
    MOP(Ri16::pc,   Ri8::d,   RW::r, PC_inc(1), MOPC::do_op       ), // 38
    MOP(Ri16::pc,   Ri8::ir,  RW::r, PC_inc(1), MOPC::dispatch    ), // 39
    MOP(Ri16::pc,   Ri8::d,   RW::r, PC_inc(0), MOPC::do_op       ), // 40
    MOP(Ri16::pc,   Ri8::ir,  RW::r, PC_inc(1), MOPC::dispatch    ), // 41
    MOP(Ri16::pc,   Ri8::a1l, RW::r, PC_inc(1), MOPC::nmop        ), // 42
    MOP(Ri16::pc,   Ri8::a1h, RW::r, PC_inc(1), MOPC::nmop        ), // 43
    MOP(Ri16::a1,   Ri8::d,   RW::r, PC_inc(0), MOPC::do_op       ), // 44
    MOP(Ri16::pc,   Ri8::ir,  RW::r, PC_inc(1), MOPC::dispatch    ), // 45
    MOP(Ri16::pc,   Ri8::a1l, RW::r, PC_inc(1), MOPC::nmop        ), // 46
    MOP(Ri16::pc,   Ri8::a1h, RW::r, PC_inc(1), MOPC::nmop        ), // 47
    MOP(Ri16::a1,   Ri8::d,   RW::r, PC_inc(0), MOPC::nmop        ), // 48
    MOP(Ri16::a1,   Ri8::d,   RW::w, PC_inc(0), MOPC::do_op       ), // 49
    MOP(Ri16::a1,   Ri8::d,   RW::w, PC_inc(0), MOPC::nmop        ), // 50
    MOP(Ri16::pc,   Ri8::ir,  RW::r, PC_inc(1), MOPC::dispatch    ), // 51
    MOP(Ri16::pc,   Ri8::d,   RW::r, PC_inc(1), MOPC::bra         ), // 52
    MOP(Ri16::pc,   Ri8::ir,  RW::r, PC_inc(1), MOPC::dispatch    ), // 53
    MOP(Ri16::a2,   Ri8::d,   RW::r, PC_inc(0), MOPC::hold_ints   ), // 54
    MOP(Ri16::pc,   Ri8::ir,  RW::r, PC_inc(1), MOPC::dispatch    ), // 55
    MOP(Ri16::a2,   Ri8::d,   RW::r, PC_inc(0), MOPC::nmop        ), // 56
    MOP(Ri16::a1,   Ri8::d,   RW::r, PC_inc(0), MOPC::nmop        ), // 57
    MOP(Ri16::pc,   Ri8::ir,  RW::r, PC_inc(1), MOPC::dispatch    ), // 58
    MOP(Ri16::pc,   Ri8::zp,  RW::r, PC_inc(1), MOPC::nmop        ), // 59
    MOP(Ri16::zp16, Ri8::a1l, RW::r, PC_inc(0), MOPC::inc_zp      ), // 60
    MOP(Ri16::zp16, Ri8::a1h, RW::r, PC_inc(0), MOPC::rm_y        ), // 61
    MOP(Ri16::a1,   Ri8::d,   RW::r, PC_inc(0), MOPC::do_op       ), // 62
    MOP(Ri16::pc,   Ri8::ir,  RW::r, PC_inc(1), MOPC::dispatch    ), // 63
    MOP(Ri16::a1,   Ri8::d,   RW::r, PC_inc(0), MOPC::nmop        ), // 64
    MOP(Ri16::a2,   Ri8::d,   RW::r, PC_inc(0), MOPC::do_op       ), // 65
    MOP(Ri16::pc,   Ri8::ir,  RW::r, PC_inc(1), MOPC::dispatch    ), // 66
    MOP(Ri16::pc,   Ri8::zp,  RW::r, PC_inc(1), MOPC::nmop        ), // 67
    MOP(Ri16::zp16, Ri8::a1l, RW::r, PC_inc(0), MOPC::inc_zp      ), // 68
    MOP(Ri16::zp16, Ri8::a1h, RW::r, PC_inc(0), MOPC::abs_y       ), // 69
    MOP(Ri16::a1,   Ri8::d,   RW::r, PC_inc(0), MOPC::nmop        ), // 70
    MOP(Ri16::a2,   Ri8::d,   RW::r, PC_inc(0), MOPC::nmop        ), // 71
    MOP(Ri16::a2,   Ri8::d,   RW::w, PC_inc(0), MOPC::do_op       ), // 72
    MOP(Ri16::a2,   Ri8::d,   RW::w, PC_inc(0), MOPC::nmop        ), // 73
    MOP(Ri16::pc,   Ri8::ir,  RW::r, PC_inc(1), MOPC::dispatch    ), // 74
    MOP(Ri16::pc,   Ri8::zp,  RW::r, PC_inc(1), MOPC::nmop        ), // 75
    MOP(Ri16::zp16, Ri8::d,   RW::r, PC_inc(0), MOPC::rm_zp_x     ), // 76
    MOP(Ri16::zp16, Ri8::d,   RW::r, PC_inc(0), MOPC::do_op       ), // 77
    MOP(Ri16::pc,   Ri8::ir,  RW::r, PC_inc(1), MOPC::dispatch    ), // 78
    MOP(Ri16::pc,   Ri8::zp,  RW::r, PC_inc(1), MOPC::nmop        ), // 79
    MOP(Ri16::zp16, Ri8::d,   RW::r, PC_inc(0), MOPC::rm_zp_x     ), // 80
    MOP(Ri16::zp16, Ri8::d,   RW::r, PC_inc(0), MOPC::nmop        ), // 81
    MOP(Ri16::zp16, Ri8::d,   RW::w, PC_inc(0), MOPC::do_op       ), // 82
    MOP(Ri16::zp16, Ri8::d,   RW::w, PC_inc(0), MOPC::nmop        ), // 83
    MOP(Ri16::pc,   Ri8::ir,  RW::r, PC_inc(1), MOPC::dispatch    ), // 84
    MOP(Ri16::pc,   Ri8::a1l, RW::r, PC_inc(1), MOPC::nmop        ), // 85
    MOP(Ri16::pc,   Ri8::a1h, RW::r, PC_inc(1), MOPC::rm_y        ), // 86
    MOP(Ri16::a1,   Ri8::d,   RW::r, PC_inc(0), MOPC::do_op       ), // 87
    MOP(Ri16::pc,   Ri8::ir,  RW::r, PC_inc(1), MOPC::dispatch    ), // 88
    MOP(Ri16::a1,   Ri8::d,   RW::r, PC_inc(0), MOPC::nmop        ), // 89
    MOP(Ri16::a2,   Ri8::d,   RW::r, PC_inc(0), MOPC::do_op       ), // 90
    MOP(Ri16::pc,   Ri8::ir,  RW::r, PC_inc(1), MOPC::dispatch    ), // 91
    MOP(Ri16::pc,   Ri8::a1l, RW::r, PC_inc(1), MOPC::nmop        ), // 92
    MOP(Ri16::pc,   Ri8::a1h, RW::r, PC_inc(1), MOPC::abs_y       ), // 93
    MOP(Ri16::a1,   Ri8::d,   RW::r, PC_inc(0), MOPC::nmop        ), // 94
    MOP(Ri16::a2,   Ri8::d,   RW::r, PC_inc(0), MOPC::nmop        ), // 95
    MOP(Ri16::a2,   Ri8::d,   RW::w, PC_inc(0), MOPC::do_op       ), // 96
    MOP(Ri16::a2,   Ri8::d,   RW::w, PC_inc(0), MOPC::nmop        ), // 97
    MOP(Ri16::pc,   Ri8::ir,  RW::r, PC_inc(1), MOPC::dispatch    ), // 98
    MOP(Ri16::pc,   Ri8::a1l, RW::r, PC_inc(1), MOPC::nmop        ), // 99
    MOP(Ri16::pc,   Ri8::a1h, RW::r, PC_inc(1), MOPC::rm_x        ), // 100
    MOP(Ri16::a1,   Ri8::d,   RW::r, PC_inc(0), MOPC::do_op       ), // 101
    MOP(Ri16::pc,   Ri8::ir,  RW::r, PC_inc(1), MOPC::dispatch    ), // 102
    MOP(Ri16::a1,   Ri8::d,   RW::r, PC_inc(0), MOPC::nmop        ), // 103
    MOP(Ri16::a2,   Ri8::d,   RW::r, PC_inc(0), MOPC::do_op       ), // 104
    MOP(Ri16::pc,   Ri8::ir,  RW::r, PC_inc(1), MOPC::dispatch    ), // 105
    MOP(Ri16::pc,   Ri8::a1l, RW::r, PC_inc(1), MOPC::nmop        ), // 106
    MOP(Ri16::pc,   Ri8::a1h, RW::r, PC_inc(1), MOPC::abs_x       ), // 107
    MOP(Ri16::a1,   Ri8::d,   RW::r, PC_inc(0), MOPC::nmop        ), // 108
    MOP(Ri16::a2,   Ri8::d,   RW::r, PC_inc(0), MOPC::nmop        ), // 109
    MOP(Ri16::a2,   Ri8::d,   RW::w, PC_inc(0), MOPC::do_op       ), // 110
    MOP(Ri16::a2,   Ri8::d,   RW::w, PC_inc(0), MOPC::nmop        ), // 111
    MOP(Ri16::pc,   Ri8::ir,  RW::r, PC_inc(1), MOPC::dispatch    ), // 112
    MOP(Ri16::pc,   Ri8::a1l, RW::r, PC_inc(1), MOPC::nmop        ), // 113
    MOP(Ri16::sp16, Ri8::d,   RW::r, PC_inc(0), MOPC::jsr         ), // 114
    MOP(Ri16::a3,   Ri8::a2h, RW::w, PC_inc(0), MOPC::nmop        ), // 115
    MOP(Ri16::a4,   Ri8::a2l, RW::w, PC_inc(0), MOPC::nmop        ), // 116
    MOP(Ri16::a2,   Ri8::pch, RW::r, PC_inc(0), MOPC::nmop        ), // 117
    MOP(Ri16::pc,   Ri8::ir,  RW::r, PC_inc(1), MOPC::dispatch    ), // 118
    MOP(Ri16::pc,   Ri8::d,   RW::r, PC_inc(0), MOPC::nmop        ), // 119
    MOP(Ri16::sp16, Ri8::d,   RW::r, PC_inc(0), MOPC::inc_sp      ), // 120
    MOP(Ri16::sp16, Ri8::p,   RW::r, PC_inc(0), MOPC::nmop        ), // 121
    MOP(Ri16::pc,   Ri8::ir,  RW::r, PC_inc(1), MOPC::dispatch    ), // 122
    MOP(Ri16::pc,   Ri8::d,   RW::r, PC_inc(0), MOPC::nmop        ), // 123
    MOP(Ri16::sp16, Ri8::d,   RW::r, PC_inc(0), MOPC::rti         ), // 124
    MOP(Ri16::a1,   Ri8::p,   RW::r, PC_inc(0), MOPC::nmop        ), // 125
    MOP(Ri16::a2,   Ri8::pcl, RW::r, PC_inc(0), MOPC::nmop        ), // 126
    MOP(Ri16::sp16, Ri8::pch, RW::r, PC_inc(0), MOPC::nmop        ), // 127
    MOP(Ri16::pc,   Ri8::ir,  RW::r, PC_inc(1), MOPC::dispatch    ), // 128
    MOP(Ri16::pc,   Ri8::d,   RW::r, PC_inc(0), MOPC::pha         ), // 129
    MOP(Ri16::a1,   Ri8::a,   RW::w, PC_inc(0), MOPC::nmop        ), // 130
    MOP(Ri16::pc,   Ri8::ir,  RW::r, PC_inc(1), MOPC::dispatch    ), // 131
    MOP(Ri16::pc,   Ri8::a1l, RW::r, PC_inc(1), MOPC::nmop        ), // 132
    MOP(Ri16::pc,   Ri8::a1h, RW::r, PC_inc(0), MOPC::jmp_abs     ), // 133
    MOP(Ri16::pc,   Ri8::ir,  RW::r, PC_inc(1), MOPC::dispatch    ), // 134
    MOP(Ri16::pc,   Ri8::d,   RW::r, PC_inc(0), MOPC::nmop        ), // 135
    MOP(Ri16::pc,   Ri8::ir,  RW::r, PC_inc(1), MOPC::dispatch_cli), // 136
    MOP(Ri16::pc,   Ri8::d,   RW::r, PC_inc(0), MOPC::nmop        ), // 137
    MOP(Ri16::sp16, Ri8::d,   RW::r, PC_inc(0), MOPC::rts         ), // 138
    MOP(Ri16::a2,   Ri8::pcl, RW::r, PC_inc(0), MOPC::nmop        ), // 139
    MOP(Ri16::sp16, Ri8::pch, RW::r, PC_inc(0), MOPC::nmop        ), // 140
    MOP(Ri16::pc,   Ri8::d,   RW::r, PC_inc(1), MOPC::nmop        ), // 141
    MOP(Ri16::pc,   Ri8::ir,  RW::r, PC_inc(1), MOPC::dispatch    ), // 142
    MOP(Ri16::pc,   Ri8::d,   RW::r, PC_inc(0), MOPC::nmop        ), // 143
    MOP(Ri16::sp16, Ri8::d,   RW::r, PC_inc(0), MOPC::inc_sp      ), // 144
    MOP(Ri16::sp16, Ri8::a,   RW::r, PC_inc(0), MOPC::a_nz        ), // 145
    MOP(Ri16::pc,   Ri8::ir,  RW::r, PC_inc(1), MOPC::dispatch    ), // 146
    MOP(Ri16::pc,   Ri8::a1l, RW::r, PC_inc(1), MOPC::nmop        ), // 147
    MOP(Ri16::pc,   Ri8::a1h, RW::r, PC_inc(1), MOPC::nmop        ), // 148
    MOP(Ri16::a1,   Ri8::pcl, RW::r, PC_inc(0), MOPC::jmp_ind     ), // 149
    MOP(Ri16::a1,   Ri8::pch, RW::r, PC_inc(0), MOPC::nmop        ), // 150
    MOP(Ri16::pc,   Ri8::ir,  RW::r, PC_inc(1), MOPC::dispatch    ), // 151
    MOP(Ri16::pc,   Ri8::d,   RW::r, PC_inc(0), MOPC::nmop        ), // 152
    MOP(Ri16::pc,   Ri8::ir,  RW::r, PC_inc(1), MOPC::dispatch_sei), // 153
    MOP(Ri16::pc,   Ri8::zp,  RW::r, PC_inc(1), MOPC::nmop        ), // 154
    MOP(Ri16::zp16, Ri8::d,   RW::r, PC_inc(0), MOPC::st_idx_ind  ), // 155
    MOP(Ri16::zp16, Ri8::a1l, RW::r, PC_inc(0), MOPC::nmop        ), // 156
    MOP(Ri16::a2,   Ri8::a1h, RW::r, PC_inc(0), MOPC::nmop        ), // 157
    MOP(Ri16::a1,   Ri8::d,   RW::w, PC_inc(0), MOPC::nmop        ), // 158
    MOP(Ri16::pc,   Ri8::ir,  RW::r, PC_inc(1), MOPC::dispatch    ), // 159
    MOP(Ri16::pc,   Ri8::zp,  RW::r, PC_inc(1), MOPC::st_reg      ), // 160
    MOP(Ri16::zp16, Ri8::d,   RW::w, PC_inc(0), MOPC::nmop        ), // 161
    MOP(Ri16::pc,   Ri8::ir,  RW::r, PC_inc(1), MOPC::dispatch    ), // 162
    MOP(Ri16::pc,   Ri8::a1l, RW::r, PC_inc(1), MOPC::nmop        ), // 163
    MOP(Ri16::pc,   Ri8::a1h, RW::r, PC_inc(1), MOPC::st_reg      ), // 164
    MOP(Ri16::a1,   Ri8::d,   RW::w, PC_inc(0), MOPC::nmop        ), // 165
    MOP(Ri16::pc,   Ri8::ir,  RW::r, PC_inc(1), MOPC::dispatch    ), // 166
    MOP(Ri16::pc,   Ri8::zp,  RW::r, PC_inc(1), MOPC::nmop        ), // 167
    MOP(Ri16::zp16, Ri8::a1l, RW::r, PC_inc(0), MOPC::inc_zp      ), // 168
    MOP(Ri16::zp16, Ri8::a1h, RW::r, PC_inc(0), MOPC::abs_y       ), // 169
    MOP(Ri16::a1,   Ri8::d,   RW::r, PC_inc(0), MOPC::nmop        ), // 170
    MOP(Ri16::a2,   Ri8::a,   RW::w, PC_inc(0), MOPC::nmop        ), // 171
    MOP(Ri16::pc,   Ri8::ir,  RW::r, PC_inc(1), MOPC::dispatch    ), // 172
    MOP(Ri16::pc,   Ri8::zp,  RW::r, PC_inc(1), MOPC::nmop        ), // 173
    MOP(Ri16::zp16, Ri8::a1l, RW::r, PC_inc(0), MOPC::inc_zp      ), // 174
    MOP(Ri16::zp16, Ri8::a1h, RW::r, PC_inc(0), MOPC::abs_y       ), // 175
    MOP(Ri16::a1,   Ri8::d,   RW::r, PC_inc(0), MOPC::do_op       ), // 176
    MOP(Ri16::a2,   Ri8::d,   RW::w, PC_inc(0), MOPC::nmop        ), // 177
    MOP(Ri16::pc,   Ri8::ir,  RW::r, PC_inc(1), MOPC::dispatch    ), // 178
    MOP(Ri16::pc,   Ri8::zp,  RW::r, PC_inc(1), MOPC::nmop        ), // 179
    MOP(Ri16::zp16, Ri8::d,   RW::r, PC_inc(0), MOPC::st_zp_x     ), // 180
    MOP(Ri16::zp16, Ri8::d,   RW::w, PC_inc(0), MOPC::nmop        ), // 181
    MOP(Ri16::pc,   Ri8::ir,  RW::r, PC_inc(1), MOPC::dispatch    ), // 182
    MOP(Ri16::pc,   Ri8::zp,  RW::r, PC_inc(1), MOPC::nmop        ), // 183
    MOP(Ri16::zp16, Ri8::d,   RW::r, PC_inc(0), MOPC::st_zp_y     ), // 184
    MOP(Ri16::zp16, Ri8::d,   RW::w, PC_inc(0), MOPC::nmop        ), // 185
    MOP(Ri16::pc,   Ri8::ir,  RW::r, PC_inc(1), MOPC::dispatch    ), // 186
    MOP(Ri16::pc,   Ri8::a1l, RW::r, PC_inc(1), MOPC::nmop        ), // 187
    MOP(Ri16::pc,   Ri8::a1h, RW::r, PC_inc(1), MOPC::abs_y       ), // 188
    MOP(Ri16::a1,   Ri8::d,   RW::r, PC_inc(0), MOPC::nmop        ), // 189
    MOP(Ri16::a2,   Ri8::a,   RW::w, PC_inc(0), MOPC::nmop        ), // 190
    MOP(Ri16::pc,   Ri8::ir,  RW::r, PC_inc(1), MOPC::dispatch    ), // 191
    MOP(Ri16::pc,   Ri8::a1l, RW::r, PC_inc(1), MOPC::nmop        ), // 192
    MOP(Ri16::pc,   Ri8::a1h, RW::r, PC_inc(1), MOPC::abs_y       ), // 193
    MOP(Ri16::a1,   Ri8::d,   RW::r, PC_inc(0), MOPC::do_op       ), // 194
    MOP(Ri16::a2,   Ri8::d,   RW::w, PC_inc(0), MOPC::nmop        ), // 195
    MOP(Ri16::pc,   Ri8::ir,  RW::r, PC_inc(1), MOPC::dispatch    ), // 196
    MOP(Ri16::pc,   Ri8::a1l, RW::r, PC_inc(1), MOPC::nmop        ), // 197
    MOP(Ri16::pc,   Ri8::a1h, RW::r, PC_inc(1), MOPC::abs_x       ), // 198
    MOP(Ri16::a1,   Ri8::d,   RW::r, PC_inc(0), MOPC::do_op       ), // 199
    MOP(Ri16::a2,   Ri8::d,   RW::w, PC_inc(0), MOPC::nmop        ), // 200
    MOP(Ri16::pc,   Ri8::ir,  RW::r, PC_inc(1), MOPC::dispatch    ), // 201
    MOP(Ri16::pc,   Ri8::a1l, RW::r, PC_inc(1), MOPC::nmop        ), // 202
    MOP(Ri16::pc,   Ri8::a1h, RW::r, PC_inc(1), MOPC::abs_x       ), // 203
    MOP(Ri16::a1,   Ri8::d,   RW::r, PC_inc(0), MOPC::nmop        ), // 204
    MOP(Ri16::a2,   Ri8::a,   RW::w, PC_inc(0), MOPC::nmop        ), // 205
    MOP(Ri16::pc,   Ri8::ir,  RW::r, PC_inc(1), MOPC::dispatch    ), // 206
    MOP(Ri16::pc,   Ri8::zp,  RW::r, PC_inc(1), MOPC::nmop        ), // 207
    MOP(Ri16::zp16, Ri8::d,   RW::r, PC_inc(0), MOPC::rm_zp_y     ), // 208
    MOP(Ri16::zp16, Ri8::d,   RW::r, PC_inc(0), MOPC::do_op       ), // 209
    MOP(Ri16::pc,   Ri8::ir,  RW::r, PC_inc(1), MOPC::dispatch    ), // 210
    MOP(Ri16::pc,   Ri8::d,   RW::r, PC_inc(1), MOPC::nmop        ), // 211
    MOP(Ri16::pc,   Ri8::d,   RW::r, PC_inc(1), MOPC::nmop        ), // 212
    MOP(Ri16::a1,   Ri8::d,   RW::r, PC_inc(0), MOPC::nmop        ), // 213
    MOP(Ri16::a2,   Ri8::d,   RW::r, PC_inc(0), MOPC::nmop        ), // 214
    MOP(Ri16::a3,   Ri8::d,   RW::r, PC_inc(0), MOPC::nmop        ), // 215
    MOP(Ri16::a4,   Ri8::pcl, RW::r, PC_inc(0), MOPC::reset       ), // 216
    MOP(Ri16::a1,   Ri8::pch, RW::r, PC_inc(0), MOPC::nmop        ), // 217
    MOP(Ri16::pc,   Ri8::ir,  RW::r, PC_inc(1), MOPC::dispatch_brk), // 218
};

const u8 MC::opc_addr[] = {
    0, 7, 13, 19, 27, 27, 30, 30, 35, 38, 40, 38, 42, 42, 46, 46,
    52, 59, 13, 67, 75, 75, 79, 79, 40, 85, 40, 92, 99, 99, 106, 106,
    113, 7, 13, 19, 27, 27, 30, 30, 119, 38, 40, 38, 42, 42, 46, 46,
    52, 59, 13, 67, 75, 75, 79, 79, 40, 85, 40, 92, 99, 99, 106, 106,
    123, 7, 13, 19, 27, 27, 30, 30, 129, 38, 40, 38, 132, 42, 46, 46,
    52, 59, 13, 67, 75, 75, 79, 79, 135, 85, 40, 92, 99, 99, 106, 106,
    137, 7, 13, 19, 27, 27, 30, 30, 143, 38, 40, 38, 147, 42, 46, 46,
    52, 59, 13, 67, 75, 75, 79, 79, 152, 85, 40, 92, 99, 99, 106, 106,
    38, 154, 38, 154, 160, 160, 160, 160, 40, 38, 40, 38, 163, 163, 163, 163,
    52, 167, 13, 173, 179, 179, 183, 183, 40, 187, 40, 192, 197, 202, 192, 192,
    38, 7, 38, 7, 27, 27, 27, 27, 40, 38, 40, 38, 42, 42, 42, 42,
    52, 59, 13, 59, 75, 75, 207, 207, 40, 85, 40, 85, 99, 99, 85, 85,
    38, 7, 38, 19, 27, 27, 30, 30, 40, 38, 40, 38, 42, 42, 46, 46,
    52, 59, 13, 67, 75, 75, 79, 79, 40, 85, 40, 92, 99, 99, 106, 106,
    38, 7, 38, 19, 27, 27, 30, 30, 40, 38, 40, 38, 42, 42, 46, 46,
    52, 59, 13, 67, 75, 75, 79, 79, 40, 85, 40, 92, 99, 99, 106, 106,
    211,
};
