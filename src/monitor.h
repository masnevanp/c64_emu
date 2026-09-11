#ifndef MONITOR_H_INCLUDED
#define MONITOR_H_INCLUDED

#include <array>
#include "common.h"
#include "state.h"
#include "utils.h"


// namespace System {


class Monitor {
public:
    Monitor(State::System& s) : con(s) {}

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
        Console(State::System& s_) : s(s_) { clr_screen(); }

        virtual void key(u8 code, const Mod_state& mod);
        virtual void draw(PETSCII_Draw& pd);

    private:
        static constexpr int column_count = (frame_width / 8) - 1;
        static constexpr int line_count = (frame_height / 8) - 1;

        enum Mode {
            idle, cmd_d, cmd_m,
        };

        using Line = std::array<u16, column_count>; // char rom indices
        using Screen = std::array<Line, line_count>;

        Screen screen{};

        int cursor_x = 0;
        int cursor_y = 0;

        u16 addr_cur;
        u16 addr_end;

        Mode mode = Mode::idle;

        void scroll_up();
        void scroll_down();

        void cursor_up  () { if (cursor_y > 0) --cursor_y; else scroll_down(); }
        void cursor_down() { if (++cursor_y == line_count) { --cursor_y; scroll_up(); } }
        void cursor_fwd () { if (++cursor_x == column_count) { cursor_x = 0; cursor_down(); } }
        void cursor_back() { if (cursor_x > 0) --cursor_x; else { cursor_x = (column_count - 1); cursor_up(); } }

        void line_feed  () { cursor_x = 0; cursor_down(); }

        void clr_screen();

        void type_chr(u16 char_rom_index)      { screen[cursor_y][cursor_x] = char_rom_index; cursor_fwd(); }
        void type_ascii_chr(u8 ascii_code)     { type_chr(ascii_to_char_code(ascii_code)); }
        void type_petscii_chr(u8 petscii_code) { type_chr(petscii_to_screen_code(petscii_code)); } // maps to lower half of char rom
        void type_txt(const std::string& txt)  { for (const char c : txt) type_ascii_chr(c); }
        void print(const std::string& txt)     { type_txt(txt); line_feed(); }

        void tick();

        State::System& s;
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