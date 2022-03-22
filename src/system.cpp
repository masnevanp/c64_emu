
#include "system.h"
#include <fstream>
#include <algorithm>



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

const u8 System::Address_space::Mode_to_PLA_idx[32] = {
    0,  0,  1,  2,  0, 11,  3,  4, 0,  8,  9,  5,  0, 11, 12,  6,
    7,  7,  7,  7,  7,  7,  7,  7, 0,  8,  9, 10,  0, 11, 12, 13,
};


void System::C1541_status_panel::draw() {
    auto draw_char = [&](u16 chr, u16 chr_x, u16 chr_y, Color fg, Color bg) {
        const u8* src = &charrom[chr * 8];
        for (int px_row = 0; px_row < 8; ++px_row, ++src) {
            u8* tgt = &overlay.pixels[(px_row + chr_y) * overlay.w + chr_x];
            for (u8 px = 0b10000000; px; px >>= 1) *tgt++ = (*src & px) ? fg : bg;
        }
    };

    static constexpr u16 pad_left = 3;
    static constexpr u16 pad_top  = 1;

    static constexpr u16 ch_zero   = 0x0030;
    static constexpr u16 ch_led_wp = 0x0051;
    static constexpr u16 ch_led    = 0x0057;

    const auto x = pad_left;
    const auto y = pad_top;
    const auto led_ch = status.wp_off() ? ch_led : ch_led_wp;
    const auto led_col = status.led_on() ? Color::light_green : Color::gray_1;
    draw_char(led_ch, x, y, led_col, col_bg);

    const auto track_n = (status.head.track_n / 2) + 1;
    const auto tn_col = status.head.active() ? Color::green : Color::gray_2;
    draw_char(ch_zero + track_n / 10, x + 8 + 2,  y, tn_col, col_bg);
    draw_char(ch_zero + track_n % 10, x + 16 + 2, y, tn_col, col_bg);
}


void System::VIC_out::frame(u8* frame) {
    auto output_frame = [&]() {
        auto output_overlay = [&](Overlay& ol) {
            const u8* src = ol.pixels;
            for (int y = 0; y < ol.h; ++y) {
                u8* tgt = &frame[((ol.y + y) * VIC_II::FRAME_WIDTH) + ol.x];
                for (int x = 0; x < ol.w; ++x, ++src) {
                    const auto col = *(src);
                    /*if (col != Overlay::transparent)*/ tgt[x] = col;
                }
            }
        };

        if (menu_overlay.visible) {
            output_overlay(menu_overlay);
            output_overlay(c1541_status.overlay);
            c1541_status.overlay.visible = true; // forces to update
        } else if (c1541_status.overlay.visible) {
            output_overlay(c1541_status.overlay);
        }

        vid_out.put(frame);
    };

    host_input.poll();

    c1541_status.update();

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
}


System::Menu::Menu(std::initializer_list<std::pair<std::string, std::function<void ()>>> imm_actions_,
    std::initializer_list<std::pair<std::string, std::function<void ()>>> actions_,
    std::initializer_list<::Menu::Group> subs_,
    const u8* charrom)
  : subs(subs_), charset(&charrom[256 * 8]) // using the 'lower case' half
{
    auto Imm_action = [&](const std::pair<std::string, std::function<void ()>>& a) {
        return ::Menu::Kludge(a.first, [act = a.second, &vis = overlay.visible](){ act(); vis = false; return nullptr; });
    };
    auto Action = [&](const std::pair<std::string, std::function<void ()>>& a) {
        return ::Menu::Action(a.first, [act = a.second, &vis = overlay.visible](){ act(); vis = false; return nullptr; });
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

    if (overlay.visible) {
        switch (code) {
            case kc::menu_ent:  ctrl.key_enter(); break;
            case kc::menu_up:   ctrl.key_up();    break;
            case kc::menu_down: ctrl.key_down();  break;
        }
    } else {
        overlay.visible = true;
    }

    update();
}


void System::Menu::toggle_visibility()  {
    if (!overlay.visible) ctrl.select(&main_menu);
    overlay.visible = !overlay.visible;
    update();
}


void System::Menu::update() {
    overlay.clear(col[0]);

    const std::string text = ctrl.state();
    auto text_x = 4; // (overlay.w - (text.length() * 8)) / 2; // centered

    for (u8 ci = 0; ci < text.length(); ++ci) {
        for (int px_row = 0; px_row < 8; ++px_row) {
            auto c = text[ci] % 64; // map ascii code to character data (subset)
            const u8* pixels = &charset[(8 * c) + px_row];
            u8* tgt = &overlay.pixels[text_x + ci * 8 + 2 * px_row * overlay.w];
            for (int px = 7; px >= 0; --px, ++tgt) {
                *tgt = *(tgt + overlay.w) = col[(*pixels >> px) & 0x1];
            }
        }
    }
}


/*void System::C64::keep_running() {
    auto run_without_c1541 = [&]() {
        while (!run_cfg_change) {
            vic.tick();

            if (!s.rdy_low || cpu.mrw() == NMOS6502::MC::RW::w) {
                addr_space.access(cpu.mar(), cpu.mdr(), cpu.mrw());
                cpu.tick();
            }

            cia1.tick(s.vic.cycle);
            cia2.tick(s.vic.cycle);
            int_hub.tick();
        }
    };

    auto run_with_c1541 = [&]() {
        while(!run_cfg_change) {
            vic.tick();

            const auto rw = cpu.mrw();
            if (!s.rdy_low || rw == NMOS6502::MC::RW::w) {
                // 'Slip in' the C1541 cycle
                if (rw == NMOS6502::MC::RW::w) c1541.tick();
                addr_space.access(cpu.mar(), cpu.mdr(), rw);
                cpu.tick();
                if (rw == NMOS6502::MC::RW::r) c1541.tick();
            } else {
                c1541.tick();
            }

            cia1.tick(s.vic.cycle);
            cia2.tick(s.vic.cycle);
            int_hub.tick();
            if (s.vic.cycle % C1541::extra_cycle_freq == 0) c1541.tick();
        }
    };

    run_cfg_change = false;

    if (c1541.idle) run_without_c1541();
    else run_with_c1541();
}*/


std::string get_filename(u8* ram) {
    u16 filename_addr = ram[0xbc] * 0x100 + ram[0xbb];
    u8 filename_len = ram[0xb7];
    return std::string(&ram[filename_addr], &ram[filename_addr + filename_len]);
}


void System::C64::do_load() {
    const u8 scnd_addr = s.ram[0xb9];

    auto inject = [&](const std::vector<u8>& bin) {
        auto sz = bin.size();

        // load addr (used if 2nd.addr == 0)
        s.ram[0xc3] = cpu.x;
        s.ram[0xc4] = cpu.y;

        u16 addr = (scnd_addr == 0)
            ? cpu.y * 0x100 + cpu.x
            : bin[1] * 0x100 + bin[0];

        // 'load'
        for (u32 b = 2; b < sz; ++b) s.ram[addr++] = bin[b];

        // end pointer
        s.ram[0xae] = cpu.x = addr;
        s.ram[0xaf] = cpu.y = addr >> 8;
    };

    auto resolve_disk_img_op = [&]() {
        using Iop = Files::Img_op;

        switch (scnd_addr) {
            case 0: return Iop(Iop::fwd | Iop::mount | Iop::inspect);
            case 1: return Iop::fwd; // would be 'fwd_1st' (first drive on the bus)
            case 8: return Iop::fwd; // would be 'fwd_8' if multiple drives were ever supported
            // case 9: return Iop::fwd_9;
            // ...
            case 2: return Iop::mount;
            case 3: return Iop::inspect;
            default: return Iop::none;
        }
    };

    auto bin = loader(get_filename(s.ram), resolve_disk_img_op());

    if (!bin || (*bin).size() == 1 || (*bin).size() == 2) {
        cpu.pc = 0xf704; // jmp to 'file not found'
    } else {
        // NOTE: Loading a file of size 0 'succeeds' (you get the 'READY.', with no errors)
        // It happens when a disk gets inserted to the disk drive, but the image is
        // not mounted (by the loader) --> no directory listing generated.
        // TODO: maybe return a 'variant' instead?
        if ((*bin).size() > 2) inject(*bin);

        // 'return' status to kernal routine
        cpu.clr(NMOS6502::Flag::C); // no error
        s.ram[0x90] = 0x00; // io status ok
    }
}


// TODO: identify the addresses used
void System::C64::do_save() {
    static const std::string dir = "data/prg/"; // TODO...

    auto do_save = [](const std::string& filename, u16 start_addr, u8* data, size_t sz) -> bool {
        // TODO: check stream state & hadle exceptions
        std::ofstream f(filename, std::ios::binary);
        f << (u8)(start_addr) << (u8)(start_addr >> 8);
        f.write((char*)data, sz);
        std::cout << "\nSaved '" << filename << "', " << (sz + 2) << " bytes ";
        return true;
    };

    std::string filename = get_filename(s.ram);
    if (filename.length() == 0) {
        // TODO: error_code enum(s)
        cpu.a = 0x08; // missing filename
        cpu.set(NMOS6502::Flag::C); // error
        return;
    }

    std::string filepath = as_lower(dir + filename);

    // TODO: end < start ??
    u16 start_addr = s.ram[0xc2] * 0x100 + s.ram[0xc1];
    u16 end_addr = s.ram[0xaf] * 0x100 + s.ram[0xae]; // 1 beyond end
    u16 sz = end_addr - start_addr;

    do_save(filepath, start_addr, &s.ram[start_addr], sz);

    // status
    cpu.clr(NMOS6502::Flag::C); // no error
    s.ram[0x90] = 0x00; // io status ok
}


void System::C64::init_ram() { // TODO: parameterize pattern (+ add 'randomness'?)
    for (int addr = 0x0000; addr <= 0xffff; ++addr)
        s.ram[addr] = (addr & 0x80) ? 0xff : 0x00;
}


// 'halt' instructions used for trapping (i.e. 'illegal' instructions that normally
// would halt the CPU). The byte following the 'halt' is used to identify the 'request'.
void System::C64::install_tape_kernal_traps(u8* kernal, u8 trap_opc) {
    static constexpr u16 kernal_start = 0xe000;
    static constexpr u8  rts_opc = 0x60;

    // trap loading (device 1)
    kernal[0xf539 - kernal_start] = trap_opc;
    kernal[0xf53a - kernal_start] = Trap_ID::load;
    kernal[0xf53b - kernal_start] = rts_opc;

    // trap saving (device 1)
    kernal[0xf65f - kernal_start] = trap_opc;
    kernal[0xf660 - kernal_start] = Trap_ID::save;
    kernal[0xf661 - kernal_start] = rts_opc;
}
