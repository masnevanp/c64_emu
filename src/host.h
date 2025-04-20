#ifndef HOST_H_INCLUDED
#define HOST_H_INCLUDED

#include <SDL.h>
#include "common.h"
#include "utils.h"
#include "menu.h"



namespace Host {


// TODO: paddles/lightpen
class Input {
public:
    struct Handlers {
        Sig_key keyboard;
        Sig_key controller_1;
        Sig_key controller_2;
        Sig_key sys;
        Sig1<const char*> filedrop;
    };

    static const u8 JOY_ID_BIT = 0x10; // included in the joy. key code

    void poll();

    void swap_joysticks();

    Input(Handlers& handlers_);

    ~Input() {
        if (sdl_joystick[0]) SDL_JoystickClose(sdl_joystick[0]);
        if (sdl_joystick[1]) SDL_JoystickClose(sdl_joystick[1]);
    }

private:
    static u8 KC_LU_TBL[];
    static const u8 SC_LU_TBL[];

    SDL_Event sdl_ev;

    Handlers& handlers;

    Sig_key* joy_handler[2];

    SDL_Joystick* sdl_joystick[2] = { nullptr, nullptr }; // TODO: struct these togeteher
    SDL_JoystickID sdl_joystick_id[2];

    u16 sh_l_idx = 0;
    u16 sh_r_idx = 0;
    u16 sh_r_down = 0x00; // bit map - keeps track of 'auto shifted' keys

    void handle_key(u8 down) {
        u8 code = translate_sdl_key();
        output_key(code, down);
    }

    u8 translate_sdl_key();
    void output_key(u8 code, u8 down);
    void handle_joy_axis();

    void handle_joy_btn(u8 down) {
        SDL_JoyButtonEvent& joy_ev = sdl_ev.jbutton;
        Sig_key& output = get_joy_output(joy_ev.which);
        output(Key_code::Joystick::jb, down);
    }

    Sig_key& get_joy_output(SDL_JoystickID which) {
        int joy_idx = (which == sdl_joystick_id[0]) ? 0 : 1;
        return *(joy_handler[joy_idx]);
    }

    void handle_win_ev() {
        switch (sdl_ev.window.event) {
            case SDL_WINDOWEVENT_CLOSE:
                handlers.sys(Key_code::System::quit, true);
                break;
            case SDL_WINDOWEVENT_FOCUS_GAINED:
                set_shift_lock();
                break;
        }
    }

    void handle_dropfile() {
        handlers.filedrop(sdl_ev.drop.file);
        SDL_free(sdl_ev.drop.file);
    }

    void set_shift_lock() {
        bool down = SDL_GetModState() & KMOD_CAPS;
        // disable/enable left shift
        KC_LU_TBL[sh_l_idx] = down
            ? (Key_code::Keyboard)Key_code::System::nop
            : Key_code::Keyboard::sh_l;

        handlers.keyboard(Key_code::Keyboard::sh_l, down);
    }

    void disable_sh_r() { KC_LU_TBL[sh_r_idx] = Key_code::System::nop; }
    void enable_sh_r() { KC_LU_TBL[sh_r_idx] = Key_code::Keyboard::sh_r; }

};


class Video_out {
public:
    static constexpr auto pixel_format = SDL_PIXELFORMAT_ARGB8888;
    static constexpr int bytes_per_pixel = 4;

    enum class Mode : u8 { win, fullscr_win, fullscr };

    struct Settings {
        Choice<Mode> mode{
            {Mode::win,  Mode::fullscr_win, Mode::fullscr},
            {"WINDOWED", "FULLSCREEN", "TRUE FULLSCREEN"},
        };
        // TODO: fullscreen_scale?
        // init, min, max, step, to_str
        Param<double> window_scale{3.40,  0.5, 8.00, 0.05,  [](double d){ return to_string(d, 2); }};
        Param<double> aspect_ratio{0.918, 0.5, 1.25, 0.001, [](double d){ return to_string(d, 3); }}; // PAL: ~0.936

        Param<u8> sharpness{0, 0, 3, 1};

        Param<u8> brightness{82, 0, 100, 1};
        Param<u8> contrast {100, 0, 100, 1};
        Param<u8> saturation{74, 0, 100, 1};

        Param<u8> mask_pattern{3, 0, 254, 1}; // TODO: actual max
        Param<u8> mask_level {15, 0,  15, 1}; // 0 --> all pass
    };
    Menu::Group settings_menu() { return Menu::Group("VIDEO /", menu_items); }

    Video_out(const double& frame_rate_in_) : frame_rate_in(frame_rate_in_) {}
    ~Video_out();

    void put(const u8* vic_frame);

    void flip() { SDL_RenderPresent(renderer); }

    void toggle_fullscr_win() { // TODO: cycle through presets instead --> TODO: presets...
        set.mode = (set.mode == Mode::win) ? Mode::fullscr_win : Mode::win;
        upd_mode();
    }

    void reconfig() { upd_mode(); }

    bool v_synced() const { return double(sdl_mode.refresh_rate) == frame_rate_in; }

    static SDL_Texture* create_texture(SDL_Renderer* r, SDL_TextureAccess ta, SDL_BlendMode bm,
                                            int w, int h);
private:
    struct SDL_frame {
        const int max_w;
        const int max_h;
        const SDL_TextureAccess ta;
        const SDL_BlendMode bm;

        SDL_Texture* texture = nullptr;
        SDL_Rect srcrect = {0, 0, 0, 0};
        SDL_Rect dstrect = {0, 0, 0, 0};
        u32* pixels = nullptr;

        SDL_frame(int max_w_, int max_h_, SDL_TextureAccess ta_, SDL_BlendMode bm_);
        ~SDL_frame();

        void connect(SDL_Renderer* renderer);
        void copy(SDL_Renderer* r) {
            SDL_RenderCopy(r, texture, &srcrect, &dstrect);
        }
    };

    struct Frame {
        static constexpr int max_w = VIC_II::FRAME_WIDTH * 3; // TODO: non-hardcoded
        static constexpr int max_h = VIC_II::FRAME_HEIGHT * 3; // TODO: non-hardcoded

        void upd_palette(const Settings& set);
        void upd_sharpness(const Settings& set);

        void put(const u8* vic_frame, SDL_Renderer* r) {
            draw(vic_frame);
            frame.copy(r);
        }

        u32 col(const u8 vic_pixel) { return palette[vic_pixel & 0xf]; } // TODO: do '&' here? (or in VIC?)}

        std::function<void (const u8* )> draw;

        u32 palette[16];
        SDL_frame frame{max_w, max_h, SDL_TEXTUREACCESS_STREAMING, SDL_BLENDMODE_NONE};
    };

    struct Mask {
        static constexpr int max_w = 1920; // TODO: non-hardcoded
        static constexpr int max_h = 1080; // TODO: non-hardcoded

        using Pattern = std::vector<std::vector<u32>>;
        static const std::vector<Pattern> patterns;

        void upd(const Settings& set);

        void put(SDL_Renderer* r) { frame.copy(r); }

        SDL_frame frame{max_w, max_h, SDL_TEXTUREACCESS_STATIC, SDL_BLENDMODE_MOD};
    };

    void upd_mode();
    void upd_dimensions();

    const double& frame_rate_in;

    Settings set;

    SDL_DisplayMode sdl_mode = { 0, 0, 0, 0, 0 };

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;

    Frame frame;
    Mask mask;

    std::vector<Menu::Knob> menu_items{
        //name                          connected setting   notify
        {"VIDEO / MODE",                set.mode,           [&](){ upd_mode(); }},
        {"VIDEO / WINDOW SCALE",        set.window_scale,   [&](){ upd_dimensions(); }},
        {"VIDEO / ASPECT RATIO",        set.aspect_ratio,   [&](){ upd_dimensions(); }},
        {"VIDEO / SHARPNESS",           set.sharpness,      [&](){ frame.upd_sharpness(set); }},
        {"VIDEO / COLODORE BRIGHTNESS", set.brightness,     [&](){ frame.upd_palette(set); }},
        {"VIDEO / COLODORE CONTRAST",   set.contrast,       [&](){ frame.upd_palette(set); }},
        {"VIDEO / COLODORE SATURATION", set.saturation,     [&](){ frame.upd_palette(set); }},
        {"VIDEO / MASK PATTERN",        set.mask_pattern,   [&](){ mask.upd(set); }},
        {"VIDEO / MASK LEVEL",          set.mask_level,     [&](){ mask.upd(set); }},
    };
};


// TODO: error handling (e.g. on error just run without audio?)
class Audio_out {
public:
    static constexpr int bytes_per_sample = 2;

    ~Audio_out();

    u16 config(u16 buf_sz);

    int put(const i16* chunk, u32 sz) {
        SDL_QueueAudio(dev, chunk, sz * bytes_per_sample);
        return SDL_GetQueuedAudioSize(dev) / bytes_per_sample;
    }

    void flush() { SDL_ClearQueuedAudio(dev); }

private:
    SDL_AudioDeviceID dev = 0;

};


class _SDL { // classic...
public:
    static _SDL& instance() { static _SDL _sdl; return _sdl; }
    ~_SDL() { if (init) SDL_Quit(); }
private:
    _SDL() {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO |  SDL_INIT_JOYSTICK) != 0) {
            Log::error("Unable to initialize SDL: %s", SDL_GetError());
            exit(1);
        } else {
            init = true;
        }

    }
    _SDL(const _SDL& ) = delete;
    void operator=(const _SDL& ) = delete;
    bool init = false;
};
extern _SDL& _sdl;

}

#endif // HOST_H_INCLUDED
