#ifndef COMMON_H_INCLUDED
#define COMMON_H_INCLUDED

#include "nmos6502/nmos6502.h"

using u8  = NMOS6502::u8;
using i8  = NMOS6502::i8;
using u16 = NMOS6502::u16;
using i16 = int16_t;
using u32 = uint32_t;
using i32 = int32_t;
using u64 = uint64_t;


// Real PAL C64 COLOR_CLOCK_FREQ: 17734472.0 --> ~50.125 fps

static constexpr double FRAME_RATE = 50.0;
static constexpr double CPU_FREQ = FRAME_RATE * (312.0 * 63.0);
static constexpr double COLOR_CLOCK_FREQ = 18.0 * CPU_FREQ;


struct U16 { // TODO: rename, e.g. U16_le (little-endian u16)
    u8 b0; u8 b1;
    operator u16() const { return (b1 << 8) | b0; }
};

struct U32 {
    u8 b0; u8 b1; u8 b2; u8 b3;
    operator u32() const { return (b3 << 24) | (b2 << 16) | (b1 << 8) | b0; }
};


using Sig = NMOS6502::Sig;

template<typename T>
using Sig1 = std::function<void (T)>;

template<typename T1, typename T2>
using Sig2 = std::function<void (T1, T2)>;


enum Color : u8 {
    black, white, red, cyan, purple, green, blue, yellow,
    orange, brown, light_red, gray_1, gray_2, light_green, light_blue, gray_3
};


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
        rst_c, swp_j, v_fsc, menu_tgl, rot_dsk,
        menu_ent, menu_up, menu_down,
    };

} // namespace Key_code


using Sig_key = Sig2<u8, u8>;


template<typename T>
class Param {
public:
    Param(T init, T min_, T max_, T step_) : val(init), min(min_), max(max_), step(step_) {}

    operator T() const { return val; }
    operator std::string() const {
        if constexpr (std::is_convertible<T, std::string>::value) return val;
        else return std::to_string(val);
    }
    Param<T>& operator++() { set(val + step); return *this; }
    Param<T>& operator--() { set(val - step); return *this; }
    Param<T>& operator=(T v) { set(v); return *this; }

private:
    T val;
    const T min;
    const T max;
    const T step;

    void set(T v) { if (v >= min && v <= max) val = v; }
};


template<typename T>
struct Choice {
    const std::vector<T> choices;
    const std::vector<std::string> choices_str;

    T chosen;

    operator T() const { return chosen; }
    Choice<T>& operator=(T c) { chosen = c; return *this; }

    Choice(std::initializer_list<T> choices_, std::initializer_list<std::string> choices_str_, T chosen_)
        : choices(choices_), choices_str(choices_str_), chosen(chosen_) {}
    Choice(std::initializer_list<T> choices_, std::initializer_list<std::string> choices_str_)
        : Choice(choices_, choices_str_, *choices_.begin()) {}
};


namespace C1541 {

enum : u8 { first_sector = 0, first_track = 1, last_track = 35, dir_track = 18 };

constexpr int sector_count(u8 track_n) {
    constexpr int cnt[35] = {
        21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
        21, 19, 19, 19, 19, 19, 19, 19, 18, 18, 18, 18, 18, 18, 17, 17,
        17, 17, 17
    };
    return (track_n < first_track || track_n > last_track) ? 0 : cnt[track_n - 1];
}

constexpr int speed_zone(u8 track_n) {
    switch (sector_count(track_n)) {
        case 17: return 0;    case 18: return 1;
        case 19: return 2;    case 21: return 3;
        default: return -1;
    }
}

} // namespace C1541


#endif // COMMON_H_INCLUDED
