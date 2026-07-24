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

    void key(u8 code, bool down) {
        if (down && code == Key_code::ar_l) active_view = (active_view + 1) % views.size();
        else views[active_view]->key(code, down);
    }

    u8* draw(const u8* charrom) {
        for (auto& px : frame) px = color_bg;

        PETSCII_Draw pd{charrom, frame, frame_width};
        views[active_view]->draw(pd);

        return frame;
    };

    bool active = false;

private:
    using Key_code = Key_code::Keyboard;

    static constexpr int frame_width = VIC_II::FRAME_WIDTH;
    static constexpr int frame_height = VIC_II::FRAME_HEIGHT;

    static constexpr Color color_bg = Color::blue;
    static constexpr Color color_fg = Color::white;

    class View {
    public:
        virtual void key(u8 code, bool down) { UNUSED2(code, down); };
        virtual void draw(PETSCII_Draw& pd) = 0;

        virtual ~View() {}
    };

    class Console : public View {
    public:
        virtual void key(u8 code, bool down);
        virtual void draw(PETSCII_Draw& pd);
    private:
        static constexpr int text_width = (frame_width - 1) / 8;
        static constexpr int text_height = (frame_height - 1) / 8;

        std::array<std::array<u8, text_width>, text_height> text{};

        int crsr_x = 0;
        int crsr_y = 0;

        bool shift = false;
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

    State::System& s;
};


// } // namespace System


#endif // MONITOR_H_INCLUDED