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


namespace bytes_per_line {
    static constexpr std::size_t i{32};
    static constexpr std::size_t m{8};
}


std::size_t default_output_range_size(char cmd) {
    switch (cmd) {
        case 'd': return 25;
        case 'i': return 25 * bytes_per_line::i - 1;
        case 'm': return 25 * bytes_per_line::m - 1;
        default : return 0;
    }
}


void Monitor::Console::key(u8 code, const Mod_state& mod) {
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


void Monitor::Console::clr_screen() {
    for (auto& line : screen) for (auto& c : line) c = ascii_to_char_code(' ');
}


void Monitor::Console::scroll_up() {
    std::fill(screen[0].begin(), screen[0].end(), ascii_to_char_code(' '));
    std::rotate(screen.begin(), screen.begin() + 1, screen.end());
}


void Monitor::Console::scroll_down() {
    std::fill(screen[line_count - 1].begin(), screen[line_count - 1].end(), ascii_to_char_code(' '));
    std::rotate(screen.rbegin(), screen.rbegin() + 1, screen.rend());
}


void Monitor::Console::handle_ret() {
    auto get_line = [&]() {
        std::array<u8, column_count> line_ascii;

        std::transform(
            std::begin(screen[cursor_y]), std::end(screen[cursor_y]),
            std::begin(line_ascii),
            [](u16 char_rom_idx) -> u8 { return char_code_to_ascii(char_rom_idx); }
        );

        return std::string{std::begin(line_ascii), std::end(line_ascii)};
    };

    const auto cur_line = get_line();
    const auto args = split(cur_line);
    handle_cmd(args);

    line_feed();
}


void Monitor::Console::handle_cmd(const std::vector<std::string>& args) {
    auto get_hex_arg = [&](std::size_t arg_pos, u16 default_val) {
        return arg_pos < args.size()
            ? u16(get_hex_val(args[arg_pos], default_val))
            : default_val;
    };

    auto init_range_output = [&](char cmd) {
        const auto start = get_hex_arg(1, ar.next());
        const auto end = get_hex_arg(2, start + default_output_range_size(cmd));
        ar.init(start, end);
        output_state = os_active;
    };

    auto handle_s = [&]() {
        const auto new_target_id = get_hex_arg(1, as.target().id);
        as.select(new_target_id);

        char buffer[column_count];
        const char* format = " %c%02x: %04x-%04x  %s";

        for (auto t : as.targets) {
            line_feed();
            const auto selected = t.id == as.target().id ? '*' : ' ';
            sprintf(buffer, format, selected, t.id, t.addr_start, t.addr_end, t.name);
            type_txt(buffer);
        }
    };

    if (args.size() > 0) {
        if (args[0].length() == 1) active_cmd = args[0][0];
        else if (args[0].length() > 1) return;
    }
    // else we will just try 'active_cmd' again...

    switch (active_cmd) {
        case 'd': case 'i': case 'm': init_range_output(active_cmd); break;
        case 's': handle_s(); break;
        default:
            active_cmd = no_cmd;
            output_state = os_idle;
            break;
    }
}


void Monitor::Console::print_d() {
    auto do_print = [&](const MOS6502::Asm::Line& line) {
        char buffer[column_count];
        const char* format = "> %04s  %-08s  %s";

        sprintf(buffer, format,
            as_lower(line.pc).c_str(),
            as_lower(line.bytes).c_str(),
            as_lower(line.text).c_str()
        );
        type_txt(buffer);
    };

    const u16 instr_addr = ar.next();

    switch (const u8 opc = as.read(instr_addr); MOS6502::Asm::instr(opc).size) {
        case 1: do_print(MOS6502::Asm::disasm_one(instr_addr, opc)); break;
        case 2: do_print(MOS6502::Asm::disasm_one(instr_addr, opc, as.read(ar.next()))); break;
        case 3: do_print(MOS6502::Asm::disasm_one(instr_addr, opc, as.read(ar.next()), as.read(ar.next()))); break;
    }
}


void Monitor::Console::print_i() {
    char buffer[column_count];
    const char* format = "; %04x  ";

    sprintf(buffer, format, ar.peek());
    type_txt(buffer);

    for (int i = 0; i < 32; ++i) {
        type_petscii_chr(as.read(ar.next()));
    }
}


void Monitor::Console::print_m() {
    char buffer[column_count];
    const char* format = ": %04x  %02x %02x %02x %02x %02x %02x %02x %02x  ";

    const u16 start_addr = ar.peek();

    sprintf(buffer, format,
        start_addr,
        as.read(ar.next()), as.read(ar.next()), as.read(ar.next()), as.read(ar.next()),
        as.read(ar.next()), as.read(ar.next()), as.read(ar.next()), as.read(ar.next())
    );
    type_txt(buffer);

    const u16 end_addr = ar.peek(); // one beyond, actually

    for (u16 a = start_addr; a < end_addr; ++a) {
        type_petscii_chr(as.read(a));
    }
};


void Monitor::Console::tick() {
    if (output_state != os_active) return;

    if (ar.at_end()) {
        output_state = os_idle;
        return;
    }

    switch (active_cmd) {
        case 'd': {
            print_d();
            break;
        }
        case 'i':
            print_i();
            break;
        case 'm':
            print_m();
            break;
        default:
            return;
    }

    line_feed();
}


void Monitor::CPU::draw(PETSCII_Draw& pd) { pd.txt("CPU View [TODO]", 100, 100, color_fg, color_bg); }
void Monitor::VIC::draw(PETSCII_Draw& pd) { pd.txt("VIC View [TODO]", 100, 100, color_fg, color_bg); }
void Monitor::CIA::draw(PETSCII_Draw& pd) { pd.txt("CIA View [TODO]", 100, 100, color_fg, color_bg); }
