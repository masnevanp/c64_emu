#include "monitor.h"
#include <algorithm>


void Monitor::key(u8 code, bool down) {
    auto is_mod = [&](u8 code) {
        return code == Key_code::sh_l || code == Key_code::sh_r
                    || code == Key_code::ctrl || code == Key_code::cmdre;
    };

    if (is_mod(code)) {
        switch (code) {
            case Key_code::sh_l:
            case Key_code::sh_r:  mod.shift = down; return;
            case Key_code::ctrl:  mod.ctrl  = down; return;
            case Key_code::cmdre: mod.cmdre = down; return;
        }
    }

    if (down) {
        switch (code) {
            case Key_code::f1: active_view = mod.shift ? 1 : 0; break;
            case Key_code::f3: active_view = mod.shift ? 3 : 2; break;
            default: views[active_view]->key(code, mod);  break;
        }
    }
}


u8* Monitor::draw(const u8* charrom) {
    for (auto& px : frame) px = color_bg;

    PETSCII_Draw pd{charrom, frame, frame_width};
    views[active_view]->draw(pd);

    return frame;
};


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

static constexpr u8 keycode_to_ascii[] = {
      0  , 'q' ,  0  , ' ' , '2' ,  0  ,  0  , '1' ,
      0  ,  0  ,  0  ,  0  ,  0  ,  0  ,  0  ,  0  ,
     ',' ,  0  , ':' , '.' , '-' , 'l' , 'p' ,  0  ,
     'n' , 'o' , 'k' , 'm' , '0' , 'j' , 'i' , '9' ,
     'v' , 'u' , 'h' , 'b' , '8' , 'g' , 'y' , '7' ,
     'x' , 't' , 'f' , 'c' , '6' , 'd' , 'r' , '5' ,
      0  , 'e' , 's' , 'z' , '4' , 'a' , 'w' , '3' ,
      0  ,  0  ,  0  ,  0  ,  0  ,  0  ,  0  ,  0  ,
};

static constexpr u8 keycode_shifted_to_ascii[] = {
      0  , 'q' ,  0  , ' ' , '2' ,  0  ,  0  , '1' ,
      0  ,  0  ,  0  ,  0  ,  0  ,  0  ,  0  ,  0  ,
     ',' ,  0  , ':' , '.' , '-' , 'l' , 'p' , '?' ,
     'n' , 'o' , 'k' , 'm' , '0' , 'j' , 'i' , ')' ,
     'v' , 'u' , 'h' , 'b' , '(' , 'g' , 'y' , '7' ,
     'x' , 't' , 'f' , 'c' , '6' , 'd' , 'r' , '5' ,
      0  , 'e' , 's' , 'z' , '$' , 'a' , 'w' , '#' ,
      0  ,  0  ,  0  ,  0  ,  0  ,  0  ,  0  ,  0  ,
};


void Monitor::Console::key(u8 code, const Mod_state& mod) {
    auto do_ret = [&]() {
        const std::string line_text{std::begin(screen[cursor_y]), std::end(screen[cursor_y])};

        for (auto token : split(line_text)) Log::info("%s", token.c_str());;

        line_feed();

        mode = Mode::cmd_m;
        addr_cur = 0xa000;
        addr_end = 0xc000;
    };

    if (mode != Mode::idle) {
        if (code == Key_code::r_stp) mode = Mode::idle;
        return;
    }

    const auto ascii = mod.shift ? keycode_shifted_to_ascii[code] : keycode_to_ascii[code];

    if (ascii) return type_ascii_chr(ascii);

    switch (code) {
        case Key_code::home: if (mod.shift) clr_screen(); cursor_y = cursor_x = 0; return;
        case Key_code::crs_d: return mod.shift ? cursor_up() : cursor_down();
        case Key_code::crs_r: return mod.shift ? cursor_back() : cursor_fwd();
        case Key_code::ret: do_ret(); return;
        case Key_code::del: cursor_back(); type_ascii_chr(' '); cursor_back(); return;
    }
}


void Monitor::Console::draw(PETSCII_Draw& pd) {
    static constexpr int text_top_left_x = (VIC_II::FRAME_WIDTH - (column_count * 8)) / 2;
    static constexpr int text_top_left_y = (VIC_II::FRAME_HEIGHT - (line_count * 8)) / 2;

    auto draw_chr = [&](u16 chr, int y, int x) {
        pd.chr(
            chr,
            text_top_left_x + (x * 8), text_top_left_y + (y * 8),
            color_fg, color_bg
        );
    };

    auto draw_cursor = [&]() {
        const auto chr_at_crsr = screen[cursor_y][cursor_x];
        const auto rvrs_chr_at_crsr = chr_at_crsr | 0x080;
        draw_chr(rvrs_chr_at_crsr, cursor_y, cursor_x);
    };

    auto draw_screen = [&]() {
        for (int y = 0; y < line_count; ++y) {
            for (int x = 0; x < column_count; ++x) {
                draw_chr(screen[y][x], y, x);
            }
        }
    };

    tick();

    draw_screen();

    if (mode == Mode::idle) draw_cursor();
}


void Monitor::Console::scroll_up() {
    std::fill(screen[0].begin(), screen[0].end(), ascii_to_char_rom(' '));
    std::rotate(screen.begin(), screen.begin() + 1, screen.end());
}


void Monitor::Console::scroll_down() {
    std::fill(screen[line_count - 1].begin(), screen[line_count - 1].end(), ascii_to_char_rom(' '));
    std::rotate(screen.rbegin(), screen.rbegin() + 1, screen.rend());
}


void Monitor::Console::clr_screen() {
    for (auto& line : screen) for (auto& c : line) c = ascii_to_char_rom(' ');
}


void Monitor::Console::tick() {
    auto tick_cmd_d = [&]() {
        if (addr_cur > addr_end) {
            mode = Mode::idle;
            return;
        }

        char buffer[column_count];
        const char* format = "> %04x";
        sprintf(buffer, format, addr_cur);
        print(buffer);

        addr_cur += 1;
    };

    auto tick_cmd_m = [&]() {
        if (addr_cur > addr_end) { // TODO: proper handling (e.g. handle wrap around..)
            mode = Mode::idle;
            return;
        }

        char buffer[column_count];
        const char* format = ":%04x  %02x %02x %02x %02x %02x %02x %02x %02x  ";
        const auto& r{s.ram};

        sprintf(buffer, format,
            addr_cur,
            r[addr_cur + 0], r[addr_cur + 1], r[addr_cur + 2], r[addr_cur + 3], 
            r[addr_cur + 4], r[addr_cur + 5], r[addr_cur + 6], r[addr_cur + 7]
        );
        type_ascii_txt(buffer);

        for (int i = 0; i < 8; ++i)
            type_petscii_chr(r[addr_cur + i]);

        line_feed();

        addr_cur += 8;
    };

    switch (mode) {
        case Mode::cmd_d: tick_cmd_d(); return;
        case Mode::cmd_m: tick_cmd_m(); return;
        default: return;
    }
}


void Monitor::CPU::draw(PETSCII_Draw& pd) { pd.txt("CPU View [TODO]", 100, 100, color_fg, color_bg); }
void Monitor::VIC::draw(PETSCII_Draw& pd) { pd.txt("VIC View [TODO]", 100, 100, color_fg, color_bg); }
void Monitor::CIA::draw(PETSCII_Draw& pd) { pd.txt("CIA View [TODO]", 100, 100, color_fg, color_bg); }
