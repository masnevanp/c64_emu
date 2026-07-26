#include "monitor.h"


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
            default: views[active_view]->key(code, down, mod);  break;
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


void Monitor::Console::key(u8 code, bool down, const Mod_state& mod) {
    auto scroll = [&]() {
        for (int y = 1; y < text_height; ++y) for (int x = 0; x < text_width; ++x) {
            text[y - 1][x] = text[y][x];
        }
        for (int x = 0; x < text_width; ++x) text[text_height - 1][x] = ' ';
    };

    auto crsr_up   = [&]() { if (crsr_y > 0) --crsr_y; };
    auto crsr_down = [&]() { if (++crsr_y == text_height) { --crsr_y; scroll(); } };
    auto crsr_fwd  = [&]() { if (++crsr_x == text_width) { crsr_x = 0; crsr_down(); } };
    auto crsr_back = [&]() { if (crsr_x == 0) { if (crsr_y > 0) { crsr_x = (text_width - 1); crsr_up(); } } else --crsr_x; };

    auto type_chr = [&](u8 c) { text[crsr_y][crsr_x] = c; crsr_fwd(); };

    auto do_ret = [&]() {
        const std::string line_text{std::begin(text[crsr_y]), std::end(text[crsr_y])};

        for (auto token : split(line_text)) Log::info("%s", token.c_str());;

        crsr_x = 0; crsr_down();
    };

    if (down) {
        const auto ascii = mod.shift ? keycode_shifted_to_ascii[code] : keycode_to_ascii[code];

        if (ascii) return type_chr(ascii);

        switch (code) {
            case Key_code::home: if (mod.shift) clr_text(); crsr_y = crsr_x = 0; return;
            case Key_code::crs_d: return mod.shift ? crsr_up() : crsr_down();
            case Key_code::crs_r: return mod.shift ? crsr_back() : crsr_fwd();
            case Key_code::ret: do_ret(); return;
            case Key_code::del: crsr_back(); type_chr(' '); crsr_back(); return;
        }
    }

}


void Monitor::Console::draw(PETSCII_Draw& pd) {
    static constexpr int text_top_left_x = 4;
    static constexpr int text_top_left_y = 4;

    auto draw_chr = [&](u16 chr, int y, int x) {
        pd.chr(
            chr,
            text_top_left_x + (x * 8), text_top_left_y + (y * 8),
            color_fg, color_bg
        );
    };

    auto draw_cursor = [&]() {
        const auto chr_at_crsr = ascii_to_char_rom(text[crsr_y][crsr_x]);
        const auto rvrs_chr_at_crsr = chr_at_crsr | 0x080;
        draw_chr(rvrs_chr_at_crsr, crsr_y, crsr_x);
    };

    for (int y = 0; y < text_height; ++y) {
        for (int x = 0; x < text_width; ++x) {
            const auto chr = ascii_to_char_rom(text[y][x]);
            draw_chr(chr, y, x);
        }
    }

    draw_cursor();
}


void Monitor::Console::clr_text() {
    for (int y = 0; y < text_height; ++y)
        for (int x = 0; x < text_width; ++x)
            text[y][x] = ' ';
}


void Monitor::CPU::draw(PETSCII_Draw& pd) { pd.txt("CPU View [TODO]", 100, 100, color_fg, color_bg); }
void Monitor::VIC::draw(PETSCII_Draw& pd) { pd.txt("VIC View [TODO]", 100, 100, color_fg, color_bg); }
void Monitor::CIA::draw(PETSCII_Draw& pd) { pd.txt("CIA View [TODO]", 100, 100, color_fg, color_bg); }
