#ifndef MONITOR_H_INCLUDED
#define MONITOR_H_INCLUDED

#include <array>
#include "common.h"
#include "state.h"
#include "utils.h"


// namespace System {


class Monitor {
public:
    Monitor(
        State::System& s, const State::System::ROM& rom,
        const std::function<u8 (u16)>& peek_sys_map
    ) : con(s, rom, peek_sys_map) {}

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
            const std::function<u8 (u16)>& peek_sys_map
        ) : as(s, rom, peek_sys_map) { clr_screen(); }

        virtual void key(u8 code, const Mod_state& mod);
        virtual void draw(PETSCII_Draw& pd);

    private:
        static constexpr int column_count = (frame_width / 8) - 1;
        static constexpr int line_count = (frame_height / 8) - 1;

        static constexpr char no_cmd = 0;

        enum Range_output_state {
            os_idle = 0,
            os_paused = 1,
            os_active = 2,
        };

        using Line = std::array<u16, column_count>; // char rom indices
        using Screen = std::array<Line, line_count>;

        Screen screen{};

        int cursor_x = 0;
        int cursor_y = 0;

        class Address_space {
        public:
            Address_space(
                State::System& s_, const State::System::ROM& rom_,
                const std::function<u8 (u16)>& peek_sys_map_
            ) : s(s_), rom(rom_), peek_sys_map(peek_sys_map_) {}

            struct Target {
                enum ID : u8 {
                    sys_map_p = 0, sys_map_rw, ram, basic, kernal, // color_ram, reu?
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

            // TODO: write
        private:
            u8 target_id{Target::ID::sys_map_p};

            State::System& s;
            const State::System::ROM& rom;

            const std::function<u8 (u16)> peek_sys_map;
        };

        class Address_range {
        public:
            void init(u16 start_addr, u16 end_addr) {
                cur = start_addr;
                end = end_addr;
                ended = false;
            }

            u16 peek() const { return cur; }

            u16 next() {
                if (cur == end) ended = true;
                return cur++;
            }

            bool at_end() const { return ended; }

        private:
            u16 cur;
            u16 end;
            bool ended;
        };

        Address_space as;

        Address_range ar;

        char active_cmd = no_cmd;

        Range_output_state output_state = os_idle;

        void clr_screen();

        void scroll_up();
        void scroll_down();

        void cursor_up  () { if (cursor_y > 0) --cursor_y; else scroll_down(); }
        void cursor_down() { if (++cursor_y == line_count) { --cursor_y; scroll_up(); } }
        void cursor_fwd () { if (++cursor_x == column_count) { cursor_x = 0; cursor_down(); } }
        void cursor_back() { if (cursor_x > 0) --cursor_x; else { cursor_x = (column_count - 1); cursor_up(); } }

        void line_feed  () { cursor_x = 0; cursor_down(); }

        std::vector<std::string> tokenize_current_line() const;

        void handle_ret();
        void handle_cmd(const std::vector<std::string>& args);

        void type_chr(u16 char_rom_index)      { screen[cursor_y][cursor_x] = char_rom_index; cursor_fwd(); }
        void type_ascii_chr(u8 ascii_code)     { type_chr(ascii_to_char_code(ascii_code)); }
        void type_petscii_chr(u8 petscii_code) { type_chr(petscii_to_screen_code(petscii_code)); } // maps to lower half of char rom
        void type_txt(const std::string& txt)  { for (const char c : txt) type_ascii_chr(c); }
        void print(const std::string& txt)     { type_txt(txt); line_feed(); }

        void print_d();
        void print_i();
        void print_m();

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