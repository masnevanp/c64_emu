
#include "system.h"
#include <fstream>



/*
     m  x   g   c   h   l  00-0f 10-7f 80-9f a0-bf c0-cf d0-df e0-ff  pla_idx
    =========================================================================
    31  1   1   1   1   1   RAM   RAM   RAM   BAS   RAM   I/O   KER   : 13

    30  1   1   1   1   0   RAM   RAM   RAM   RAM   RAM   I/O   KER   : 12
    14  0   1   1   1   0   RAM   RAM   RAM   RAM   RAM   I/O   KER

    29  1   1   1   0   1   RAM   RAM   RAM   RAM   RAM   I/O   RAM   : 11
    13  0   1   1   0   1   RAM   RAM   RAM   RAM   RAM   I/O   RAM
    5   0   0   1   0   1   RAM   RAM   RAM   RAM   RAM   I/O   RAM

    27  1   1   0   1   1   RAM   RAM   RAM   BAS   RAM   CHRR  KER   : 10

    26  1   1   0   1   0   RAM   RAM   RAM   RAM   RAM   CHRR  KER   : 09
    10  0   1   0   1   0   RAM   RAM   RAM   RAM   RAM   CHRR  KER

    25  1   1   0   0   1   RAM   RAM   RAM   RAM   RAM   CHRR  RAM   : 08
    9   0   1   0   0   1   RAM   RAM   RAM   RAM   RAM   CHRR  RAM

    23  1   0   1   1   1   RAM   ---   CRL   ---   ---   I/O   CRH   : 07
    22  1   0   1   1   0   RAM   ---   CRL   ---   ---   I/O   CRH
    21  1   0   1   0   1   RAM   ---   CRL   ---   ---   I/O   CRH
    20  1   0   1   0   0   RAM   ---   CRL   ---   ---   I/O   CRH
    19  1   0   0   1   1   RAM   ---   CRL   ---   ---   I/O   CRH
    18  1   0   0   1   0   RAM   ---   CRL   ---   ---   I/O   CRH
    17  1   0   0   0   1   RAM   ---   CRL   ---   ---   I/O   CRH
    16  1   0   0   0   0   RAM   ---   CRL   ---   ---   I/O   CRH

    15  0   1   1   1   1   RAM   RAM   CRL   BAS   RAM   I/O   KER   : 06

    11  0   1   0   1   1   RAM   RAM   CRL   BAS   RAM   CHRR  KER   : 05

    7   0   0   1   1   1   RAM   RAM   CRL   CRH   RAM   I/O   KER   : 04

    6   0   0   1   1   0   RAM   RAM   RAM   CRH   RAM   I/O   KER   : 03

    3   0   0   0   1   1   RAM   RAM   CRL   CRH   RAM   CHRR  KER   : 02

    2   0   0   0   1   0   RAM   RAM   RAM   CRH   RAM   CHRR  KER   : 01

    28  1   1   1   0   0   RAM   RAM   RAM   RAM   RAM   RAM   RAM   : 00
    24  1   1   0   0   0   RAM   RAM   RAM   RAM   RAM   RAM   RAM
    12  0   1   1   0   0   RAM   RAM   RAM   RAM   RAM   RAM   RAM
    8   0   1   0   0   0   RAM   RAM   RAM   RAM   RAM   RAM   RAM
    4   0   0   1   0   0   RAM   RAM   RAM   RAM   RAM   RAM   RAM
    1   0   0   0   0   1   RAM   RAM   RAM   RAM   RAM   RAM   RAM
    0   0   0   0   0   0   RAM   RAM   RAM   RAM   RAM   RAM   RAM

*/

const System::PLA::Mapping System::PLA::array[14][2][16] { /*
    {
        {ram0_w, ram0_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r},
        {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r},
    },
    {
        {ram0_w, ram0_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r},
        {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, romh_r}, {ram_w, romh_r}, {ram_w, ram_r}, {ram_w, charr_r}, {ram_w, kern_r}, {ram_w, kern_r}
    },
    {
        {ram0_w, ram0_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r},
        {ram_w, roml_r}, {ram_w, roml_r}, {ram_w, romh_r}, {ram_w, romh_r}, {ram_w, ram_r}, {ram_w, charr_r}, {ram_w, kern_r}, {ram_w, kern_r}
    },
    {
        {ram0_w, ram0_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r},
        {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, romh_r}, {ram_w, romh_r}, {ram_w, ram_r}, {io_w, io_r}, {ram_w, kern_r}, {ram_w, kern_r}
    },
    {
        {ram0_w, ram0_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r},
        {ram_w, roml_r}, {ram_w, roml_r}, {ram_w, romh_r}, {ram_w, romh_r}, {ram_w, ram_r}, {io_w, io_r}, {ram_w, kern_r}, {ram_w, kern_r}
    },
    {
        {ram0_w, ram0_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r},
        {ram_w, roml_r}, {ram_w, roml_r}, {ram_w, bas_r}, {ram_w, bas_r}, {ram_w, ram_r}, {ram_w, charr_r}, {ram_w, kern_r}, {ram_w, kern_r}
    },
    {
        {ram0_w, ram0_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r},
        {ram_w, roml_r}, {ram_w, roml_r}, {ram_w, bas_r}, {ram_w, bas_r}, {ram_w, ram_r}, {io_w, io_r}, {ram_w, kern_r}, {ram_w, kern_r}
    },
    {
        {ram0_w, ram0_r}, {none_w, none_r}, {none_w, none_r}, {none_w, none_r}, {none_w, none_r}, {none_w, none_r}, {none_w, none_r}, {none_w, none_r},
        {roml_w, roml_r}, {roml_w, roml_r}, {none_w, none_r}, {none_w, none_r}, {none_w, none_r}, {io_w, io_r}, {romh_w, romh_r}, {romh_w, romh_r}
    },
    {
        {ram0_w, ram0_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r},
        {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, charr_r}, {ram_w, ram_r}, {ram_w, ram_r}
    },
    {
        {ram0_w, ram0_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r},
        {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, charr_r}, {ram_w, kern_r}, {ram_w, kern_r}
    },
    {
        {ram0_w, ram0_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r},
        {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, bas_r}, {ram_w, bas_r}, {ram_w, ram_r}, {ram_w, charr_r}, {ram_w, kern_r}, {ram_w, kern_r}
    },
    {
        {ram0_w, ram0_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r},
        {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {io_w, io_r}, {ram_w, ram_r}, {ram_w, ram_r}
    },
    {
        {ram0_w, ram0_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r},
        {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {io_w, io_r}, {ram_w, kern_r}, {ram_w, kern_r}
    },
    {
        {ram0_w, ram0_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r},
        {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, bas_r}, {ram_w, bas_r}, {ram_w, ram_r}, {io_w, io_r}, {ram_w, kern_r}, {ram_w, kern_r}
    },
    */
    {   // 00: modes 0, 1, 4, 8, 12, 24, 28
        { ram0_w,   ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,
          ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w  },
        { ram0_r,   ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,
          ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r  }
    },
    {   // 01: mode 2
        { ram0_w,   ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,
          ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w  },
        { ram0_r,   ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,
          ram_r,    ram_r,    romh_r,   romh_r,   ram_r,    charr_r,  kern_r,   kern_r }
    },
    {   // 02: mode 3
        { ram0_w,   ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,
          ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w  },
        { ram0_r,   ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,
          roml_r,   roml_r,   romh_r,   romh_r,   ram_r,    charr_r,  kern_r,   kern_r }
    },
    {   // 03: mode 6
        { ram0_w,   ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,
          ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    io_w,     ram_w,    ram_w  },
        { ram0_r,   ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,
          ram_r,    ram_r,    romh_r,   romh_r,   ram_r,    io_r,     kern_r,   kern_r }
    },
    {   // 04: mode 7
        { ram0_w,   ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,
          ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    io_w,     ram_w,    ram_w  },
        { ram0_r,   ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,
          roml_r,   roml_r,   romh_r,   romh_r,   ram_r,    io_r,     kern_r,   kern_r }
    },
    {   // 05: mode 11
        { ram0_w,   ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,
          ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w  },
        { ram0_r,   ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,
          roml_r,   roml_r,   bas_r,    bas_r,    ram_r,    charr_r,  kern_r,   kern_r }
    },
    {   // 06: mode 15
        { ram0_w,   ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,
          ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    io_w,     ram_w,    ram_w  },
        { ram0_r,   ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,
          roml_r,   roml_r,   bas_r,    bas_r,    ram_r,    io_r,     kern_r,   kern_r }
    },
    {   // 07: modes 16..23
        { ram0_w,   none_w,   none_w,   none_w,   none_w,   none_w,   none_w,   none_w,
          roml_w,   roml_w,   none_w,   none_w,   none_w,   io_w,     romh_w,   romh_w },
        { ram0_r,   none_r,   none_r,   none_r,   none_r,   none_r,   none_r,   none_r,
          roml_r,   roml_r,   none_r,   none_r,   none_r,   io_r,     romh_r,   romh_r }
    },
    {   // 08: modes 9, 25
        { ram0_w,   ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,
          ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w  },
        { ram0_r,   ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,
          ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    charr_r,  ram_r,    ram_r  }
    },
    {   // 09: modes 10, 26
        { ram0_w,   ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,
          ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w  },
        { ram0_r,   ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,
          ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    charr_r,  kern_r,   kern_r }
    },
    {   // 10: modes 27
        { ram0_w,   ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,
          ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w  },
        { ram0_r,   ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,
          ram_r,    ram_r,    bas_r,    bas_r,    ram_r,    charr_r,  kern_r,   kern_r }
    },
    {   // 11: modes 5, 13, 29
        { ram0_w,   ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,
          ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    io_w,     ram_w,    ram_w  },
        { ram0_r,   ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,
          ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    io_r,     ram_r,    ram_r  }
    },
    {   // 12: modes 14, 30
        { ram0_w,   ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,
          ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    io_w,     ram_w,    ram_w  },
        { ram0_r,   ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,
          ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    io_r,     kern_r,   kern_r }
    },
    {   // 13: mode 31
        { ram0_w,   ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,
          ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    io_w,     ram_w,    ram_w   },
        { ram0_r,   ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,
          ram_r,    ram_r,    bas_r,    bas_r,    ram_r,    io_r,     kern_r,   kern_r }
    },
};


void System::Input_matrix::update_matrix() {
    auto get_row = [](int n, u64& from) -> u64 { return (from >> (8 * n)) & 0xff; };
    auto set_row = [](int n, u64& to, u64 val) { to |= (val << (8 * n)); };

    s.kb_matrix = s.key_states;

    if (s.kb_matrix) { // any key down?
        // emulate GND propagation in the matrix (can produce 'ghost' keys)
        for (int ra = 0; ra < 7; ++ra) for (int rb = ra + 1; rb < 8; ++rb) {
            const u64 a = get_row(ra, s.kb_matrix);
            const u64 b = get_row(rb, s.kb_matrix);
            if (a & b) {
                const u64 r = a | b;
                set_row(ra, s.kb_matrix, r);
                set_row(rb, s.kb_matrix, r);
            }
        }
    }
}


void System::Input_matrix::output() {
    u8 pa = s.pa_state & s.cp2_state;
    u8 pb = s.pb_state & s.cp1_state;

    if (s.kb_matrix) { // any key down?
        u64 key = 0b1;
        for (int n = 0; n < 64; ++n, key <<= 1) {
            const bool key_down = s.kb_matrix & key;
            if (key_down) {
                // figure out connected lines & states
                const auto row_bit = (0b1 << (n / 8));
                const auto pa_bit = pa & row_bit;
                const auto col_bit = (0b1 << (n % 8));
                const auto pb_bit = pb & col_bit;

                if (!pa_bit || !pb_bit) { // at least one connected line is low?
                    // pull both lines low
                    pa &= (~row_bit);
                    pb &= (~col_bit);
                }
            }
        }
    }

    pa_in(0b11111111, pa);
    pb_in(0b11111111, pb);

    static constexpr u8 cia1_pb_lp_bit = 0b00010000;
    const auto lp_low = !(pb & cia1_pb_lp_bit);
    lp(lp_low);
}


void System::Menu::handle_key(u8 code) {
    using kc = Key_code::System;

    if (!active) {
        active = true;
    } else {
        switch (code) {
            case kc::menu_ent:  root.enter(); break;
            case kc::menu_exit: root.exit();  break;
            case kc::menu_up:   root.up();    break;
            case kc::menu_down: root.down();  break;
        }
    }
}

/*
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

*/


/*
- two look-up tables: keycode_to_ascii & keycode_shifted_to_ascii (need to keep track of shift)
- special handling
    - return
    - del
    - shift l/r
    - curs r/d
    - r_stp, cmdre, ctrl?
    - function keys?
*/

void System::Monitor::key(u8 code, u8 down) {
    using kc = Key_code::Keyboard;

    UNUSED2(code, down); // TODO
    screen[0][0] = 't'; screen[0][1] = 'e'; screen[0][2] = 's'; screen[0][3] = 't';
    screen[29][0] = 't'; screen[29][1] = 'e'; screen[29][2] = 's'; screen[29][3] = 't';
    if (down) {
        if (code == kc::crs_d) crsr_y += 1;
        if (code == kc::crs_r) crsr_x += 1;
    }
}


u8* System::Monitor::draw(const u8* charrom) {
    auto draw_chr = [&](u16 chr, int y, int x) {
        PETSCII_Draw{charrom, frame, VIC_II::FRAME_WIDTH}.chr(
            chr,
            text_top_left_x + (x * 8), text_top_left_y + (y * 8),
            color_fg, color_bg
        );
    };

    auto draw_cursor = [&]() {
        const auto chr_at_crsr = ascii_to_char_rom(screen[crsr_y][crsr_x]);
        const auto rvrs_chr_at_crsr = chr_at_crsr | 0x080;
        draw_chr(rvrs_chr_at_crsr, crsr_y, crsr_x);
    };

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            // TODO: maybe store also the char_rom mapping with the ascii code
            //       (or maybe use std::string for a line of chars ==> pd.txt() can be used)
            const auto chr = ascii_to_char_rom(screen[y][x]);
            draw_chr(chr, y, x);
        }
    }


    draw_cursor();

    return frame;
}


void System::C64::run(Mode init_mode) {
    s.mode = init_mode;

    reset_cold();

    do {
        switch (s.mode) {
            case Mode::none: break;
            case Mode::clocked:   run_clocked();   break;
            case Mode::stepped:   run_stepped();   break;
            case Mode::unlimited: run_unlimited(); break;
        }
        pre_run();
    }
    while (s.mode != Mode::none);
}


void System::C64::check_deferred() {
    if (deferred) {
        deferred();
        deferred = nullptr;
    }
}


char mapped_at(const System::Bus& bus, const u16 addr, const State::System::Bus::RW rw) {
    static constexpr char mc[] = {
        'r', 'r', 'r', 'r', 'b', 'k', 'c', 'l', 'l', 'h', 'h', 'i', 'i', '.', '.'
    };
    const auto m = bus.mapped_at(addr, rw);
    return mc[m];
};


void System::C64::log_cpu_status() {
    auto nmi_irq_srcs = [&](const State::System::Int_hub& int_hub) {
        using Src = IO::Int_sig::Src;

        std::string s = "...|...";

        if (int_hub.state & Src::cia2) s[0] = 'c';
        if (int_hub.old_state & Src::rstr) s[1] = 'r'; // using 'old_state', since it is cleared immediately
        if (int_hub.state & Src::exp_n) s[2] = 'e';
        if (int_hub.state & Src::cia1) s[4] = 'c';
        if (int_hub.state & Src::vic) s[5] = 'v';
        if (int_hub.state & Src::exp_i) s[6] = 'e';

        return s;
    };

    auto rdy_srcs = [&]() {
        /*
        std::string rs = "..........";
        if (s.ba) {
            for (int mn = 0; mn < 8; ++mn) if (s.ba & (0x0100 << mn)) rs[mn] = std::to_string(mn)[0];
            if (s.ba & 0x00ff) rs[8] = 'g';
        }
        */
        std::string rs = "...";

        if (s.dma) rs[0] = 'd';
        if (s.ba & 0x00ff) rs[1] = 'g';
        if (s.ba & 0xff00) rs[2] = 's';

        return rs;
    };

    const auto& c = s.cpu;

    const int frame = s.vic.cycle / FRAME_CYCLE_COUNT;
    const int line = (s.vic.cycle / LINE_CYCLE_COUNT) % FRAME_LINE_COUNT;
    const int line_cycle = s.vic.cycle % LINE_CYCLE_COUNT;

    std::string disasm;
    if (cpu.at_fetch()) {
        const auto pc = c.bus.a;
        const auto bytes = Bytes{{bus.peek(pc), bus.peek(pc + 1), bus.peek(pc + 2)}};
        const auto line = MOS6502::Asm::disasm_first(bytes, pc);
        disasm = "> " + as_lower(line.text);
    } else {
        disasm = ".";
    }

    // we need to peek, since the read has not happened yet (i.e. c.bus.d is the 'old' value....)
    const auto bus_d = c.bus.rw ? bus.peek(c.bus.a) : c.bus.d;

    Log::info("%06d.%03d.%02d  %c %02x %04x [%c] %-14s axysp: %02x %02x %02x %03x [%s]  %c%c%c: [%s|%s]",
        frame, line, line_cycle,
        (c.bus.rw ? 'r' : 'w'), bus_d, c.bus.a, mapped_at(bus, c.bus.a, c.bus.rw),
        disasm.c_str(),
        c.a, c.x, c.y, c.sp, Dbg::flags_str(c.p).c_str(),
        (c.nmi_act ? 'n' : '.'), (c.irq_act ? 'i' : '.'), ((s.ba || s.dma) ? 'r' : '.'),
        nmi_irq_srcs(s.int_hub).c_str(), rdy_srcs().c_str()
    );
}


void System::C64::log_sys_status() {
    using RW = State::System::Bus::RW;

    char buffer[128];

    auto pla_mode_str = [&](u8 mode) {
        std::string mode_str = ".....";

        if (mode & 0b10000) mode_str[0] = 'e';
        if (mode & 0b01000) mode_str[1] = 'g';
        if (mode & 0b00100) mode_str[2] = 'c';
        if (mode & 0b00010) mode_str[3] = 'h';
        if (mode & 0b00001) mode_str[4] = 'l';

        return mode_str;
    };

    auto rw_mappings = [&]() {
        static constexpr u16 zone_addr[] = {
            0x0000, 0x1000, 0x8000, 0xa000, 0xc000, 0xd000, 0xe000,
        };

        std::string rw = ".......|.......";
        for (int z = 0; z < 7; ++z) {
            rw[z] = mapped_at(bus, zone_addr[z], RW::r);
            rw[z + 8] = mapped_at(bus, zone_addr[z], RW::w);
        }

        return rw;
    };

    const char* format = "c: %d   pla: %02d [%s => %s]";
    sprintf(buffer, format,
        s.vic.cycle,
        System::pla_mode(s), pla_mode_str(System::pla_mode(s)).c_str(), rw_mappings().c_str()
    );

    Log::info("%s", buffer);
}


void System::C64::pre_run() {
    switch (s.mode) {
        case Mode::none: break;
        case Mode::clocked:
            vid_out.flip();
            sid.flush();
            frame_timer.reset();
            watch.start();
            break;
        case Mode::stepped:
            sid.flush();
            log_sys_status();
            log_cpu_status();
            break;
        case Mode::unlimited:
            sid.flush();
            break;
    }
}


void System::C64::run_clocked() {
    auto sync = [&]() {
        const auto frame_duration = [&]() { return Timer::one_second() / perf.frame_rate.chosen; };

        const bool frame_done = (s.vic.cycle % FRAME_CYCLE_COUNT) == 0;
        if (frame_done) {
            watch.stop();
            
            if (vid_out.v_synced()) {
                output_frame();
                frame_timer.reset();
            } else {
                frame_timer.wait_elapsed(frame_duration(), true);
                output_frame();
            }
            
            watch.start();
            
            sid.sync();

            host_input.poll();
            
            watch.stop();
            watch.reset();

            check_deferred();
        } else {
            watch.stop();
            
            const auto frame_progress = double(s.vic.raster_y) / double(FRAME_LINE_COUNT);
            const auto frame_progress_time = frame_progress * frame_duration();
            frame_timer.wait_elapsed(frame_progress_time);
            
            watch.start();
            
            sid.sync();

            host_input.poll();
        }
    };

    // TODO: consider special loops for different configs (e.g. REU with no 1541)
    while (s.mode == Mode::clocked) {
        run_cycle();
        const bool sync_point = (s.vic.cycle % perf.latency.chosen.sync_freq) == 0;
        if (sync_point) sync();
    }
}


void System::C64::run_stepped() {
    frame_timer.reset();

    while (s.mode == Mode::stepped) {
        output_frame();
        frame_timer.wait_elapsed(Timer::one_second() / 50.0, true);
        host_input.poll();
        check_deferred();
    }
}


void System::C64::run_unlimited() {
    auto frame_done = [&]() {
        const bool the_50th_frame = ((s.vic.cycle / FRAME_CYCLE_COUNT) % 50) == 0;
        if (the_50th_frame) {
            output_frame();
            host_input.poll();
            check_deferred();
        }
        sid.sync(false);
    };

    while (s.mode == Mode::unlimited) {
        run_cycle();
        const bool frame_is_done = (s.vic.cycle % FRAME_CYCLE_COUNT) == 0;
        if (frame_is_done) frame_done();
    }
}


// TODO: when stepping cycle/instr/line, add a beam_pos indicator (line/dot?)
void System::C64::step_forward(u8 key_code) {
    using kc = Key_code::System;

    // 'clear' remaining pixels (yes, most of the time this is redundant, but meh... )
    for (auto bp = s.vic.beam_pos; bp < VIC_II::FRAME_SIZE; ++bp)
        s.vic.frame[bp] = Color::black;

    switch (key_code) {
        case kc::step_cycle:
            run_cycle();
            break;
        case kc::step_instr:
            if (!cpu.halted()) {
                do run_cycle(); while (!cpu.at_fetch());
            }
            break;
        case kc::step_line:
            do run_cycle(); while (s.vic.line_cycle() < (LINE_CYCLE_COUNT - 1));
            break;
        case kc::step_frame:
            do run_cycle(); while (s.vic.frame_cycle() < (FRAME_CYCLE_COUNT - 1));
            break;
    }

    sid.sync();

    log_cpu_status();
}


void System::C64::output_frame() { 
    auto draw_menu = [&](PETSCII_Draw& pd) {
        if (!menu.active) return;

        static const int width_chr = 39;
        static const int pos_x = VIC_II::BORDER_SZ_V + 4;
        static const int pos_y = (VIC_II::FRAME_HEIGHT - VIC_II::BORDER_SZ_H) + 4;
        static const int pad_px = 4;
        static const Color col_fg = Color::light_green;
        static const Color col_bg = Color::gray_1;

        pd.txt(std::string(width_chr, ' '), pos_x, pos_y, col_fg, col_bg);
        pd.txt(menu.text(), pos_x + pad_px, pos_y, col_fg, col_bg);

        /*
        // draw char.rom (fun & profit...)
        const int x = 64;
        const int y = 64 + 1;
 
        for (u16 c = 0; c < 512; ++c) {
            const int col = c % 32;
            const int row = c / 32;
            pd.chr(c, x + (col * 8), y + (row * 8), col_fg, col_bg);
        }
        */
    };

    auto draw_status = [&](PETSCII_Draw& pd) {

        auto draw_c1541_led = [&]() {
            static const int pos_y = (VIC_II::FRAME_HEIGHT - VIC_II::BORDER_SZ_H) + 14;;
            static const int pos_x = VIC_II::FRAME_WIDTH - VIC_II::BORDER_SZ_V - 12;

            static const Color col_bg = Color::black;
            static const Color col_led_on = Color::light_green;
            static const Color col_led_off = Color::gray_1;

            static const u16 ch_led_wp = 0x0051;
            static const u16 ch_led    = 0x0057;

            const auto led_ch = c1541.dc.status.write_prot_on() ? ch_led_wp : ch_led;
            const auto led_col = c1541.dc.status.led_on() ? col_led_on : col_led_off;

            pd.chr(led_ch, pos_x, pos_y, led_col, col_bg);

            /*static constexpr u16 ch_zero   = 0x0030;
            const auto track_n = (status.head.track_n / 2) + 1;
            const auto tn_col = status.head.active() ? Color::green : Color::gray_2;
            draw_char(ch_zero + track_n / 10, x + 8 + 2,  y, tn_col, col_bg);
            draw_char(ch_zero + track_n % 10, x + 16 + 2, y, tn_col, col_bg);*/
        };

        auto draw_disk_and_exp_names = [&]() {
            static const int width_chr = 39;
            static const int pos_x = VIC_II::BORDER_SZ_V + 4;
            static const int pad_px = 4;

            static const Color col_fg = Color::light_green;
            static const Color col_bg = Color::gray_1;

            if (!c1541.disk_carousel.no_disk()) {
                static const int pos_y = 14;
                pd.txt(std::string(width_chr, ' '), pos_x, pos_y, col_fg, col_bg);
                const std::string txt = "8: " + c1541.disk_carousel.selected().disk_name;
                pd.txt(txt, pos_x + pad_px, pos_y, col_fg, col_bg);
            }

            if (s.exp.type != Expansion::Type::none) {
                static const int pos_y = 4;
                pd.txt(std::string(width_chr, ' '), pos_x, pos_y, col_fg, col_bg);
                const std::string txt = "e: " + std::string(s.exp.name);
                pd.txt(txt, pos_x + pad_px, pos_y, col_fg, col_bg);
            }
        };

        if (show_status) {
            draw_c1541_led();
            draw_disk_and_exp_names();
        } else if (c1541.dc.status.head.active()) {
            draw_c1541_led();
        }
    };

    u8* const frame_out = monitor.active ? monitor.draw(rom.charr) : s.vic.frame;
    PETSCII_Draw pd{rom.charr, frame_out, VIC_II::FRAME_WIDTH};

    draw_menu(pd);
    draw_status(pd);

    vid_out.put(frame_out);
}


void System::C64::reset_warm() {
    Log::info("System reset (warm)");
    cia1.reset_warm(); // need to reset for correct irq handling
    cia2.reset_warm();
    cpu.reset();
    int_hub.reset();
}


void System::C64::reset_cold() {
    Log::info("System reset (cold)");

    auto init_ram = [&]() { // TODO: parameterize pattern (+ add 'randomness'?)
        for (int addr = 0x0000; addr <= 0xffff; ++addr)
            s.ram[addr] = (addr & 0x80) ? 0xff : 0x00;
    };

    init_ram();

    s.ba = false;
    s.dma = false;

    cia1.reset_cold();
    cia2.reset_cold();
    sid.reset();
    vic.reset();
    input_matrix.reset();
    bus.reset();
    cpu.reset();
    int_hub.reset();
    c1541.reset();

    pre_run();
}


void System::C64::save_state_req() {
    static const std::string dir = "./_local"; // TODO...

    // TODO: spawn thread (a copy of sys_snap required though...)?
    deferred = [&]() {
        sys_snap.sid = sid.core.read_state();

        const std::string filepath = as_lower(dir + "/emu.state"); // TODO...
        // TODO: hadle exceptions?
        if (auto f = std::ofstream(filepath, std::ios::binary)) {
            f.write((const char*)&sys_snap, sizeof(sys_snap));
            if (f) Log::info("State saved: %s", filepath.c_str());
            else Log::error("save state failed");
        }
    };
};


bool System::C64::handle_file(Files::File& file) {
    auto inject = [&](const Bytes& data) {
        // load addr (used if 2nd.addr == 0)
        s.ram[0xc3] = cpu.s.x;
        s.ram[0xc4] = cpu.s.y;

        const u8 scnd_addr = s.ram[0xb9];
        u16 addr = (scnd_addr == 0)
            ? cpu.s.y * 0x100 + cpu.s.x
            : data[1] * 0x100 + data[0];

        // 'load'
        for (u32 b = 2; b < data.size(); ++b) s.ram[addr++] = data[b];

        // end pointer
        s.ram[0xae] = cpu.s.x = addr;
        s.ram[0xaf] = cpu.s.y = addr >> 8;
    };

    using Type = Files::File::Type;

    // NOTE: 'crt' & 'sys_snap' loads are deferred, because otherwise we could be
    //       jumping in mid-cycle (because loading can be triggerd also by the cpu.tick())
    switch (file.type) {
        case Type::crt: {
            Log::info("CRT '%s' ...", file.name.c_str());
            deferred = [&, name = file.name, data = std::move(file.data)]() {
                Expansion::attach(s, name, Files::CRT{data});
                reset_cold();
            };
            return true;
        }
        case Type::d64: // TODO: maybe for d64&g64 the name should be squashed elsewhere...
                        //       (TBD if/when the state file includes the disk_carousel)
            // auto slot = s.ram[0xb9]; // secondary address
            // slot=0 --> first free slot
            c1541.disk_carousel.insert(0,
                    new C1541::D64(Files::D64{file.data}), as_lower(squash(file.name, 35)));
            return true;
        case Type::g64:
            c1541.disk_carousel.insert(0,
                    new C1541::G64(std::move(file.data)), as_lower(squash(file.name, 35)));
            return true;
        case Type::c64_bin:
            inject(file.data);
            return true;
        case Type::sys_snap: {
            deferred = [&, d = std::move(file.data)]() {
                Files::System_snapshot& iss = *((Files::System_snapshot*)d.data()); // brutal...
                sys_snap.sys_state = iss.sys_state;
                sid.core.write_state(iss.sid);
                pre_run(); // NOTE: required for now (see 'sid.h' for more info)
            };
            return true;
        }
        default:
            Log::info("'%s' ignored", file.name.c_str());
            return false;
    }
    // TODO: support 't64' (e.g. cold reset & feed the appropriate 'LOAD'
    // command + 'RETURN' key into the C64 keyboard input buffer)
};


std::string get_filename(const u8* ram) {
    const u16 filename_addr = ram[0xbc] * 0x100 + ram[0xbb];
    const u8 filename_len = ram[0xb7];
    return std::string(&ram[filename_addr], &ram[filename_addr + filename_len]);
}


void System::C64::do_load() {
    // TODO: use secondary address as an action id, e.g.:
    //           'LOAD "SOME.CRT",1,2' --> inspect only (i.e. generate_basic_info_list & inject)
    auto file = loader(get_filename(s.ram));
    if (file.identified() && handle_file(file)) {
        if (auto info_file = generate_basic_info_list(file); info_file) {
            // TOFIX: Here we rely on the fact, that 'generate_basic_info_list'
            //        currently supports only D64s (so for example we don't corrupt
            //        a restored snapshot with an injected info listing...)
            handle_file(info_file);
        }

        //'return' status to kernal routine
        cpu.s.clr(MOS6502::Flag::C); // no error
        s.ram[0x90] = 0x00; // io status ok
    } else {
        cpu.s.pc = 0xf704; // --> file not found
    }
}


// TODO: identify the addresses used
void System::C64::do_save() {
    static const std::string dir = "./_local"; // TODO...

    auto do_save = [](const std::string& filename, u16 start_addr, u8* data, size_t sz) -> bool {
        // TODO: hadle exceptions?
        if (auto f = std::ofstream(filename, std::ios::binary)) {
            f << (u8)(start_addr) << (u8)(start_addr >> 8);
            f.write((char*)data, sz);
            if (f) return true;
        }

        return false;
    };

    const std::string filename = get_filename(s.ram);
    if (filename.length() == 0) {
        // TODO: error_code enum(s)
        cpu.s.a = 0x08; // missing filename
        cpu.s.set(MOS6502::Flag::C); // error
        return;
    }

    const std::string filepath = as_lower(dir + "/" + filename);

    // TODO: end < start ??
    const u16 start_addr = s.ram[0xc2] * 0x100 + s.ram[0xc1];
    const u16 end_addr = s.ram[0xaf] * 0x100 + s.ram[0xae]; // 1 beyond end
    const u16 sz = end_addr - start_addr;

    if (do_save(filepath, start_addr, &s.ram[start_addr], sz)) {
        // status
        cpu.s.clr(MOS6502::Flag::C); // no error
        s.ram[0x90] = 0x00; // io status ok
        Log::info("Saved '%s', %d bytes", filepath.c_str(), int(sz + 2));
    } else {
        cpu.s.a = 0x07; // not output file
        cpu.s.set(MOS6502::Flag::C); // error
        Log::error("Failed to save '%s'", filename.c_str());
    }
}


// 'halt' instructions used for trapping (i.e. 'illegal' instructions that normally
// would halt the CPU). The byte following the 'halt' is used to identify the 'request'.
void System::C64::install_kernal_tape_traps(u8* kernal, u8 trap_opc) {
    static constexpr u16 kernal_start = 0xe000;

    // trap loading (device 1)
    kernal[0xf539 - kernal_start] = trap_opc;
    kernal[0xf53a - kernal_start] = Trap_ID::load;
    kernal[0xf53b - kernal_start] = MOS6502::OPC::rts;

    // trap saving (device 1)
    kernal[0xf65f - kernal_start] = trap_opc;
    kernal[0xf660 - kernal_start] = Trap_ID::save;
    kernal[0xf661 - kernal_start] = MOS6502::OPC::rts;
}
