#include "monitor.h"
#include "mos6502/asm.h"
#include <array>
#include <algorithm>



inline bool is_hex(std::string_view s) { return is_hex(s, std::size(s)); }

inline bool is_hex_byte(std::string_view s) { return is_hex(s, 2); }
inline bool is_hex_word(std::string_view s) { return is_hex(s, 4); }


static const char* cmd_help[] = {
    " d [adr1] [adr2]  : disassemble",
    " i [adr1] [adr2]  : dump petscii",
    " m [adr1] [adr2]  : dump hex & petscii",
    " s [n]            : select target",
    " ?                : help",
    " ",
    " ",
};


struct Command {
    enum class ID : u8 {
        none,
        help,
        disasm,
        dump_petscii,
        dump_hex_petscii,
        select_tgt,
        assemble,
        disasm_edit,
        dump_petscii_line,
        hex_edit,
    };

    const ID id;
    const char chr;
    const char output_prefix;
    const std::size_t output_bytes_per_line;
};

using cmd = Command::ID;

static constexpr Command commands[] = {
    { cmd::none,                0,   0,  0, },
    { cmd::help,              '?',   0,  0, },
    { cmd::disasm,            'd', '>',  1, },
    { cmd::dump_petscii,      'i', ';', 32, },
    { cmd::dump_hex_petscii,  'm', ':',  8, },
    { cmd::select_tgt,        's',   0,  0, },
    { cmd::assemble,          'a', 'a',  0, },
    { cmd::disasm_edit,       '>',   0,  1, },
    { cmd::dump_petscii_line, ';',   0,  0, }, // TODO: support 'petscii_edit' instead?
    { cmd::hex_edit,          ':',   0,  0, },
};


static constexpr auto default_disasm_prefix = commands[u8(cmd::disasm)].output_prefix;

static constexpr std::size_t default_output_seq_lines = 25;


u32 get_hex_val(const std::string& hex_str, u32 default_val) {
    if (is_hex(hex_str)) {
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
    if (output_seq == Output_sequence::active) {
        if (code == Key_code::r_stp) {
            output_seq = Output_sequence::paused;
        }
        return;
    }

    const auto ascii = mod.shift ? keycode_shifted_to_ascii[code] : keycode_to_ascii[code];

    if (ascii) return type(ascii);

    switch (code) {
        case Key_code::r_stp:
            if (output_seq == Output_sequence::paused) output_seq = Output_sequence::active;
            else clr_line();
            break;
        case Key_code::home:
            cursor_y = cursor_x = 0;
            if (mod.shift) clr_screen();
            break;
        case Key_code::crs_d: mod.shift ? cursor_up() : cursor_down();           break;
        case Key_code::crs_r: mod.shift ? cursor_back() : cursor_fwd();          break;
        case Key_code::ret:   handle_cmd(tokenize_current_line());               break;
        case Key_code::del:
            if (mod.shift) {
                clr_line();
            } else {
                cursor_back(); type(' '); cursor_back();
            }
            break;
    }
}


void Monitor::Console::draw(PETSCII_Draw& pd) {
    static constexpr int text_top_left_x = (VIC_II::FRAME_WIDTH - (screen_width * 8)) / 2;
    static constexpr int text_top_left_y = (VIC_II::FRAME_HEIGHT - (screen_height * 8)) / 2;

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
        for (u16 y = 0; y < screen_height; ++y) {
            for (u16 x = 0; x < screen_width; ++x) {
                draw_chr(screen[y][x], y, x);
            }
        }
    };

    tick();

    draw_screen();

    if (output_seq != Output_sequence::active) draw_cursor();
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
    for (auto& line : screen) for (auto& c : line) {
        c = ascii_to_char_code(' ');
    }
}


void Monitor::Console::clr_line() {
    for (auto& c : screen[cursor_y]) {
        c = ascii_to_char_code(' ');
    }
    cursor_x = 0;
}


void Monitor::Console::scroll_up() {
    std::fill(screen[0].begin(), screen[0].end(), ascii_to_char_code(' '));
    std::rotate(screen.begin(), screen.begin() + 1, screen.end());

    if (output_seq != Output_sequence::active) active_cmd_output();
}


void Monitor::Console::scroll_down() {
    std::fill(screen[screen_height - 1].begin(), screen[screen_height - 1].end(), ascii_to_char_code(' '));
    std::rotate(screen.rbegin(), screen.rbegin() + 1, screen.rend());

    active_cmd_output();
}


std::string Monitor::Console::current_line() const {
    std::array<u8, screen_width> line_ascii;

    std::transform(
        std::begin(screen[cursor_y]), std::end(screen[cursor_y]),
        std::begin(line_ascii),
        [](u16 char_rom_idx) -> u8 { return char_code_to_ascii(char_rom_idx); }
    );

    return {std::begin(line_ascii), std::end(line_ascii)};
}


void Monitor::Console::parse_cmd(const Tokens& tokens) {
    if (std::size(tokens) > 0 && std::size(tokens[0]) == 1) {
        const char maybe_cmd = tokens[0][0];

        for (const auto cmd : commands) {
            if (cmd.chr == maybe_cmd) {
                active_cmd = u8(cmd.id);
                return;
            }
        }
    }

    active_cmd = u8(cmd::none);
}


bool Monitor::Console::parse_addr1(const Tokens& tokens) {
    if (std::size(tokens) >= 2) {
        const auto& maybe_addr = tokens[1];
        if (is_hex(maybe_addr)) {
            addr_cur = get_hex_val(maybe_addr, addr_cur);
            return true;
        }
    }

    return false;
}


bool Monitor::Console::parse_prefix_and_addr1() {
    const auto tokens = tokenize_current_line();

    if (std::size(tokens) > 0 && std::size(tokens[0]) == 1) {
        const char maybe_prefix = tokens[0][0];

        for (const auto cmd : commands) {
            if (cmd.output_prefix == maybe_prefix || cmd.chr == maybe_prefix) {
                active_cmd = u8(cmd.id);
                break;
            }
        }
    }

    return parse_addr1(tokens);
}


static constexpr std::size_t guess_depth = 25;

// Recursively walk backwards trying different intr.sizes, and select (=guess) the best fit...
// Incorrect alignment ususally gives more undocumented instructions, so we take advantage of that.
// Possible improvements:
//     - During normal forward disassembly keep track of recently seen 'good' addresses.
//       (e.g. a circular buffer of size 20)
//     - Try to validate jump/branch targets
//       (is it in valid range? does it look like a valid instruction stream?)
std::pair<u8, int> guess_prev_instr_size(const std::array<u8, guess_depth>& bytes, u8 pos) {
    int top_score = 0;
    u8 top_size = 0;

    auto check_size = [&](u8 s, int reward) {
        if (MOS6502::Asm::instr(bytes[pos - s]).size == s) {
            auto score = guess_prev_instr_size(bytes, pos - s).second + 1 + reward;

            // penalties for undocumented instructions
            if (MOS6502::Asm::is_undoc(bytes[pos - s])) score -= s;
            if (MOS6502::Asm::is_hlt(bytes[pos - s])) score -= 5; // extra penalty for hlt

            if (score > top_score) {
                top_size = s;
                top_score = score;
            }
        }
    };

    // NOTE: Instruction size distribution in actual code is roughly: 1: 25%, 2: 50%, 3: 25%
    //       --> 2 byte instructions get the biggest reward here (TODO: more testing...)
    if (pos > 0) check_size(1, 1);
    if (pos > 1) check_size(2, 3);
    if (pos > 2) check_size(3, 2);

    return {top_size, top_score};
}


void Monitor::Console::goto_next_addr(bool higher) {
    switch (commands[active_cmd].id) {
        case cmd::disasm: case cmd::disasm_edit: {
            if (higher) {
                const u8 opc = as.read(addr_cur);
                addr_cur += MOS6502::Asm::instr(opc).size;
            } else {
                std::array<u8, guess_depth> bytes;
                for (u8 b = 1; b <= guess_depth; ++b) {
                    bytes[guess_depth - b] = as.read(addr_cur - b);
                }

                auto sz = guess_prev_instr_size(bytes, guess_depth).first;
                if (sz == 0) sz = 1; // was unable to guess --> make it 1, and hope for the best... :/
                addr_cur -= sz;
            }
            break;
        }
        default:
            if (higher) addr_cur += commands[active_cmd].output_bytes_per_line;
            else addr_cur -= commands[active_cmd].output_bytes_per_line;
            break;
    }
}


struct Machine_code_instr {
    u16 pc;
    u8 size;
    u8 bytes[3];
};

// NOTE: in many cases trailing garbage is just ignored, e.g 'lda $20,xpp' => 'lda $20,x'.... meh...
Machine_code_instr generate_code(const std::string& asm_line) {
    using namespace MOS6502::Asm;

    std::size_t line_pos = 0;
    Machine_code_instr code{0x0000, 0, {}};

    auto hex_val = [&](std::string_view s) { return std::stoul(s.data(), nullptr, 16); };

    auto next_chr = [&](bool skip_spaces = true) -> char {
        // NOTE: fails if there can be other types of whitespaces (fix later, if needed...)
        if (skip_spaces) while (asm_line[line_pos] == ' ') line_pos++;
        if (line_pos >= asm_line.size() || asm_line[line_pos] == ';') return '\0';
        return asm_line[line_pos++];
    };

    // TODO: a general 'scan_token()' that can be used everywhere

    auto scan_pc = [&]() -> bool {
        int chr_count = 0;
        while (auto c = next_chr(false)) {
            chr_count = (c == ' ') ? 0 : chr_count + 1;
            if (chr_count == 4 && (next_chr(false) == ' ')) {
                const auto maybe_pc = asm_line.substr(line_pos - 5, 4);
                if (is_hex_word(maybe_pc)) {
                    code.pc = hex_val(maybe_pc);
                    return true;
                }
            }
        }

        return false;
    };

    auto scan_op = [&]() -> Op {
        int chr_count = 0;
        while (auto c = next_chr(false)) {
            chr_count = (c == ' ') ? 0 : chr_count + 1;
            if (chr_count == 3) {
                const auto maybe_op = as_upper(asm_line.substr(line_pos - 3, 3));
                for (u8 op = 0; op < u8(Op::count); ++op) {
                    if (strcmp(Op_str[op], maybe_op.c_str()) == 0) {
                        return Op(op);
                    }
                }
            }
        }
    
        return Op::UNKNOWN;
    };

    auto scan_addr = [&]() {
        char maybe_addr[5] {'\0', '\0', '\0', '\0', '\0'};
        int size = 0;

        while(size < 5) {
            auto c = next_chr();
            if (c == ',' || c == '\0') break;
            maybe_addr[size++] = c;
        }

        if (size == 2 && is_hex_byte(maybe_addr)) {
            code.bytes[1] = hex_val(maybe_addr);
            auto c = next_chr();
            if (c == '\0') return Addr_mode::zp;
            if (c == 'x') return Addr_mode::zpx;
            if (c == 'y') return Addr_mode::zpy;
        } else if (size == 4 && is_hex_word(maybe_addr)) {
            auto addr = hex_val(maybe_addr);
            code.bytes[1] = u8(addr);
            code.bytes[2] = u8(addr >> 8);
            auto c = next_chr();
            if (c == '\0') return Addr_mode::abs;
            if (c == 'x') return Addr_mode::absx;
            if (c == 'y') return Addr_mode::absy;
        }

        return Addr_mode::unknown;
    };

    auto scan_rel_addr = [&]() {
        char maybe_addr[5] {'\0', '\0', '\0', '\0', '\0'};
        int size = 0;

        while(size < 5) {
            auto c = next_chr();
            if (c == ',' || c == '\0') break;
            maybe_addr[size++] = c;
        }

        if (size == 4 && is_hex_word(maybe_addr)) {
            u16 addr = hex_val(maybe_addr);
            code.bytes[1] = u8(addr - u16(code.pc + 2));
            return Addr_mode::rel;
        }

        return Addr_mode::unknown;
    };

    auto scan_imm = [&]() {
        char maybe_imm[3] {'\0', '\0', '\0'};

        maybe_imm[0] = next_chr();
        maybe_imm[1] = next_chr();
        if (is_hex_byte(maybe_imm)) {
            code.bytes[1] = hex_val(maybe_imm);
            return Addr_mode::imm;
        }

        return Addr_mode::unknown;
    };

    auto scan_ind_addr = [&]() {
        char maybe_addr[5] {'\0', '\0', '\0', '\0', '\0'};
        int size = 0;

        while(size < 5) {
            auto c = next_chr();
            if (c == ',' || c == ')' || c == '\0') break;
            maybe_addr[size++] = c;
        }

        if (size == 2 && is_hex_byte(maybe_addr)) {
            code.bytes[1] = hex_val(maybe_addr);
            auto c = next_chr();
            if (c == 'x') return Addr_mode::indx;
            if ((c == ',') && (next_chr() == 'y')) return Addr_mode::indy;
        } else if (size == 4 && is_hex_word(maybe_addr)) {
            auto addr = hex_val(maybe_addr);
            code.bytes[1] = u8(addr);
            code.bytes[2] = u8(addr >> 8);
            /*if (next_chr() == '\0')*/ return Addr_mode::ind;
        }

        return Addr_mode::unknown;
    };

    auto scan_addr_mode = [&](Op op) {
        switch (next_chr()) {
            case '\0': return Addr_mode::impl;
            case 'a' : return Addr_mode::accu;
            case '$' : return is_branch(op) ? scan_rel_addr() : scan_addr();
            case '#' : if (next_chr() == '$') return scan_imm(); else break;
            case '(' : if (next_chr() == '$') return scan_ind_addr(); else break;
            default  : break;
        }

        return Addr_mode::unknown;
    };

    auto scan_instr = [&]() {
        const auto op = scan_op();
        const auto addr_mode = scan_addr_mode(op);

        for (auto instr : instruction) {
            if (instr.op == op && instr.addr_mode == addr_mode) {
                code.size = instr.size;
                code.bytes[0] = instr.code;
                if (!is_undoc(instr.code)) break; // prefere the documented ones...
            }
        }
        Log::info("%04x %02x %02x --> %02x", code.pc, u8(op), u8(addr_mode), code.bytes[0]);
    };

    if (scan_pc()) scan_instr();

    // TODO: pseudo_instr (ROL -> ROL A, BRK impl?)
    return code;
}


void Monitor::Console::handle_cmd(const Tokens& args) {
    auto get_hex_arg = [&](std::size_t arg_pos, u16 default_val) {
        return arg_pos < args.size()
            ? u16(get_hex_val(args[arg_pos], default_val))
            : default_val;
    };

    auto init_output_seq = [&]() {
        parse_addr1(args);
        const auto seq_size = (default_output_seq_lines * commands[active_cmd].output_bytes_per_line) - 1;
        addr_end = get_hex_arg(2, addr_cur + seq_size);
        output_seq = Output_sequence::active;
    };

    auto handle_select_tgt = [&]() {
        const auto new_target_id = get_hex_arg(1, as.target().id);
        as.select(new_target_id);

        if (!as.is_valid(addr_cur)) {
            addr_cur = as.target().addr_start;
        }

        const char* format = "%s%02x: %04x-%04x  %s";

        line_feed();
        for (auto t : as.targets) {
            line_feed();
            const std::string selected = t.id == as.target().id ? "->" : "  ";
            sprintf(line_buffer, format, selected.c_str(), t.id, t.addr_start, t.addr_end, t.name);
            Line_write().txt(line_buffer);
        }

        line_feed(); clr_line(); line_feed(); clr_line();
    };

    auto handle_assemble = [&]() {
        const auto code = generate_code(current_line());

        if (code.size == 0) return; // TODO: notify user '; ???'

        addr_cur = code.pc;

        for (u8 b = 0; b < code.size; ++b) {
            as.write(addr_cur + b, code.bytes[b]);
        }
        
        output_disasm(commands[u8(cmd::assemble)].output_prefix);

        addr_cur += code.size;

        output_seq = Output_sequence::active; // for printing the next address on next line
    };

    auto handle_disasm_edit = [&]() {
        if (!parse_addr1(args)) return; // TODO: active cmd = none?

        for (u8 byte = 0; byte < 3; ++byte) {
            const std::size_t argn = byte + 2;

            if (argn >= args.size() || !is_hex_byte(args[argn])) break;

            const u8 byte_val = get_hex_val(args[argn], 0);
            as.write(addr_cur + byte, byte_val);
        }

        output_disasm(default_disasm_prefix);

        goto_next_addr(true);

        output_seq = Output_sequence::active; // output also the next instr (it might have changed)
    };

    auto handle_dump_petscii_line = [&]() {
        if (!parse_addr1(args)) return;
        output_dump_petscii();
    };

    auto handle_hex_edit = [&]() {
        if (!parse_addr1(args)) return;

        for (u8 byte = 0; byte < 8; ++byte) {
            const std::size_t argn = byte + 2;

            if (argn >= args.size() || !is_hex_byte(args[argn])) break;

            const u8 byte_val = get_hex_val(args[argn], 0);
            as.write(addr_cur + byte, byte_val);
        }

        output_dump_hex_petscii();
    };

    parse_cmd(args);

    switch (commands[active_cmd].id) {
        case cmd::help: output_help(); return;
        case cmd::disasm: case cmd::dump_petscii: case cmd::dump_hex_petscii:
            init_output_seq();
            break;
        case cmd::select_tgt: handle_select_tgt(); return;
        case cmd::assemble: handle_assemble(); break;
        case cmd::disasm_edit: handle_disasm_edit(); break;
        case cmd::dump_petscii_line: handle_dump_petscii_line(); break;
        case cmd::hex_edit:   handle_hex_edit(); break;
        default:
            output_seq = Output_sequence::off; // TODO: needed?
            break;
    }

    line_feed();
}


void Monitor::Console::output_help() {
    for (const auto h : cmd_help) {
        line_feed();
        Line_write().txt(h);
    }
}


void Monitor::Console::output_disasm(char prefix) {
    auto do_print = [&](const MOS6502::Asm::Line& line, const std::string& undoc) {
        const char* format = "%c %04s  %-08s  %-012s %s";

        sprintf(line_buffer, format,
            prefix,
            as_lower(line.pc).c_str(),
            as_lower(line.bytes).c_str(),
            as_lower(line.text).c_str(),
            undoc.c_str()
        );
        Line_write().txt(line_buffer);
    };

    const u8 opc = as.read(addr_cur);

    const std::string undoc{MOS6502::Asm::is_undoc(opc) ? "; undoc" : " "};

    switch (MOS6502::Asm::instr(opc).size) {
        case 1:
            do_print(MOS6502::Asm::disasm_one(addr_cur, opc), undoc);
            break;
        case 2:
            do_print(MOS6502::Asm::disasm_one(addr_cur, opc, as.read(addr_cur + 1)), undoc);
            break;
        case 3:
            do_print(MOS6502::Asm::disasm_one(addr_cur, opc, as.read(addr_cur + 1),
                        as.read(addr_cur + 2)), undoc);
    }
}


void Monitor::Console::output_dump_petscii() {
    const char* format = "%c %04x  ";

    auto wr = Line_write();

    sprintf(line_buffer, format, commands[u8(cmd::dump_petscii)].output_prefix, addr_cur);
    wr.txt(line_buffer);

    const auto bpl = commands[u8(cmd::dump_petscii)].output_bytes_per_line;
    for (std::size_t n = 0; n < bpl; ++n) {
        wr.petscii(as.read(addr_cur + n));
    }
}


void Monitor::Console::output_dump_hex_petscii() {
    const char* format = "%c %04x  %02x %02x %02x %02x %02x %02x %02x %02x  ";

    auto wr = Line_write();

    u16 line_addr = addr_cur;

    static constexpr auto bpl = commands[u8(cmd::dump_hex_petscii)].output_bytes_per_line;

    std::array<u8, bpl> bytes;
    std::generate(begin(bytes), end(bytes), [&]() { return as.read(line_addr++); });

    sprintf(line_buffer, format,
        commands[u8(cmd::dump_hex_petscii)].output_prefix, addr_cur,
        bytes[0], bytes[1], bytes[2], bytes[3], bytes[4], bytes[5], bytes[6], bytes[7]
    );
    wr.txt(line_buffer);

    for (const auto b : bytes) wr.petscii(b);
};


void Monitor::Console::active_cmd_output() {
    switch (commands[active_cmd].id) {
        case cmd::disasm:           output_disasm(default_disasm_prefix); break;
        case cmd::dump_petscii:     output_dump_petscii();                break;
        case cmd::dump_hex_petscii: output_dump_hex_petscii();            break;
        default: return;
    }
}


void Monitor::Console::tick() {
    if (output_seq != Output_sequence::active) return;

    switch (commands[active_cmd].id) {
        case cmd::disasm: case cmd::dump_petscii: case cmd::dump_hex_petscii: {
            active_cmd_output();
            line_feed();

            const u16 old_addr_cur = addr_cur;
            goto_next_addr(true);

            if (is_in_wrapped_range(addr_end, old_addr_cur, addr_cur)) { // did we touch 'addr_end'?
                output_seq = Output_sequence::off;
            }

            break;
        }
        case cmd::assemble: { // TODO: --> a function
            const char* format = "%c %04x  ";
            auto wr = Line_write();
            sprintf(line_buffer, format, commands[u8(cmd::assemble)].output_prefix, addr_cur);
            wr.txt(line_buffer);
            output_seq = Output_sequence::off;
            cursor_x = 8;
            break;
        }
        case cmd::disasm_edit: // TODO: --> a function
            output_disasm(default_disasm_prefix);
            output_seq = Output_sequence::off;
            cursor_x = 8;
            break;

        default: return;
    }
}


void Monitor::CPU::draw(PETSCII_Draw& pd) { pd.txt("CPU View [TODO]", 100, 100, color_fg, color_bg); }
void Monitor::VIC::draw(PETSCII_Draw& pd) { pd.txt("VIC View [TODO]", 100, 100, color_fg, color_bg); }
void Monitor::CIA::draw(PETSCII_Draw& pd) { pd.txt("CIA View [TODO]", 100, 100, color_fg, color_bg); }
