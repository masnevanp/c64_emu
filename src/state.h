#ifndef STATE_H_INCLUDED
#define STATE_H_INCLUDED

#include "common.h"


namespace State {

// TODO: - c1541 and expansion states
//       - optimize data layouts (hot data, proper alignment, etc...)
//       - for better data locality, move all the data here...?
//         (even things that strictly speaking don't need to be here)?
//         (currently only data needed for 'save/load -state' included)
//       - collect all cycle-level flags to a single variable (u32?)?

struct VIC_II {
    static constexpr int REG_COUNT = 64;
    static constexpr int MOB_COUNT = 8;

    struct R {
        enum u8 {
            m0x,  m0y,  m1x,  m1y,  m2x,  m2y,  m3x,  m3y,  m4x,  m4y,  m5x,  m5y,  m6x,  m6y,  m7x,  m7y,
            mnx8, cr1,  rast, lpx,  lpy,  mne,  cr2,  mnye, mptr, ireg, ien,  mndp, mnmc, mnxe, mnm,  mnd,
            ecol, bgc0, bgc1, bgc2, bgc3, mmc0, mmc1, m0c,  m1c,  m2c,  m3c,  m4c,  m5c,  m6c,  m7c,
        };
    };

    enum V_blank : u8 { vb_off = 0, vb_on = 63 };

    struct Light_pen {
        u8 triggered = 0;
        u8 trigger_at_phi1 = false;
    };

    struct MOB {
        static constexpr u8 transparent = 0xff;

        u8 disp_on = false;

        u8 ye;
        u8 mdc_base_upd;

        u8 mdc_base = 0xff;
        u8 mdc;

        u16 x;

        u32 data;
        u32 pixel_mask;

        u8 shift_base_delay;
        u8 shift_delay;
        u8 shift_amount;
        u8 shift_timer;

        u8 col[4] = { transparent, transparent, transparent, transparent };
    };

    struct GFX {
        u8 mode = 0;

        u8 y_scroll;

        u8 ba_area = false; // set for each frame (at the top of disp.win)
        u8 ba_line = false;

        struct VM_data { // video-matrix data
            u8 data = 0; // 'screen memory' data
            u8 col = 0;  // color-ram data
        };
        VM_data vm[40]; // vm row buffer

        u64 pipeline;
        u16 vc_base;
        u16 vc;
        u16 rc;
        u16 gfx_addr;
        VM_data vmd;
        u8 act;
        u8 gd;
        u8 blocked; // if blocked, only output bg col (toggled by border unit)
        u8 vmri; // vm read index
        u8 vmoi; // vm output index
        u8 px; // for storing 2bit pixels across 'output()' runs
    };

    struct Border {
        static constexpr u32 not_set = 0xffffffff;
        // indicate beam_pos at which border should start/end
        u32 on_at  = not_set;
        u32 off_at = not_set;
    };

    u8 reg[REG_COUNT];

    u64 cycle = 0; // good for ~595K years...
    u16 raster_y = 0;
    u16 raster_y_cmp;
    u8 raster_y_cmp_edge;
    V_blank v_blank = V_blank::vb_on;

    Light_pen lp;
    MOB mob[MOB_COUNT];
    GFX gfx;
    Border border;

    u32 beam_pos = 0;
    u8 frame[::VIC_II::FRAME_SIZE] = {};

    u8 cr1(u8 field) const { return reg[R::cr1] & field; }
    u8 cr2(u8 field) const { return reg[R::cr2] & field; }

    int line_cycle() const  { return cycle % LINE_CYCLE_COUNT; }
    int frame_cycle() const { return cycle % FRAME_CYCLE_COUNT; }

    // TODO: apply an offset in LP usage (this value is good for mob timinig...)
    u16 raster_x() const { return (400 + (line_cycle() * 8)) % (LINE_CYCLE_COUNT * 8); }
};


struct CIA {
    struct Int_ctrl {
        u16 new_icr;

        u8 icr;
        u8 mask;
    };

    struct Timer {
        u8 cr;

        u8 pb_bit;
        u8 pb_bit_val;

        u8 t_ctrl;
        u8 t_os;

        u8 inmode;

        u16 timer;
        u16 latch;

        u8 pb_toggle;
        u8 pb_pulse;
    };

    struct TOD {
        struct Time_rep {
            enum Kind { tod = 0, alarm = 1, latch = 2 };

            u8 hr;
            u8 min;
            u8 sec;
            u8 tnth;

            bool operator==(const Time_rep& ct) {
                return (tnth == ct.tnth) && (sec == ct.sec) && (min == ct.min) && (hr == ct.hr);
            }
        };

        Time_rep time[3];

        Time_rep::Kind r_src;
        Time_rep::Kind w_dst;

        int tod_tick_timer = 0;
    };

    IO::Port::State port_a;
    IO::Port::State port_b;

    Int_ctrl int_ctrl;

    Timer timer_a;
    Timer timer_b;

    TOD tod;

    bool cnt;
};


struct System {
    enum class Mode {
        none, clocked, stepped, // warp, debug...? 
    };

    struct ROM {
        // TODO: include in State::System, if ROMs are to be included in sys_snap
        // (just the definition here for now...)
        const u8* basic;
        const u8* kernal;
        const u8* charr;
        const u8* c1541;
    };

    struct Bus {
        using RW = NMOS6502::MC::RW;

        u16 addr;
        u8 data;
        RW rw;
    };

    struct PLA {
        int active; // the active PLA array (set based on mode)

        u8 io_port_dd;
        u8 io_port_pd;
        u8 io_port_state;

        u8 exrom_game;

        u8 ultimax = false;
        u8 vic_bank = 0b11;
    };

    struct Int_hub {
        u8 state;
        u8 old_state;
        bool nmi_act;
        bool irq_act;
    };

    struct Input_matrix {
        // key states, 8x8 bits (64 keys)
        u64 key_states; // 'actual' state
        u64 kb_matrix;  // key states in the matrix (includes possible 'ghost' keys)

        // current output state of the cia-ports (any input bit will be set)
        u8 pa_state;
        u8 pb_state;

        // current state of controllers
        u8 cp2_state;
        u8 cp1_state;
    };

    Mode mode;

    u8 ram[RAM_SIZE];
    u8 color_ram[COLOR_RAM_SIZE] = {};

    u16 ba_low;
    u16 dma_low;

    Bus bus;

    PLA pla;

    u16 expansion_type;

    Int_hub int_hub;

    Input_matrix input_matrix; // problems with load/save state?

    VIC_II vic;

    NMOS6502::Core::State cpu;

    CIA cia1;
    CIA cia2;

    // holds all the memory that an expansion (e.g. a cart) requires: ROM, RAM,
    // emulation control data, ...
    // TODO: dynamic, or meh..? (although 640k is enough for everyone...except easyflash..)
    //       (nasty, if state saving is implemented)
    u8 exp_ram[640 * 1024];
};


} // namespace State


namespace System {

inline void update_pla(State::System& s) { // TODO: non-inlined? (definition to .cpp)
    static constexpr u8 Mode_to_array[32] = {
        0,  0,  1,  2,  0, 11,  3,  4, 0,  8,  9,  5,  0, 11, 12,  6,
        7,  7,  7,  7,  7,  7,  7,  7, 0,  8,  9, 10,  0, 11, 12, 13,
    };
    static constexpr u8 loram_hiram_charen_bits = 0x07;

    const u8 lhc = (s.pla.io_port_state | ~s.pla.io_port_dd) & loram_hiram_charen_bits; // inputs -> pulled up
    const u8 mode = s.pla.exrom_game | lhc;
    s.pla.active = Mode_to_array[mode];
}

inline void set_exrom_game(bool e, bool g, State::System& s) {
    s.pla.exrom_game = (e << 4) | (g << 3);
    update_pla(s);

    s.pla.ultimax = (e && !g);
}

} // namespace System


#endif // STATE_H_INCLUDED