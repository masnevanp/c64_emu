#include "monitor.h"
#include "mos6502/asm.h"
#include <array>
#include <algorithm>


/*
inline bool is_hex(std::string_view s) { return is_hex(s, std::size(s)); }

inline bool is_hex_byte(std::string_view s) { return is_hex(s, 2); }
inline bool is_hex_word(std::string_view s) { return is_hex(s, 4); }
*/

static const char* cmd_help[] = {
    " d [adr1] [adr2]  : disassemble",
    " i [adr1] [adr2]  : dump petscii",
    " m [adr1] [adr2]  : dump hex & petscii",
    " s [n]            : select target",
    " ?                : help"
};


enum Cmd_char : char {
    help             = '?',
    disasm           = 'd',
    dump_petscii     = 'i',
    dump_hex_petscii = 'm',
    select_tgt       = 's',
    //unknown          = char(0),
};


enum Prefix : char {
    disasm_prefix           = '>',
    dump_petscii_prefix     = ';',
    dump_hex_petscii_prefix = ':',
};


u32 get_hex_val(const std::string& hex_str, u32 default_val) {
    if (is_hex(hex_str, hex_str.length())) {
        try {
            return std::stoul(hex_str, nullptr, 16);
        } catch (std::exception& e) {}
    }

    return default_val;
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


u8 Monitor::Console::Address_space::read(u16 address) {
    if (!is_valid(address)) return 0xff; // TODO: anything else..?

    // translate address
    address = address - target().addr_start;

    switch (target_id) {
        case Target::ID::sys_map_p:  return sys_map.peek(address);
        case Target::ID::sys_map_rw: return sys_map.read(address);
        case Target::ID::ram:        return s.ram[address];
        case Target::ID::basic:      return rom.basic[address];
        case Target::ID::kernal:     return rom.kernal[address];
        default: return 0xff;
    }
}


void Monitor::Console::Address_space::write(u16 address, u8 data) {
    if (!is_valid(address)) return;

    // translate address
    address = address - target().addr_start;

    switch (target_id) {
        case Target::ID::sys_map_rw: sys_map.write(address, data); break;
        case Target::ID::ram:        s.ram[address] = data;        break;
        default: break;
    }
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


std::vector<std::string> Monitor::Console::tokenize_current_line() const {
    std::array<u8, column_count> line_ascii;

    std::transform(
        std::begin(screen[cursor_y]), std::end(screen[cursor_y]),
        std::begin(line_ascii),
        [](u16 char_rom_idx) -> u8 { return char_code_to_ascii(char_rom_idx); }
    );

    const std::string cur_line{std::begin(line_ascii), std::end(line_ascii)};

    return split(cur_line);
};


void Monitor::Console::handle_ret() {
    const auto args = tokenize_current_line();
    handle_cmd(args);

    line_feed();
}


namespace Output_bytes_per_line {
    static constexpr std::size_t dump_petscii{32};
    static constexpr std::size_t dump_hex_petscii{8};
}

std::size_t default_output_range_size(char cmd) {
    using cc = Cmd_char;
    switch (cmd) {
        case cc::disasm:           return 25;
        case cc::dump_petscii:     return 25 * Output_bytes_per_line::dump_petscii - 1;
        case cc::dump_hex_petscii: return 25 * Output_bytes_per_line::dump_hex_petscii - 1;
        default : return 0;
    }
}


void Monitor::Console::handle_cmd(const std::vector<std::string>& args) {
    auto get_cmd = [&](char default_cmd) {
        if (args.size() > 0) {
            if (args[0].length() == 1) return args[0][0];
            else if (args[0].length() > 1) return no_cmd;
        }
        return default_cmd;
    };

    auto get_hex_arg = [&](std::size_t arg_pos, u16 default_val) {
        return arg_pos < args.size()
            ? u16(get_hex_val(args[arg_pos], default_val))
            : default_val;
    };

    auto init_range_output = [&](char cmd) {
        addr_cur = get_hex_arg(1, addr_cur);
        addr_end = get_hex_arg(2, addr_cur + default_output_range_size(cmd));
        output_state = os_active;
    };

    auto print_help = [&]() {
        for (const auto h : cmd_help) {
            line_feed();
            type_txt(h);
        }
    };

    auto handle_select_tgt = [&]() {
        const auto new_target_id = get_hex_arg(1, as.target().id);
        as.select(new_target_id);

        if (!as.is_valid(addr_cur)) {
            addr_cur = as.target().addr_start;
        }

        char buffer[column_count];
        const char* format = " %c%02x: %04x-%04x  %s";

        for (auto t : as.targets) {
            line_feed();
            const auto selected = t.id == as.target().id ? '*' : ' ';
            sprintf(buffer, format, selected, t.id, t.addr_start, t.addr_end, t.name);
            type_txt(buffer);
        }
    };

    active_cmd = get_cmd(active_cmd);

    using cc = Cmd_char;
    switch (active_cmd) {
        case cc::help: print_help(); break;
        case cc::disasm: case cc::dump_petscii: case cc::dump_hex_petscii:
            init_range_output(active_cmd);
            break;
        case cc::select_tgt: handle_select_tgt(); break;
        default:
            active_cmd = no_cmd;
            output_state = os_idle;
            break;
    }
}


void Monitor::Console::print_disasm() {
    auto do_print = [&](const MOS6502::Asm::Line& line) {
        char buffer[column_count];
        const char* format = "%c %04s  %-08s  %s";

        sprintf(buffer, format,
            Prefix::disasm_prefix,
            as_lower(line.pc).c_str(),
            as_lower(line.bytes).c_str(),
            as_lower(line.text).c_str()
        );
        type_txt(buffer);
    };

    const u8 opc = as.read(addr_cur);
    const u8 instr_size = MOS6502::Asm::instr(opc).size;

    // TODO: mark undocumented ops (e.g. "lax ($22),y  :ud")
    //       (or use color --> add support for per char. colors...)
    switch (instr_size) {
        case 1:
            do_print(MOS6502::Asm::disasm_one(addr_cur, opc));
            break;
        case 2:
            do_print(MOS6502::Asm::disasm_one(addr_cur, opc, as.read(addr_cur + 1)));
            break;
        case 3:
            do_print(MOS6502::Asm::disasm_one(addr_cur, opc, as.read(addr_cur + 1), as.read(addr_cur + 2)));
    }

    addr_cur += instr_size;
}


void Monitor::Console::print_dump_petscii() {
    char buffer[column_count];
    const char* format = "%c %04x  ";

    sprintf(buffer, format, Prefix::dump_petscii_prefix, addr_cur);
    type_txt(buffer);

    for (std::size_t n = 0; n < Output_bytes_per_line::dump_petscii; ++n) {
        type_petscii_chr(as.read(addr_cur++));
    }
}


void Monitor::Console::print_dump_hex_petscii() {
    char buffer[column_count];
    const char* format = "%c %04x  %02x %02x %02x %02x %02x %02x %02x %02x  ";

    const u16 line_addr = addr_cur;

    std::array<u8, Output_bytes_per_line::dump_hex_petscii> bytes;

    std::generate(begin(bytes), end(bytes), [this]() { return as.read(addr_cur++); });

    sprintf(buffer, format,
        Prefix::dump_hex_petscii_prefix, line_addr,
        bytes[0], bytes[1], bytes[2], bytes[3], bytes[4], bytes[5], bytes[6], bytes[7]
    );
    type_txt(buffer);

    for (std::size_t byte = 0; byte < Output_bytes_per_line::dump_hex_petscii; ++byte) {
        type_petscii_chr(bytes[byte]);
    }
};


void Monitor::Console::tick() {
    if (output_state != os_active) return;

    const u16 old_addr_cur = addr_cur;

    using cc = Cmd_char;
    switch (active_cmd) {
        case cc::disasm:           print_disasm();           break;
        case cc::dump_petscii:     print_dump_petscii();     break;
        case cc::dump_hex_petscii: print_dump_hex_petscii(); break;
        default:
            output_state = os_idle;
            return;
    }

    line_feed();

    if (is_in_wrapped_range(addr_end, old_addr_cur, addr_cur)) { // did we touch 'addr_end'?
        output_state = os_idle;
    }
}


void Monitor::CPU::draw(PETSCII_Draw& pd) { pd.txt("CPU View [TODO]", 100, 100, color_fg, color_bg); }
void Monitor::VIC::draw(PETSCII_Draw& pd) { pd.txt("VIC View [TODO]", 100, 100, color_fg, color_bg); }
void Monitor::CIA::draw(PETSCII_Draw& pd) { pd.txt("CIA View [TODO]", 100, 100, color_fg, color_bg); }
