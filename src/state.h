#ifndef STATE_H_INCLUDED
#define STATE_H_INCLUDED

#include "common.h"


namespace State {


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

    struct Address_space {
        u8 ultimax = false;
        u8 bank = 0b11;
    };

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
    u16 raster_y_cmp = 0;
    u8 raster_y_cmp_edge;
    V_blank v_blank = V_blank::vb_on;

    Address_space addr_space;
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


struct System {
    u8 ram[0x10000];
    u8 color_ram[::VIC_II::Color_RAM::size] = {};

    u16 ba_low;
    u16 dma_low;

    VIC_II vic;

    // holds all the memory that an expansion (e.g. a cart) requires: ROM, RAM,
    // emulation control data, ...
    // TODO: dynamic, or meh..? (although 640k is enough for everyone...except easyflash..)
    //       (nasty, if state saving is implemented)
    u8 exp_ram[640 * 1024];
};


} // namespace State


#endif // STATE_H_INCLUDED