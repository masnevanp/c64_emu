#include "monitor.h"
#include "mos6502/asm.h"
#include <array>
#include <algorithm>


/*
inline bool is_hex(std::string_view s) { return is_hex(s, std::size(s)); }

inline bool is_hex_byte(std::string_view s) { return is_hex(s, 2); }
inline bool is_hex_word(std::string_view s) { return is_hex(s, 4); }
*/


u32 get_hex_val(const std::string& hex_str, u32 default_val) {
    try {
        return std::stoi(hex_str, nullptr, 16);
    } catch (std::exception& e) {
        return default_val;
    }
}


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
      0  ,  0  ,  0  ,  0  ,  0  , ';' ,  0  ,  0  ,
     ',' ,  0  , ':' , '.' , '-' , 'l' , 'p' ,  0  ,
     'n' , 'o' , 'k' , 'm' , '0' , 'j' , 'i' , '9' ,
     'v' , 'u' , 'h' , 'b' , '8' , 'g' , 'y' , '7' ,
     'x' , 't' , 'f' , 'c' , '6' , 'd' , 'r' , '5' ,
      0  , 'e' , 's' , 'z' , '4' , 'a' , 'w' , '3' ,
      0  ,  0  ,  0  ,  0  ,  0  ,  0  ,  0  ,  0  ,
};

static constexpr u8 keycode_shifted_to_ascii[] = {
      0  , 'q' ,  0  , ' ' , '2' ,  0  ,  0  , '1' ,
      0  ,  0  ,  0  ,  0  ,  0  , ';' ,  0  ,  0  ,
     ',' ,  0  , ':' , '>' , '-' , 'l' , 'p' , '?' ,
     'n' , 'o' , 'k' , 'm' , '0' , 'j' , 'i' , ')' ,
     'v' , 'u' , 'h' , 'b' , '(' , 'g' , 'y' , '7' ,
     'x' , 't' , 'f' , 'c' , '6' , 'd' , 'r' , '5' ,
      0  , 'e' , 's' , 'z' , '$' , 'a' , 'w' , '#' ,
      0  ,  0  ,  0  ,  0  ,  0  ,  0  ,  0  ,  0  ,
};


std::size_t default_output_range_size(char cmd) {
    switch (cmd) {
        case 'd': return 25;
        case 'i': return 25 * 32 - 1;
        case 'm': return 25 * 8 - 1;
        default : return 0;
    }
}


void Monitor::Console::key(u8 code, const Mod_state& mod) {
    auto handle_ret = [&]() {
        auto get_line = [&]() {
            std::array<u8, column_count> line_ascii;

            std::transform(
                std::begin(screen[cursor_y]), std::end(screen[cursor_y]),
                std::begin(line_ascii),
                [](u16 char_rom_idx) -> u8 { return char_code_to_ascii(char_rom_idx); }
            );

            return std::string{std::begin(line_ascii), std::end(line_ascii)};
        };

        auto handle_cmd = [&](const std::vector<std::string>& args) {
            auto get_hex_arg = [&](std::size_t arg_pos, u16 default_val) {
                return arg_pos < args.size()
                    ? u16(get_hex_val(args[arg_pos], default_val)) // TODO: don't truncate to u16? (e.g. target = REU)
                    : default_val;
            };

            auto init_output_cmd = [&](char cmd) {
                addr_cur = get_hex_arg(1, addr_cur);
                addr_end = get_hex_arg(2, addr_cur + default_output_range_size(cmd));
                output_state = os_active;
            };

            if (args.size() > 0 && args[0].length() == 1) active_cmd = args[0][0];
            // else try 'active_cmd' again

            switch (active_cmd) {
                case 'd': case 'i': case 'm': init_output_cmd(active_cmd); break;
                default:
                    active_cmd = no_cmd;
                    output_state = os_idle;
                    break;
            }
        };

        const auto args = split(get_line());
        handle_cmd(args);

        line_feed();
    };

    if (output_state == os_active) {
        if (code == Key_code::r_stp) {
            output_state = os_paused;
        }
        return;
    }

    const auto ascii = mod.shift ? keycode_shifted_to_ascii[code] : keycode_to_ascii[code];

    if (ascii) return type_ascii_chr(ascii);

    switch (code) {
        case Key_code::r_stp:
            if (output_state == os_paused) output_state = os_active;
            break;
        case Key_code::home:
            if (mod.shift) clr_screen();
            cursor_y = cursor_x = 0;
            break;
        case Key_code::crs_d: mod.shift ? cursor_up() : cursor_down();  break;
        case Key_code::crs_r: mod.shift ? cursor_back() : cursor_fwd(); break;
        case Key_code::ret:   handle_ret(); break;
        case Key_code::del:   cursor_back(); type_ascii_chr(' '); cursor_back(); break;
    }
}


void Monitor::Console::draw(PETSCII_Draw& pd) {
    static constexpr int text_top_left_x = (VIC_II::FRAME_WIDTH - (column_count * 8)) / 2;
    static constexpr int text_top_left_y = (VIC_II::FRAME_HEIGHT - (line_count * 8)) / 2;

    auto draw_chr = [&](u16 char_rom_index, int y, int x, Color col_fg = color_fg) {
        pd.chr(
            char_rom_index,
            text_top_left_x + (x * 8), text_top_left_y + (y * 8),
            col_fg, color_bg
        );
    };

    auto draw_cursor = [&]() {
        const u16 chr_at_crsr = screen[cursor_y][cursor_x];
        const u16 rvrs_chr_at_crsr = chr_at_crsr ^ 0x80;
        draw_chr(rvrs_chr_at_crsr, cursor_y, cursor_x, color_cursor);
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

    if (output_state != os_active) draw_cursor();
}


void Monitor::Console::scroll_up() {
    std::fill(screen[0].begin(), screen[0].end(), ascii_to_char_code(' '));
    std::rotate(screen.begin(), screen.begin() + 1, screen.end());
}


void Monitor::Console::scroll_down() {
    std::fill(screen[line_count - 1].begin(), screen[line_count - 1].end(), ascii_to_char_code(' '));
    std::rotate(screen.rbegin(), screen.rbegin() + 1, screen.rend());
}


void Monitor::Console::clr_screen() {
    for (auto& line : screen) for (auto& c : line) c = ascii_to_char_code(' ');
}


void Monitor::Console::tick() {
    auto print_d = [&](u16 addr) {
        const auto& r{s.ram};

        char buffer[column_count];
        const char* format = "> %04s  %-08s  %s";

        const u8 opc = r[addr];
        const u8 byte_2 = r[u16(addr + 1)]; // byte_2 and/or byte_3 might not be needed
        const u8 byte_3 = r[u16(addr + 2)]; // (e.g. if instr.size is 1)

        const auto line = MOS6502::Asm::disasm_one(opc, byte_2, byte_3, addr);

        sprintf(buffer, format,
            as_lower(line.pc).c_str(),
            as_lower(line.bytes).c_str(),
            as_lower(line.text).c_str()
        );
        type_txt(buffer);

        return MOS6502::Asm::instruction[opc].size;
    };

    auto print_i = [&](u16 addr) {
        char buffer[column_count];
        const char* format = "; %04x  ";
        const auto& r{s.ram};

        sprintf(buffer, format, addr);
        type_txt(buffer);

        for (int i = 0; i < 32; ++i) {
            type_petscii_chr(r[u16(addr + i)]);
        }
    };

    auto print_m = [&](u16 addr) {
        char buffer[column_count];
        const char* format = ": %04x  %02x %02x %02x %02x %02x %02x %02x %02x  ";
        const auto& r{s.ram};

        sprintf(buffer, format,
            addr,
            r[u16(addr + 0)], r[u16(addr + 1)], r[u16(addr + 2)], r[u16(addr + 3)], 
            r[u16(addr + 4)], r[u16(addr + 5)], r[u16(addr + 6)], r[u16(addr + 7)]
        );
        type_txt(buffer);

        for (int i = 0; i < 8; ++i) {
            type_petscii_chr(r[u16(addr + i)]);
        }
    };

    auto inc_addr_and_check_end = [&](u16 inc_size) {
        const u16 old_addr_cur = addr_cur;
        addr_cur += inc_size;
        if (is_in_wrapped_range(addr_end, old_addr_cur, addr_cur)) {
            output_state = os_idle;
        }
    };

    auto tick_cmd_d = [&]() {
        const auto instr_size = print_d(addr_cur);
        line_feed();
        inc_addr_and_check_end(instr_size);
    };

    auto tick_cmd_i = [&]() {
        print_i(addr_cur);
        line_feed();
        inc_addr_and_check_end(32);
    };

    auto tick_cmd_m = [&]() {
        print_m(addr_cur);
        line_feed();
        inc_addr_and_check_end(8);
    };

    if (output_state != os_active) return;

    switch (active_cmd) {
        case 'd': tick_cmd_d(); return;
        case 'i': tick_cmd_i(); return;
        case 'm': tick_cmd_m(); return;
        default: return;
    }
}


void Monitor::CPU::draw(PETSCII_Draw& pd) { pd.txt("CPU View [TODO]", 100, 100, color_fg, color_bg); }
void Monitor::VIC::draw(PETSCII_Draw& pd) { pd.txt("VIC View [TODO]", 100, 100, color_fg, color_bg); }
void Monitor::CIA::draw(PETSCII_Draw& pd) { pd.txt("CIA View [TODO]", 100, 100, color_fg, color_bg); }
