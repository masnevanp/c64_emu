#ifndef MONITOR_H_INCLUDED
#define MONITOR_H_INCLUDED

#include <array>
#include "common.h"
#include "state.h"
#include "utils.h"


// namespace System {


class Monitor {
public:
    Monitor(State::System& s_) : s(s_) {}

    void key(u8 code, bool down);

    u8* draw(const u8* charrom);

    bool active = false;

private:
    using Key_code = Key_code::Keyboard;

    static constexpr int frame_width = VIC_II::FRAME_WIDTH;
    static constexpr int frame_height = VIC_II::FRAME_HEIGHT;

    static constexpr Color color_bg = Color::blue;
    static constexpr Color color_fg = Color::white;

    struct Mod_state {
        bool shift = false;
        bool ctrl = false;
        bool cmdre = false;
    };

    class View {
    public:
        virtual void key(u8 code, bool down, const Mod_state& mod) { UNUSED2(code, down); UNUSED(mod); };
        virtual void draw(PETSCII_Draw& pd) = 0;

        virtual ~View() {}
    };

    class Console : public View {
    public:
        Console() { clr_text(); }

        virtual void key(u8 code, bool down, const Mod_state& mod);
        virtual void draw(PETSCII_Draw& pd);
    private:
        static constexpr int text_width = (frame_width - 1) / 8;
        static constexpr int text_height = (frame_height - 1) / 8;

        std::array<std::array<u8, text_width>, text_height> text{};

        int crsr_x = 0;
        int crsr_y = 0;

        void clr_text();
    };

    class CPU : public View { public: virtual void draw(PETSCII_Draw& pd); };
    class VIC : public View { public: virtual void draw(PETSCII_Draw& pd); };
    class CIA : public View { public: virtual void draw(PETSCII_Draw& pd); };

    Console con;
    CPU cpu;
    VIC vic;
    CIA cia;

    u8 frame[VIC_II::FRAME_SIZE] = {};

    std::array<View*, 4> views{ &con, &cpu, &vic, &cia };
    int active_view = 0;

    Mod_state mod;

    State::System& s;
};


// } // namespace System


#endif // MONITOR_H_INCLUDED