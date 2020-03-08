#ifndef HOST_H_INCLUDED
#define HOST_H_INCLUDED

#include <SDL.h>
#include "common.h"
#include "utils.h"


namespace Host {


extern u8 KC_LU_TBL[];
extern const u8 SC_LU_TBL[];


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
    void put_frame(u8* vic_frame) {
        u32* scan_1 = frame;
        u32* scan_2;

        for (int y = 0; y < frame_height; ++y) {
            scan_2 = scan_1 + (2 * frame_width);
            u32 s1c1; // 'scan_1_color_1'...
            u32 s1c2 = 0;
            u32 s2c1;
            u32 s2c2 = 0;
            for (int x = 0; x < frame_width; ++x) {
                u8 vic_col = *vic_frame++;
                auto p = (x /*>> 1*/) & 0x1;

                // some averaging for a simple blur effect....
                s1c1 = palette[vic_col][0 + p];
                s1c2 = (((s1c1 & 0xfefefe) + (s1c2 & 0xfefefe)) / 2) & 0xfefefe;
                *scan_1++ = s1c2;
                s1c2 = (((s1c1 & 0xfefefe) + (s1c2 & 0xfefefe)) / 2) & 0xfefefe;
                *scan_1++ = s1c2;
                s1c2 = s1c1;

                s2c1 = palette[vic_col][2 + (1 - p)];
                s2c2 = (((s2c1 & 0xfefefe) + (s2c2 & 0xfefefe)) / 2) & 0xfefefe;
                *scan_2++ = s2c2;
                s2c2 = (((s2c1 & 0xfefefe) + (s2c2 & 0xfefefe)) / 2) & 0xfefefe;
                *scan_2++ = s2c2;
                s2c2 = s2c1;
            }
            scan_1 = scan_2;
        }

        SDL_UpdateTexture(texture, 0, (void*)frame, 2 * (4 * frame_width));
        SDL_RenderCopy(renderer, texture, nullptr, render_dstrect);
        SDL_RenderPresent(renderer);
    }

    void adjust_scale(i8 amount) { set_scale(scale + amount); }

    void set_scale(i8 new_scale) { // TODO: fullscreen scaling
        if (mode != Mode::window) return;

        if (new_scale >= min_scale && new_scale <= max_scale) {
            scale = new_scale;
            int w = aspect_ratio * (scale / 10.0) * frame_width;
            int h = (scale / 10.0) * frame_height;
            SDL_SetWindowSize(window, w, h);
        }
    }

    void toggle_fullscreen() { set_mode(Mode::fullscreen); }
    void toggle_windowed() {
        if (mode != Mode::window) set_mode(Mode::window);
        else set_mode(Mode::fullscreen_window);
    }

    bool v_synced() const { return mode == Mode::fullscreen; }

    Video_out(u16 frame_width_, u16 frame_height_) // TODO: error handling
            : frame_width(frame_width_), frame_height(frame_height_)
    {
        window = SDL_CreateWindow("The display...",
            400, 100,
            aspect_ratio * frame_width, frame_height, 0
        );
        if (!window) {
            SDL_Log("Failed to SDL_CreateWindow: %s", SDL_GetError());
            exit(1);
        }

        frame = new u32[(2 * frame_width) * (2 * frame_height)];

        for (int p = 0; p < 4; ++p) {
            static constexpr double palette_conf[][3] = {
                {68, 100, 75}, {65, 100, 73},
                {42, 80,  45}, {39, 80,  45},
            };
            u32 colodore[16];
            get_Colodore(colodore, palette_conf[p][0], palette_conf[p][1], palette_conf[p][2]);
            colodore[0] = 0x020202; // black is black...
            for (int c = 0; c < 16; ++c)
                palette[c][p] = colodore[c];
        }

        set_mode(Mode::window);
        set_scale(35);
    }

    ~Video_out() {
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);

        delete[] frame;
    };

private:
    enum class Mode { window, fullscreen_window, fullscreen, none };

    void set_mode(Mode new_mode) {
        if (new_mode == mode) return;

        auto re_config = [&](int flags = 0) {
            if (texture) SDL_DestroyTexture(texture);
            if (renderer) SDL_DestroyRenderer(renderer);

            renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | flags);
            if (!renderer) {
                SDL_Log("Failed to SDL_CreateRenderer: %s", SDL_GetError());
                exit(1);
            }
            SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2");
            texture = SDL_CreateTexture(renderer,
                        SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
                        2 * frame_width, 2 * frame_height);
            if (!texture) {
                SDL_Log("Failed to SDL_CreateTexture: %s", SDL_GetError());
                exit(1);
            }
            if (SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_NONE) != 0) {
                SDL_Log("Failed to SDL_SetTextureBlendMode: %s", SDL_GetError());
            }
        };

        auto fullscreen_dstrect = [&]() {
            int win_w;
            int win_h;
            SDL_GetWindowSize(window, &win_w, &win_h);
            int w = aspect_ratio * (((double)win_h / frame_height) * frame_width);
            if (w < win_w) {
                render_dstrect_fullscreen.x = (win_w - w) / 2;
                render_dstrect_fullscreen.y = 0;
                render_dstrect_fullscreen.w = w;
                render_dstrect_fullscreen.h = win_h;
            } else {
                int h =  (((double)win_w / frame_width) * frame_height) / aspect_ratio;
                render_dstrect_fullscreen.x = 0;
                render_dstrect_fullscreen.y = (win_h - h) / 2;
                render_dstrect_fullscreen.w = win_w;
                render_dstrect_fullscreen.h = h;
            }
            render_dstrect = &render_dstrect_fullscreen;
        };

        switch (new_mode) {
            case Mode::window:
                if (SDL_SetWindowFullscreen(window, 0) != 0) {
                    SDL_Log("Failed to SDL_SetWindowFullscreen: %s", SDL_GetError());
                    return;
                }
                if (mode >= Mode::fullscreen) re_config();
                render_dstrect = nullptr;
                break;
            case Mode::fullscreen_window:
                if (SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP) != 0) {
                    SDL_Log("Failed to SDL_SetWindowFullscreen: %s", SDL_GetError());
                    return;
                }
                if (mode >= Mode::fullscreen) re_config();
                fullscreen_dstrect();
                break;
            case Mode::fullscreen: { // TODO: cycle through supported modes?
                SDL_DisplayMode sdm = { SDL_PIXELFORMAT_ARGB8888, 1920, 1080, 50, 0 };
                if (SDL_SetWindowDisplayMode(window, &sdm) != 0) {
                    SDL_Log("Failed to SDL_SetWindowDisplayMode: %s", SDL_GetError());
                    return;
                }
                if (SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN) != 0) {
                    SDL_Log("Failed to SDL_SetWindowFullscreen: %s", SDL_GetError());
                    return;
                }
                re_config(SDL_RENDERER_PRESENTVSYNC);
                fullscreen_dstrect();
                break;
            }
            case Mode::none: exit(1); return;
        }

        SDL_ShowCursor(new_mode == Mode::window);

        mode = new_mode;
    }

    Mode mode = Mode::none;

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* texture = nullptr;

    SDL_Rect* render_dstrect = nullptr;
    SDL_Rect render_dstrect_fullscreen;

    const u16 frame_width;
    const u16 frame_height;

    u32* frame;

    u32 palette[16][4];

    // TODO: tweakable?
    const double aspect_ratio = 0.936;
    i8 scale;
    const i8 min_scale = 5;
    const i8 max_scale = 80;

};


// TODO: error handling (e.g. on error just run without audio?)
class Audio_out {
public:
    void put(const i16* chunk, u32 sz) { SDL_QueueAudio(dev, chunk, 2 * sz); }

    // u32 queued() const { return SDL_GetQueuedAudioSize(dev) * 2; }

    void flush() { SDL_ClearQueuedAudio(dev); }

    Audio_out() {
        SDL_AudioSpec want;
        SDL_AudioSpec have;

        SDL_memset(&want, 0, sizeof(want));
        want.freq = 44100;
        want.format = AUDIO_S16LSB;
        want.channels = 1;
        want.samples = 1024;

        dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
        if (dev == 0) {
            SDL_Log("Unable to SDL_OpenAudioDevice: %s", SDL_GetError());
            exit(1);
        }
        //std::cout << (int)have.freq << " " << (int)have.channels << " ";
        //std::cout << (int)want.format << " " << (int)have.format;

        SDL_PauseAudioDevice(dev, 0);
    }

    ~Audio_out() { if (dev) SDL_CloseAudioDevice(dev); }

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
