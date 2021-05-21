#ifndef COMMON_H_INCLUDED
#define COMMON_H_INCLUDED

#include "nmos6502/nmos6502.h"

using u8  = NMOS6502::u8;
using i8  = NMOS6502::i8;
using u16 = NMOS6502::u16;
using i16 = int16_t;
using u32 = uint32_t;
using i32 = int32_t;


// Real PAL C64 COLOR_CLOCK_FREQ: 17734472.0 --> ~50.125 fps

static constexpr double FRAME_RATE = 50.0;
static constexpr double CPU_FREQ = FRAME_RATE * (312.0 * 63.0);
static constexpr double COLOR_CLOCK_FREQ = 18.0 * CPU_FREQ;


using Sig = NMOS6502::Sig;

template<typename T>
using Sig1 = std::function<void (T)>;

template<typename T1, typename T2>
using Sig2 = std::function<void (T1, T2)>;


namespace Key_code {
    enum Group { keyboard, keyboard_ext, joystick, system };

    static constexpr u8 GK = Group::keyboard << 6;
    static constexpr u8 GE = Group::keyboard_ext << 6;
    static constexpr u8 GJ = Group::joystick << 6;
    static constexpr u8 GS = Group::system << 6;

    /*       PB7   PB6   PB5   PB4   PB3   PB2   PB1   PB0
        PA7  STOP  Q     C=    SPACE 2     CTRL  <-    1
        PA6  /     ^     =     RSHFT HOME  ;     *     £
        PA5  ,     @     :     .     -     L     P     +
        PA4  N     O     K     M     0     J     I     9
        PA3  V     U     H     B     8     G     Y     7
        PA2  X     T     F     C     6     D     R     5
        PA1  LSHFT E     S     Z     4     A     W     3
        PA0  CRS_D F5    F3    F1    F7    CRS_R RET   DEL

        Source: https://www.c64-wiki.com/wiki/Keyboard#Keyboard_Map
    */

    enum Keyboard : u8 { // fall into 'keyboard' group
        del = GK, ret,  crs_r, f7,    f1,    f3,    f5,    crs_d,
        num_3,    w,    a,     num_4, z,     s,     e,     sh_l,
        num_5,    r,    d,     num_6, c,     f,     t,     x,
        num_7,    y,    g,     num_8, b,     h,     u,     v,
        num_9,    i,    j,     num_0, m,     k,     o,     n,
        plus,     p,    l,     minus, dot,   colon, at,    comma,
        pound,    mul,  s_col, home,  sh_r,  eq,    ar_up, div,
        num_1,    ar_l, ctrl,  num_2, space, cmdre, q,     r_stp,
    };

    enum Keyboard_ext : u8 {
        crs_l = GE, f8, f2, f4, f6, crs_u, // these translate to left shift + a 'real' key
        s_lck,
    };

    enum Joystick : u8 {
        ju = GJ, jd, jl, jr, jb
    };

    enum System : u8 {
        rstre = GS, quit, nop,
        rst_c, swp_j, v_fsc, menu_tgl,
        menu_ent, menu_up, menu_down,
    };

} // namespace Key_code


using Sig_key = Sig2<u8, u8>;


template<typename T>
struct Param {
    T val;
    const T min;
    const T max;
    const T step;

    Param(T val_, T min_, T max_, T step_) : val(val_), min(min_), max(max_), step(step_) {}
    operator T() const { return val; }
    Param<T>& operator=(T v) { val = v; return *this; }
};


#endif // COMMON_H_INCLUDED
