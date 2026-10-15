#ifndef MONITOR_H_INCLUDED
#define MONITOR_H_INCLUDED

#include <array>
#include "common.h"
#include "state.h"
#include "utils.h"


// namespace System {


class Monitor {
public:
    struct Sys_map_interface {
        const std::function<u8 (u16)> peek;
        const std::function<u8 (u16)> read;
        const std::function<void (u16, u8)> write;
    };

    Monitor(
        State::System& s, const State::System::ROM& rom,
        const Sys_map_interface& sys_map
    ) : con(s, rom, sys_map) {}

    void key(u8 code, bool down);

    u8* draw(const u8* charrom);

    bool active = false;

private:
    using Key_code = Key_code::Keyboard;

    static constexpr int frame_width = VIC_II::FRAME_WIDTH;
    static constexpr int frame_height = VIC_II::FRAME_HEIGHT;

    static constexpr Color color_bg = Color::blue;
    static constexpr Color color_fg = Color::white;
    static constexpr Color color_cursor = Color::cyan;

    struct Mod_state {
        bool shift = false;
        bool ctrl = false;
        bool cmdre = false;
    };

    struct View {
        virtual void key(u8 code, const Mod_state& mod) { UNUSED2(code, mod); };
        virtual void draw(PETSCII_Draw& pd) = 0;
    };

    struct Console : public View {
        Console(
            State::System& s, const State::System::ROM& rom,
            const Sys_map_interface& sys_map
        ) : as(s, rom, sys_map) { clr_screen(); output_help(); }

        virtual void key(u8 code, const Mod_state& mod);
        virtual void draw(PETSCII_Draw& pd);

    private:
        static constexpr std::size_t screen_width = (frame_width / 8) - 1;
        static constexpr std::size_t screen_height = (frame_height / 8) - 1;

        enum class Range_output {
            off    = 0,
            paused = 1,
            active = 2,
        };

        using Line = std::array<u16, screen_width>; // char rom indices
        using Screen = std::array<Line, screen_height>;

        using Tokens = std::vector<std::string>;

        class Address_space {
        public:
            Address_space(
                State::System& s_, const State::System::ROM& rom_,
                const Sys_map_interface& sys_map_
            ) : s(s_), rom(rom_), sys_map(sys_map_) {}

            struct Target {
                enum ID : u8 {
                    // TODO:
                    //   - char_rom, color_ram, io, rom h/l, reu?
                    //   - expansion inspection?
                    //   - a seperate view for c1541?
                    sys_map_p = 0, sys_map_rw, ram, basic, kernal,
                    //c1541_map, c1541_ram, c1541_dos, // TODO: c1541.peek(...)
                    _last = kernal
                };

                const ID id;
                const u16 addr_start;
                const u16 addr_end;
                const char* name;
            };

            static constexpr Target targets[] = {
                { Target::ID::sys_map_p,  0x0000, 0xffff, "sys map [peek only]" },
                { Target::ID::sys_map_rw, 0x0000, 0xffff, "sys map [read/write]" },
                { Target::ID::ram,        0x0000, 0xffff, "ram"     },
                { Target::ID::basic,      0xa000, 0xbfff, "basic"   },
                { Target::ID::kernal,     0xe000, 0xffff, "kernal"  },
            };

            void select(u8 new_target_id) {
                if (new_target_id <= Target::ID::_last) target_id = new_target_id;
            }

            const Target& target() const { return targets[target_id]; }

            bool is_valid(u16 address) {
                return (address >= target().addr_start) && (address <= target().addr_end);
            }

            u8 read(u16 address);
            void write(u16 address, u8 data);

        private:
            u8 target_id{Target::ID::sys_map_p};

            State::System& s;
            const State::System::ROM& rom;

            Sys_map_interface sys_map;
        };

        struct _Line_write {
            Console& con;

            u8 x = 0;

            void chr(u16 char_rom_index)     { con.screen[con.cursor_y][x++] = char_rom_index; }
            void ascii(u8 ascii_code)        { chr(ascii_to_char_code(ascii_code)); }
            void petscii(u8 petscii_code)    { chr(petscii_to_screen_code(petscii_code)); } // maps to lower half of char rom
            void txt(const std::string& txt) { for (const char c : txt) ascii(c); }
        
            ~_Line_write() {
                const u16 space = ascii_to_char_code(' ');
                while (x < con.screen_width) ascii(space);
            }
        };
        _Line_write Line_write() { return _Line_write{*this}; }

        Screen screen{};

        u8 cursor_x = 0;
        u8 cursor_y = 0;

        Address_space as;

        u16 addr_cur;
        u16 addr_end;

        u8 active_cmd{0};

        Range_output range_output{Range_output::off};

        char line_buffer[screen_width + 1];

        void clr_screen();
        void clr_line();

        void scroll_up();
        void scroll_down();

        void cursor_up  () {
            if (parse_prefix_and_addr1()) goto_next_addr(false);
            if (cursor_y > 0) --cursor_y; else scroll_down();
        }
        void cursor_down() {
            if (range_output != Range_output::active) {
                if (parse_prefix_and_addr1()) goto_next_addr(true);
            }
            if (++cursor_y == screen_height) { --cursor_y; scroll_up(); }
        }
        void cursor_fwd () { if (++cursor_x == screen_width) { cursor_x = 0; cursor_down(); } }
        void cursor_back() { if (cursor_x > 0) --cursor_x; else { cursor_x = (screen_width - 1); cursor_up(); } }

        void line_feed() { cursor_down(); cursor_x = 0; }

        Tokens tokenize_current_line() const;

        void parse_cmd(const Tokens& tokens);
        void parse_addr1(const Tokens& tokens);
        bool parse_prefix_and_addr1();

        void goto_next_addr(bool higher);

        void handle_cmd(const Tokens& tokens);
        
        void type(u8 ascii_code) {
            screen[cursor_y][cursor_x] = ascii_to_char_code(ascii_code);
            cursor_fwd();
        }

        void output_help();

        void output_disasm();
        void output_dump_petscii();
        void output_dump_hex_petscii();

        void active_cmd_output();

        void tick();
    };

    struct CPU : public View { virtual void draw(PETSCII_Draw& pd); };
    struct VIC : public View { virtual void draw(PETSCII_Draw& pd); };
    struct CIA : public View { virtual void draw(PETSCII_Draw& pd); };

    Console con;
    CPU cpu;
    VIC vic;
    CIA cia;

    u8 frame[VIC_II::FRAME_SIZE] = {};

    std::array<View*, 4> views{ &con, &cpu, &vic, &cia };
    int active_view = 0;

    Mod_state mod;
};


// } // namespace System


#endif // MONITOR_H_INCLUDED