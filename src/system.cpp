
#include "system.h"
#include <fstream>




void System::Address_space::update_state() { // output bits set from 'pd', input bits unchanged
    s.io_port_state = (s.io_port_pd & s.io_port_dd) | (s.io_port_state & ~s.io_port_dd);
    set_pla();
}


void System::Address_space::set_pla() {
    const u8 lhc = (s.io_port_state | ~s.io_port_dd) & loram_hiram_charen_bits; // inputs -> pulled up
    const u8 mode = s.exrom_game | lhc;
    s.pla_line = Mode_to_PLA_line[mode];
}


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

const System::Address_space::PLA_Line System::Address_space::PLA[14] = {
    /*
    PLA_Line {{
        {ram0_w, ram0_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r},
        {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r},  }},
    PLA_Line {{
        {ram0_w, ram0_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r},
        {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, romh_r}, {ram_w, romh_r}, {ram_w, ram_r}, {ram_w, charr_r}, {ram_w, kern_r}, {ram_w, kern_r},  }},
    PLA_Line {{
        {ram0_w, ram0_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r},
        {ram_w, roml_r}, {ram_w, roml_r}, {ram_w, romh_r}, {ram_w, romh_r}, {ram_w, ram_r}, {ram_w, charr_r}, {ram_w, kern_r}, {ram_w, kern_r},  }},
    PLA_Line {{
        {ram0_w, ram0_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r},
        {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, romh_r}, {ram_w, romh_r}, {ram_w, ram_r}, {io_w, io_r}, {ram_w, kern_r}, {ram_w, kern_r},  }},
    PLA_Line {{
        {ram0_w, ram0_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r},
        {ram_w, roml_r}, {ram_w, roml_r}, {ram_w, romh_r}, {ram_w, romh_r}, {ram_w, ram_r}, {io_w, io_r}, {ram_w, kern_r}, {ram_w, kern_r},  }},
    PLA_Line {{
        {ram0_w, ram0_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r},
        {ram_w, roml_r}, {ram_w, roml_r}, {ram_w, bas_r}, {ram_w, bas_r}, {ram_w, ram_r}, {ram_w, charr_r}, {ram_w, kern_r}, {ram_w, kern_r},  }},
    PLA_Line {{
        {ram0_w, ram0_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r},
        {ram_w, roml_r}, {ram_w, roml_r}, {ram_w, bas_r}, {ram_w, bas_r}, {ram_w, ram_r}, {io_w, io_r}, {ram_w, kern_r}, {ram_w, kern_r},  }},
    PLA_Line {{
        {ram0_w, ram0_r}, {none_w, none_r}, {none_w, none_r}, {none_w, none_r}, {none_w, none_r}, {none_w, none_r}, {none_w, none_r}, {none_w, none_r},
        {roml_w, roml_r}, {roml_w, roml_r}, {none_w, none_r}, {none_w, none_r}, {none_w, none_r}, {io_w, io_r}, {romh_w, romh_r}, {romh_w, romh_r},  }},
    PLA_Line {{
        {ram0_w, ram0_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r},
        {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, charr_r}, {ram_w, ram_r}, {ram_w, ram_r},  }},
    PLA_Line {{
        {ram0_w, ram0_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r},
        {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, charr_r}, {ram_w, kern_r}, {ram_w, kern_r},  }},
    PLA_Line {{
        {ram0_w, ram0_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r},
        {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, bas_r}, {ram_w, bas_r}, {ram_w, ram_r}, {ram_w, charr_r}, {ram_w, kern_r}, {ram_w, kern_r},  }},
    PLA_Line {{
        {ram0_w, ram0_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r},
        {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {io_w, io_r}, {ram_w, ram_r}, {ram_w, ram_r},  }},
    PLA_Line {{
        {ram0_w, ram0_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r},
        {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {io_w, io_r}, {ram_w, kern_r}, {ram_w, kern_r},  }},
    PLA_Line {{
        {ram0_w, ram0_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r},
        {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, bas_r}, {ram_w, bas_r}, {ram_w, ram_r}, {io_w, io_r}, {ram_w, kern_r}, {ram_w, kern_r},  }},
    */
    PLA_Line {{ // 00: modes 0, 1, 4, 8, 12, 24, 28
        { ram0_w,   ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,
          ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w   },
        { ram0_r,   ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,
          ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r   } }},
    PLA_Line {{ // 01: mode 2
        { ram0_w,   ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,
          ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w   },
        { ram0_r,   ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,
          ram_r,    ram_r,    romh_r,   romh_r,   ram_r,    charr_r,  kern_r,   kern_r } }},
    PLA_Line {{ // 02: mode 3
        { ram0_w,   ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,
          ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w   },
        { ram0_r,   ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,
          roml_r,   roml_r,   romh_r,   romh_r,   ram_r,    charr_r,  kern_r,   kern_r } }},
    PLA_Line {{ // 03: mode 6
        { ram0_w,   ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,
          ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    io_w,     ram_w,    ram_w   },
        { ram0_r,   ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,
          ram_r,    ram_r,    romh_r,   romh_r,   ram_r,    io_r,     kern_r,   kern_r } }},
    PLA_Line {{ // 04: mode 7
        { ram0_w,   ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,
          ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    io_w,     ram_w,    ram_w   },
        { ram0_r,   ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,
          roml_r,   roml_r,   romh_r,   romh_r,   ram_r,    io_r,     kern_r,   kern_r } }},
    PLA_Line {{ // 05: mode 11
        { ram0_w,   ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,
          ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w   },
        { ram0_r,   ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,
          roml_r,   roml_r,   bas_r,    bas_r,    ram_r,    charr_r,  kern_r,   kern_r } }},
    PLA_Line {{ // 06: mode 15
        { ram0_w,   ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,
          ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    io_w,     ram_w,    ram_w   },
        { ram0_r,   ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,
          roml_r,   roml_r,   bas_r,    bas_r,    ram_r,    io_r,     kern_r,   kern_r } }},
    PLA_Line {{ // 07: modes 16..23
        { ram0_w,   none_w,   none_w,   none_w,   none_w,   none_w,   none_w,   none_w,
          roml_w,   roml_w,   none_w,   none_w,   none_w,   io_w,     romh_w,   romh_w },
        { ram0_r,   none_r,   none_r,   none_r,   none_r,   none_r,   none_r,   none_r,
          roml_r,   roml_r,   none_r,   none_r,   none_r,   io_r,     romh_r,   romh_r } }},
    PLA_Line {{ // 08: modes 9, 25
        { ram0_w,   ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,
          ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w   },
        { ram0_r,   ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,
          ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    charr_r,  ram_r,    ram_r   } }},
    PLA_Line {{ // 09: modes 10, 26
        { ram0_w,   ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,
          ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w   },
        { ram0_r,   ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,
          ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    charr_r,  kern_r,   kern_r } }},
    PLA_Line {{ // 10: modes 27
        { ram0_w,   ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,
          ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w   },
        { ram0_r,   ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,
          ram_r,    ram_r,    bas_r,    bas_r,    ram_r,    charr_r,  kern_r,   kern_r } }},
    PLA_Line {{ // 11: modes 5, 13, 29
        { ram0_w,   ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,
          ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    io_w,     ram_w,    ram_w   },
        { ram0_r,   ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,
          ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    io_r,     ram_r,    ram_r   } }},
    PLA_Line {{ // 12: modes 14, 30
        { ram0_w,   ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,
          ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    io_w,     ram_w,    ram_w   },
        { ram0_r,   ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,
          ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    io_r,     kern_r,   kern_r } }},
    PLA_Line {{ // 13: mode 31
        { ram0_w,   ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,
          ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    io_w,     ram_w,    ram_w   },
        { ram0_r,   ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,
          ram_r,    ram_r,    bas_r,    bas_r,    ram_r,    io_r,     kern_r,   kern_r } }},
};

const u8 System::Address_space::Mode_to_PLA_line[32] = {
    0,  0,  1,  2,  0, 11,  3,  4, 0,  8,  9,  5,  0, 11, 12,  6,
    7,  7,  7,  7,  7,  7,  7,  7, 0,  8,  9, 10,  0, 11, 12, 13,
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

    vic.set_lp(VIC::LP_src::cia, pb & VIC_II::CIA1_PB_LP_BIT);
}


void System::Video_overlay::draw_chr(u16 chr, u16 cx, u16 cy, Color fg, Color bg) {
    const u8* src = &charrom[chr * 8];
    for (int px_row = 0; px_row < 8; ++px_row, ++src) {
        u8* tgt = &pixels[(px_row + cy) * w + cx];
        for (u8 px = 0b10000000; px; px >>= 1) *tgt++ = (*src & px) ? fg : bg;
    }
}


System::Menu::Menu(std::initializer_list<std::pair<std::string, std::function<void ()>>> imm_actions_,
    std::initializer_list<std::pair<std::string, std::function<void ()>>> actions_,
    std::initializer_list<::Menu::Group> subs_,
    const u8* charrom)
  : video_overlay(pos_x, pos_y, 36 * 8, 8, charrom), subs(subs_)
{
    auto Imm_action = [&](const std::pair<std::string, std::function<void ()>>& a) {
        return ::Menu::Kludge(a.first, [act = a.second, &vis = video_overlay.visible](){ act(); vis = false; return nullptr; });
    };
    auto Action = [&](const std::pair<std::string, std::function<void ()>>& a) {
        return ::Menu::Action(a.first, [act = a.second, &vis = video_overlay.visible](){ act(); vis = false; return nullptr; });
    };

    for (auto a : imm_actions_) imm_actions.push_back(Imm_action(a));
    for (auto a : actions_) actions.push_back(Action(a));

    imm_actions.push_back(Imm_action({"^", [&](){}}));

    main_menu.add(actions);
    main_menu.add(subs);
    main_menu.add(imm_actions);

    for (auto& sub : subs) { // link all to all
        sub.add(subs);
        sub.add(actions);
        sub.add(imm_actions);
    }
}


void System::Menu::handle_key(u8 code) {
    using kc = Key_code::System;

    if (video_overlay.visible) {
        switch (code) {
            case kc::menu_ent:  ctrl.key_enter(); break;
            case kc::menu_up:   ctrl.key_up();    break;
            case kc::menu_down: ctrl.key_down();  break;
        }
    } else {
        video_overlay.visible = true;
    }

    update();
}


void System::Menu::toggle_visibility()  {
    if (!video_overlay.visible) ctrl.select(&main_menu);
    video_overlay.visible = !video_overlay.visible;
    update();
}


void System::Menu::update() {
    video_overlay.clear(col_bg);

    const std::string text = ctrl.state();
    auto x = 4;
    for (u8 ci = 0; ci < text.length(); ++ci, x += 8) {
        const auto c = (text[ci] % 64) + 256; // map ascii code to petscii (using the 'lower case' half)
        video_overlay.draw_chr(c, x, 0, col_fg, col_bg);
    }
}


void System::C1541_status_panel::update(const C1541::Disk_ctrl::Status& c1541_status) {
    static const u16 ch_led_wp = 0x0051;
    static const u16 ch_led    = 0x0057;

    const auto led_ch = c1541_status.wp_off() ? ch_led : ch_led_wp;
    const auto led_col = c1541_status.led_on() ? col_led_on : col_led_off;
    video_overlay.draw_chr(led_ch, 0, 0, led_col, col_bg);

    /*static constexpr u16 ch_zero   = 0x0030;
    const auto track_n = (status.head.track_n / 2) + 1;
    const auto tn_col = status.head.active() ? Color::green : Color::gray_2;
    draw_char(ch_zero + track_n / 10, x + 8 + 2,  y, tn_col, col_bg);
    draw_char(ch_zero + track_n % 10, x + 16 + 2, y, tn_col, col_bg);*/
}


void System::C64::init_sync() {
    vid_out.flip();
    sid.flush();
    frame_timer.reset();
    watch.start();
}


void System::C64::sync() {
    const auto frame_duration_us = [&]() { return 1000000.0 / perf.frame_rate.chosen; };

    const bool frame_done = (s.vic.cycle % FRAME_CYCLE_COUNT) == 0;
    if (frame_done) {
        host_input.poll();

        watch.stop();

        if (vid_out.v_synced()) {
            output_frame();
            frame_timer.reset();
        } else {
            frame_timer.wait_elapsed(frame_duration_us(), true);
            output_frame();
        }

        watch.start();

        sid.output();

        watch.stop();
        watch.reset();
    } else {
        host_input.poll();

        watch.stop();

        const auto frame_progress = double(s.vic.raster_y) / double(FRAME_LINE_COUNT);
        const auto frame_progress_us = frame_progress * frame_duration_us();
        frame_timer.wait_elapsed(frame_progress_us);

        watch.start();

        sid.output();
    }
}


void System::C64::output_frame() {
    auto output_overlay = [&](Video_overlay& ol) {
        const u8* src = ol.pixels;
        for (int y = 0; y < ol.h; ++y) {
            u8* tgt = &s.vic.frame[((ol.y + y) * VIC_II::FRAME_WIDTH) + ol.x];
            for (int x = 0; x < ol.w; ++x, ++src) {
                const auto col = *(src);
                /*if (col != Video_overlay::transparent)*/ tgt[x] = col;
            }
        }
    };

    if (menu.video_overlay.visible) {
        output_overlay(menu.video_overlay);
        c1541_status_panel.update(c1541.dc.status);
        output_overlay(c1541_status_panel.video_overlay);
    } else if (c1541.dc.status.head.active()) {
        c1541_status_panel.update(c1541.dc.status);
        output_overlay(c1541_status_panel.video_overlay);
    }

    vid_out.put(s.vic.frame);
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
    init_ram();
    cia1.reset_cold();
    cia2.reset_cold();
    sid.reset();
    vic.reset();
    input_matrix.reset();
    addr_space.reset();
    cpu.reset();
    int_hub.reset();
    c1541.reset();
    exp_ctx.reset();

    s.ba_low = false;
    s.dma_low = false;

    init_sync();
}


std::string get_filename(const u8* ram) {
    const u16 filename_addr = ram[0xbc] * 0x100 + ram[0xbb];
    const u8 filename_len = ram[0xb7];
    return std::string(&ram[filename_addr], &ram[filename_addr + filename_len]);
}


void System::C64::handle_img(Files::Img& img) {
    using Type = Files::Img::Type;

    switch (img.type) {
        case Type::crt: {
            const Files::CRT crt{img.data};
            Log::info("Attaching CRT '%s' ...", img.name.c_str());
            if (Cartridge::attach(crt, exp_ctx)) reset_cold();
            break;
        }
        case Type::d64:
            // auto slot = s.ram[0xb9]; // secondary address
            // slot=0 --> first free slot
            c1541.disk_carousel.insert(0,
                    new C1541::D64_disk(Files::D64{img.data}), img.name);
            break;
        case Type::g64:
            c1541.disk_carousel.insert(0,
                    new C1541::G64_disk(std::move(img.data)), img.name);
            break;
        default:
            Log::info("'%s' ignored", img.name.c_str());
            break;
    }
    // TODO: support 't64' (e.g. cold reset & feed the appropriate 'LOAD'
    // command + 'RETURN' key into the C64 keyboard input buffer)
};


void System::C64::do_load() {
    auto inject = [&](const std::vector<u8>& bin) {
        // load addr (used if 2nd.addr == 0)
        s.ram[0xc3] = cpu.s.x;
        s.ram[0xc4] = cpu.s.y;

        const u8 scnd_addr = s.ram[0xb9];
        u16 addr = (scnd_addr == 0)
            ? cpu.s.y * 0x100 + cpu.s.x
            : bin[1] * 0x100 + bin[0];

        // 'load'
        for (u32 b = 2; b < bin.size(); ++b) s.ram[addr++] = bin[b];

        // end pointer
        s.ram[0xae] = cpu.s.x = addr;
        s.ram[0xaf] = cpu.s.y = addr >> 8;
    };

    auto [img, bin] = loader(get_filename(s.ram));
    if (img || bin) {
        if (bin) inject(*bin);
        if (img) handle_img(*img);
        //'return' status to kernal routine
        cpu.s.clr(NMOS6502::Flag::C); // no error
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
        cpu.s.set(NMOS6502::Flag::C); // error
        return;
    }

    const std::string filepath = as_lower(dir + "/" + filename);

    // TODO: end < start ??
    const u16 start_addr = s.ram[0xc2] * 0x100 + s.ram[0xc1];
    const u16 end_addr = s.ram[0xaf] * 0x100 + s.ram[0xae]; // 1 beyond end
    const u16 sz = end_addr - start_addr;

    if (do_save(filepath, start_addr, &s.ram[start_addr], sz)) {
        // status
        cpu.s.clr(NMOS6502::Flag::C); // no error
        s.ram[0x90] = 0x00; // io status ok
        Log::info("Saved '%s', %d bytes", filepath.c_str(), int(sz + 2));
    } else {
        cpu.s.a = 0x07; // not output file
        cpu.s.set(NMOS6502::Flag::C); // error
        Log::error("Failed to save '%s'", filename.c_str());
    }
}


void System::C64::init_ram() { // TODO: parameterize pattern (+ add 'randomness'?)
    for (int addr = 0x0000; addr <= 0xffff; ++addr)
        s.ram[addr] = (addr & 0x80) ? 0xff : 0x00;
}


// 'halt' instructions used for trapping (i.e. 'illegal' instructions that normally
// would halt the CPU). The byte following the 'halt' is used to identify the 'request'.
void System::C64::install_kernal_tape_traps(u8* kernal, u8 trap_opc) {
    static constexpr u16 kernal_start = 0xe000;

    // trap loading (device 1)
    kernal[0xf539 - kernal_start] = trap_opc;
    kernal[0xf53a - kernal_start] = Trap_ID::load;
    kernal[0xf53b - kernal_start] = NMOS6502::OPC::rts;

    // trap saving (device 1)
    kernal[0xf65f - kernal_start] = trap_opc;
    kernal[0xf660 - kernal_start] = Trap_ID::save;
    kernal[0xf661 - kernal_start] = NMOS6502::OPC::rts;
}
