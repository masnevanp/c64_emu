#ifndef COMMON_H_INCLUDED
#define COMMON_H_INCLUDED

#include "nmos6502/nmos6502.h"
#include "nmos6502/nmos6502_core.h"

using u8  = NMOS6502::u8;
using i8  = NMOS6502::i8;
using u16 = NMOS6502::u16;
using i16 = int16_t;
using u32 = uint32_t;
using i32 = int32_t;
using u64 = uint64_t;


/*
    y1 (color clock) freq                 = 17734472 (pal)
    cpu_freq = y1 / 18                    = 985248.444...
    raster_lines_per_frame                = 312
    cycles_per_raster_line                = 63
    frame_cycle_cnt = 312 * 63            = 19656
    pal_freq = cpu_freq / frame_cycle_cnt = 50.12456474...
    frame_duration_ms = 1000 / pal_freq   = 19.95029793...
*/

static constexpr double COLOR_CLOCK_FREQ_PAL = 17734472.0;
static constexpr double CPU_FREQ_PAL         = COLOR_CLOCK_FREQ_PAL / 18.0;


static constexpr int FRAME_LINE_COUNT  = 312;
static constexpr int LINE_CYCLE_COUNT  =  63;
static constexpr int FRAME_CYCLE_COUNT = FRAME_LINE_COUNT * LINE_CYCLE_COUNT;


static constexpr double FRAME_RATE_MIN = 50.0;
static constexpr double FRAME_RATE_MAX = 60.0;
static constexpr double FRAME_RATE_PAL = CPU_FREQ_PAL / FRAME_CYCLE_COUNT;


static constexpr int AUDIO_OUTPUT_FREQ = 44100;


struct U16l { // little-endian
    u8 b0; u8 b1;
    operator u16() const { return (b1 << 8) | b0; }
};

struct U32l {
    u8 b0; u8 b1; u8 b2; u8 b3;
    operator u32() const { return (b3 << 24) | (b2 << 16) | (b1 << 8) | b0; }
};

struct U16b { // big-endian
    u8 b0; u8 b1;
    operator u16() const { return (b0 << 8) | b1; }
};

struct U32b {
    u8 b0; u8 b1; u8 b2; u8 b3;
    operator u32() const { return (b0 << 24) | (b1 << 16) | (b2 << 8) | b3; }
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
        r_stp=GK, q,     cmdre, space, num_2, ctrl,  ar_l, num_1,
        div,      ar_up, eq,    sh_r,  home,  s_col, mul,  pound,
        comma,    at,    colon, dot,   minus, l,     p,    plus,
        n,        o,     k,     m,     num_0, j,     i,    num_9,
        v,        u,     h,     b,     num_8, g,     y,    num_7,
        x,        t,     f,     c,     num_6, d,     r,    num_5,
        sh_l,     e,     s,     z,     num_4, a,     w,    num_3,
        crs_d,    f5,    f3,    f1,    f7,    crs_r, ret,  del,
    };

    enum Keyboard_ext : u8 {
        crs_u = GE, f6, f4, f2, f8, crs_l, // these translate to left shift + a 'real' key
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
    Param(T init, T min_, T max_, T step_, std::function<std::string (T)> to_str_ = std::function<std::string (T)>())
        : val(init), min(min_), max(max_), step(step_), to_str(to_str_) {}

    operator T() const { return val; }
    operator std::string() const {
        if (to_str) return to_str(val);
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

    std::function<std::string (T)> to_str;

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


namespace IO {


class Int_hub {
public:
    enum Src : u8 {
        cia1 = 0x01, vic  = 0x02, exp_i = 0x04, // IRQ sources
        cia2 = 0x10, rstr = 0x20, exp_n = 0x40, // NMI sources
        irq  = 0x0f, nmi  = 0xf0, // source mask
    };

    void reset() { state = old_state = 0x00; nmi_act = irq_act = false; }

    void set(Src s) { state |= s;  }
    void clr(Src s) { state &= ~s; }

    void tick(NMOS6502::Core& cpu) {
        if (state != old_state) {
            old_state = state;

            bool act = (state & Src::nmi);
            if (nmi_act != act) {
                nmi_act = act;
                cpu.set_nmi(nmi_act);
                clr(Src::rstr); // auto clr (pulse)
            }

            act = (state & Src::irq);
            if (irq_act != act) {
                irq_act = act;
                cpu.set_irq(irq_act);
            }
        }
    }

private:
    u8 state;
    u8 old_state;
    bool nmi_act;
    bool irq_act;
};


class Port {
public:
    // connections for external input/output
    using PD_in  = Sig2<u8, u8>; // bits, bit_vals
    using PD_out = Sig1<u8>; // bit_vals

    Port(const PD_out& ext_out_) : ext_out(ext_out_) { reset(); }

    /*  The port pins are set as inputs and port registers to
        zero (although a read of the ports will return all high
        because of passive pullups).
    */
    void reset() {
        set_dd(0b00000000);
        p_in  = 0b11111111;
        p_out = 0b00000000;
    }

    u8   r_dd() const     { return out_bits; }
    void w_dd(u8 dd)      { set_dd(dd); output(); }

    u8   r_pd() const     { return (((p_out & out_bits) | in_bits()) & p_in); }
    void w_pd(u8 d)       { p_out = d; output(); }

    void _set_p_out(u8 bits, u8 vals) { // for CIA to directly set port value (timer PB usage)
        p_out = (p_out & ~bits) | (vals & bits);
        output();
    }

    PD_in ext_in {
        [this](u8 e_in_bits, u8 e_bit_vals) {
            p_in = (p_in & ~e_in_bits) | (e_in_bits & e_bit_vals);
        }
    };

private:
    void set_dd(u8 dd) { out_bits = dd; }

    u8 in_bits() const { return ~out_bits; }

    void output() const { ext_out((p_out & out_bits) | in_bits()); }

    const PD_out& ext_out;

    u8 out_bits;
    u8 p_in;
    u8 p_out;
};


} // namespace IO


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
