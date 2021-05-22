#ifndef HOST_H_INCLUDED
#define HOST_H_INCLUDED

#include <functional>
#include <SDL.h>
#include "common.h"
#include "utils.h"
#include "vic_ii.h"
#include "menu.h"



namespace Host {


// TODO: paddles/lightpen
class Input {
public:
    struct Handlers {
        Sig_key keyboard;
        Sig_key sys;
        Sig_key joy1;
        Sig_key joy2;
    };

    static const u8 JOY_ID_BIT = 0x10; // included in the joy. key code

    void poll() { // TODO: filtering?
        while (SDL_PollEvent(&sdl_ev)) {
            switch (sdl_ev.type) {
                case SDL_KEYDOWN:       handle_key(true);      break;
                case SDL_KEYUP:         handle_key(false);     break;
                case SDL_JOYAXISMOTION: handle_joy_axis();     break;
                case SDL_JOYBUTTONDOWN: handle_joy_btn(true);  break;
                case SDL_JOYBUTTONUP:   handle_joy_btn(false); break;
                case SDL_WINDOWEVENT:   handle_win_ev();       break;
            }
        }
    }

    void swap_joysticks() {
        Sig_key* t = joy_handler[0];
        joy_handler[0] = joy_handler[1];
        joy_handler[1] = t;
    }

    Input(Handlers& handlers_)
      : handlers(handlers_), joy_handler{ &handlers_.joy1, &handlers_.joy2 }
    {
        // find index of left/right shift
        while (KC_LU_TBL[sh_l_idx] != Key_code::Keyboard::sh_l) ++sh_l_idx;
        while (KC_LU_TBL[sh_r_idx] != Key_code::Keyboard::sh_r) ++sh_r_idx;

        // TODO: allow selection (for now just the first two found are attached)
        // TODO: joystick calibration/configuration...
        int open_joys = 0;
        for (int j = 0; j < SDL_NumJoysticks() && open_joys < 2; ++j) {
            SDL_Joystick* sj = SDL_JoystickOpen(j);
            if(!sj) SDL_Log("Unable to SDL_JoystickOpen: %s", SDL_GetError());
            else {
                sdl_joystick_id[open_joys] = SDL_JoystickInstanceID(sj);
                sdl_joystick[open_joys++] = sj;
            }
        }
        SDL_Log("%d joysticks attached", open_joys);
    }

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

    u8 translate_sdl_key() {
        static const i32 MAX_KC             = SDLK_SLEEP;
        static const i32 LAST_CHAR_KC       = SDLK_DELETE;
        static const i32 FIRST_NON_CHAR_KC  = SDLK_CAPSLOCK;
        static const i32 OFFSET_NON_CHAR_KC = FIRST_NON_CHAR_KC - (LAST_CHAR_KC + 1);
        /*
        std::cout << "\nkc: " << sdl_ev.key.keysym.sym << " ";
        std::cout << "sc: " << sdl_ev.key.keysym.scancode << " ";
        */
        SDL_Keysym key_sym = sdl_ev.key.keysym;

        if (key_sym.sym <= MAX_KC) {
            // most mapped on keycode, some on scancode
            if (key_sym.sym <= LAST_CHAR_KC)
                return KC_LU_TBL[key_sym.sym];
            else if (key_sym.sym >= FIRST_NON_CHAR_KC)
                return KC_LU_TBL[key_sym.sym - OFFSET_NON_CHAR_KC];
            else if (key_sym.scancode >= 0x2e && key_sym.scancode <= 0x35)
                return SC_LU_TBL[key_sym.scancode - 0x2e];
        }

        return Key_code::System::nop;
    }

    void output_key(u8 code, u8 down) {
        auto group = code >> 6;
        switch (group) {
            case Key_code::Group::keyboard:
                handlers.keyboard(code, down);
                break;
            case Key_code::Group::keyboard_ext: {
                // NOTE: Currently 'ext' contains only 'auto shifted' type of keys.
                //       This code might break if different type of keys get added.
                if (code == Key_code::Keyboard_ext::s_lck) {
                    set_shift_lock();
                } else {
                    static const u8 KC_OFFSET
                        = Key_code::Keyboard_ext::crs_l - Key_code::Keyboard::crs_r;
                    // 6 of these are actually used (and probably there will not be more)
                    static const u8 BIT[] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 };

                    if (down) {
                        if (sh_r_down == 0x00) {
                            // first 'auto shifted' key held down
                            //   --> set 'left shift' key down & disable the 'real' one
                            handlers.keyboard(Key_code::Keyboard::sh_r, true);
                            disable_sh_r();
                        }
                        sh_r_down |= BIT[code & 0x7]; // keep track of key presses
                    } else {
                        sh_r_down &= (~BIT[code & 0x7]);
                        if (sh_r_down == 0x00) {
                            // last 'auto shifted' key released
                            //   --> set 'left shift' key up & enable the 'real' one
                            handlers.keyboard(Key_code::Keyboard::sh_r, false);
                            enable_sh_r();
                        }
                    }

                    handlers.keyboard(code - KC_OFFSET, down); // send the corresponding 'real' code
                }

                break;
            }
            case Key_code::Group::joystick: {
                auto id = code & JOY_ID_BIT ? 1 : 0;
                (*joy_handler[id])(code & 0x7, down);
                break;
            }
            case Key_code::Group::system:
                handlers.sys(code, down);
                break;
        }
    }

    void handle_joy_axis() {
        /*  SDL Wiki: "On most modern joysticks the X axis is usually represented by axis 0
        and the Y axis by axis 1. The value returned by SDL_JoystickGetAxis()
        is a signed integer (-32768 to 32767) representing the current position
        of the axis. It may be necessary to impose certain tolerances on these
        values to account for jitter. */ // ==> TODO (config/calib)

        static const int X_AXIS = 0;

        SDL_JoyAxisEvent& joy_ev = sdl_ev.jaxis;
        Sig_key& output = get_joy_output(joy_ev.which);

        using Key_code::Joystick;

        if (joy_ev.axis == X_AXIS) {
            if (joy_ev.value < 0) output(Joystick::jl, true);
            else if (joy_ev.value > 0) output(Joystick::jr, true);
            else {
                output(Joystick::jl, false);
                output(Joystick::jr, false);
            }
        } else {
            if (joy_ev.value < 0) output(Joystick::ju, true);
            else if (joy_ev.value > 0) output(Joystick::jd, true);
            else {
                output(Joystick::ju, false);
                output(Joystick::jd, false);
            }
        }
    }

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
            {Mode::win,  Mode::fullscr_win, Mode::fullscr },
            {"WINDOWED", "FULLSCREEN",      "TRUE FULLSCREEN" },
        };
        // TODO: fullscreen_scale?
        Param<double> window_scale{4.00, 0.5, 8.00, 0.05}; // init, min, max, step
        Param<double> aspect_ratio{0.89, 0.5, 1.25, 0.005}; // PAL actual: ~0.935 

        Param<u8> sharpness{0, 0, 3, 1};

        Param<u8> brightness{78, 0, 100, 1};
        Param<u8> contrast {100, 0, 100, 1};
        Param<u8> saturation{68, 0, 100, 1};

        Param<u8> filter_pattern{7, 0, 254, 1}; // TODO: actual max
        Param<u8> filter_level {11, 0,  15, 1}; // 0 --> all pass
    };

    Video_out() : frame(set), filter(set),
        _menu_items{
            //name                     connected setting   notify
            {"VIDEO / MODE",           set.mode,           std::bind(&Video_out::upd_mode, this)},
            {"VIDEO / WINDOW SCALE",   set.window_scale,   std::bind(&Video_out::upd_dimensions, this)},
            {"VIDEO / ASPECT RATIO",   set.aspect_ratio,   std::bind(&Video_out::upd_dimensions, this)},
            {"VIDEO / SHARPNESS",      set.sharpness,      std::bind(&Frame::upd_sharpness, &frame)},
            {"VIDEO / BRIGHTNESS",     set.brightness,     std::bind(&Frame::upd_palette, &frame)},
            {"VIDEO / CONTRAST",       set.contrast,       std::bind(&Frame::upd_palette, &frame)},
            {"VIDEO / SATURATION",     set.saturation,     std::bind(&Frame::upd_palette, &frame)},
            {"VIDEO / FILTER PATTERN", set.filter_pattern, std::bind(&Filter::upd, &filter)},
            {"VIDEO / FILTER LEVEL",   set.filter_level,   std::bind(&Filter::upd, &filter)},
        }
    {}

    ~Video_out();

    void put(const u8* vic_frame) {
        //SDL_RenderClear(renderer);
        frame.draw(vic_frame);
        frame.frame.copy(renderer, dstrect);
        filter.frame.copy(renderer, dstrect);
        flip();
    }

    void flip() { SDL_RenderPresent(renderer); }

    void toggle_fullscr_win() { // TODO: cycle through presets instead --> TODO: presets...
        set.mode = (set.mode == Mode::win) ? Mode::fullscr_win : Mode::win;
        upd_mode();
    }

    bool v_synced() const { return sdl_mode.refresh_rate == FRAME_RATE; }

    Menu::Group settings_menu();

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
        u32* pixels = nullptr;

        SDL_frame(int max_w_, int max_h_, SDL_TextureAccess ta_, SDL_BlendMode bm_);
        ~SDL_frame();

        void connect(SDL_Renderer* renderer);
        void copy(SDL_Renderer* r, const SDL_Rect* dstrect) {
            SDL_RenderCopy(r, texture, &srcrect, dstrect);
        }
    };

    struct Frame {
        static constexpr int max_w = VIC_II::FRAME_WIDTH * 3; // TODO: non-hardcoded
        static constexpr int max_h = VIC_II::FRAME_HEIGHT * 3; // TODO: non-hardcoded

        Frame(const Settings& set_) : set(set_) {}

        void upd_palette();
        void upd_sharpness();

        u32 col(const u8 vic_pixel) { return palette[vic_pixel & 0xf]; } // TODO: do '&' here? (or in VIC?)}

        std::function<void (const u8* )> draw;

        const Settings& set;
        u32 palette[16];
        SDL_frame frame{max_w, max_h, SDL_TEXTUREACCESS_STREAMING, SDL_BLENDMODE_NONE};
    };

    struct Filter {
        static constexpr int max_w = VIC_II::FRAME_WIDTH * 4; // TODO: non-hardcoded
        static constexpr int max_h = VIC_II::FRAME_HEIGHT * 3; // TODO: non-hardcoded

        struct Pattern {
            using Shape = std::vector<std::vector<u32>>;

            const u8 px_w;
            const u8 px_h;

            const Shape shape;
        };

        static const std::vector<Pattern> patterns;

        Filter(const Settings& set_) : set(set_) {}

        void upd();

        const Settings& set;
        SDL_frame frame{max_w, max_h, SDL_TEXTUREACCESS_STATIC, SDL_BLENDMODE_MOD};
    };

    void upd_mode();
    void upd_dimensions();

    Settings set;

    SDL_DisplayMode sdl_mode = { 0, 0, 0, 0, 0 };

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;

    Frame frame;
    Filter filter;

    SDL_Rect fullscreen_dstrect = {0, 0, 0, 0};
    SDL_Rect* dstrect = nullptr;

    std::vector<Menu::Knob> _menu_items;
};


// TODO: error handling (e.g. on error just run without audio?)
class Audio_out {
public:
    Audio_out();
    ~Audio_out();

    void put(const i16* chunk, u32 sz) { SDL_QueueAudio(dev, chunk, 2 * sz); }

    // u32 queued() const { return SDL_GetQueuedAudioSize(dev) * 2; }

    void flush() { SDL_ClearQueuedAudio(dev); }

private:
    SDL_AudioDeviceID dev;

};


class _SDL { // classic...
public:
    static _SDL& instance() { static _SDL _sdl; return _sdl; }
    ~_SDL() { if (init) SDL_Quit(); }
private:
    _SDL() {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO |  SDL_INIT_JOYSTICK) != 0) {
            SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
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
