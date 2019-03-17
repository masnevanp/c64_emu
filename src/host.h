#ifndef HOST_H_INCLUDED
#define HOST_H_INCLUDED

#include <SDL.h>
#include "common.h"
#include "utils.h"


namespace Host {


extern const u8 KC_LU_TBL[];
extern const u8 SC_LU_TBL[];


// TODO: paddles/lightpen
class Input {
public:
    struct Handlers {
        Sig_key client_key;
        Sig_key restore_key;
        Sig_key host_key;
        Sig_joy joy1;
        Sig_joy joy2;
    };

    void poll() { // TODO: filtering?
        while (SDL_PollEvent(&sdl_ev)) {
            switch (sdl_ev.type) {
                case SDL_KEYDOWN:       handle_key(true);      break;
                case SDL_KEYUP:         handle_key(false);     break;
                case SDL_JOYAXISMOTION: handle_joy_axis();     break;
                case SDL_JOYBUTTONDOWN: handle_joy_btn(true);  break;
                case SDL_JOYBUTTONUP:   handle_joy_btn(false); break;
            }
        }
    }

    void swap_joysticks() {
        Sig_joy* t = joy_handler[0];
        joy_handler[0] = joy_handler[1];
        joy_handler[1] = t;
    }

    Input(Handlers& handlers_)
      : handlers(handlers_), joy_handler{ &handlers_.joy1, &handlers_.joy2 }
    {
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
    Handlers& handlers;

    Sig_joy* joy_handler[2];

    SDL_Joystick* sdl_joystick[2] = { nullptr, nullptr }; // TODO: struct these togeteher
    SDL_JoystickID sdl_joystick_id[2];

    SDL_Event sdl_ev;

    void handle_key(bool key_down) {
        static const i32 MAX_KC             = 0x4000011a;
        static const i32 LAST_CHAR_KC       = 0x7f;
        static const i32 FIRST_NON_CHAR_KC  = 0x40000039;
        static const i32 OFFSET_NON_CHAR_KC = 0x3fffffb9;

        Key_event key_ev{Key_event::KC::none, key_down};
        SDL_Keysym key_sym = sdl_ev.key.keysym;

        /*if (key_down) {
            std::cout << "\nkc: " << sdl_ev.key.keysym.sym << " ";
            std::cout << "sc: " << sdl_ev.key.keysym.scancode << " ";
        }*/

        if (key_sym.sym <= MAX_KC) {
            // most mapped on keycode, some on scancode
            if (key_sym.sym <= LAST_CHAR_KC)
                key_ev.code = KC_LU_TBL[key_sym.sym];
            else if (key_sym.sym >= FIRST_NON_CHAR_KC)
                key_ev.code = KC_LU_TBL[key_sym.sym - OFFSET_NON_CHAR_KC];
            else if (key_sym.scancode >= 0x2e && key_sym.scancode <= 0x35)
                key_ev.code = SC_LU_TBL[key_sym.scancode - 0x2e];

            if (key_ev.code != Key_event::KC::none) {
                if (key_ev.code >= Key_event::FIRST_JOY_KC) {
                    Joystick_event joy_ev {
                        (u8)((key_ev.code - Key_event::FIRST_JOY_KC) % 5), // code (direction/btn)
                        key_ev.down // active
                    };
                    if (key_ev.code <= Key_event::j1_b) (*joy_handler[0])(joy_ev);
                    else (*joy_handler[1])(joy_ev);
                }
                else if (key_ev.code >= Key_event::FIRST_HOST_KC) handlers.host_key(key_ev);
                else if (key_ev.code == Key_event::KC::rstre) handlers.restore_key(key_ev);
                else handlers.client_key(key_ev);
            }
        }
    }

    void handle_joy_axis() {
        static const int X_AXIS = 0;

        SDL_JoyAxisEvent& joy_ev = sdl_ev.jaxis;
        int joy_idx = (joy_ev.which == sdl_joystick_id[0]) ? 0 : 1;
        Sig_joy& handler = *(joy_handler[joy_idx]);

        if (joy_ev.axis == X_AXIS) {
            if (joy_ev.value < 0) handler({Joystick_event::left, true});
            else if (joy_ev.value > 0) handler({Joystick_event::right, true});
            else {
                handler({Joystick_event::left, false});
                handler({Joystick_event::right, false});
            }
        } else {
            if (joy_ev.value < 0) handler({Joystick_event::up, true});
            else if (joy_ev.value > 0) handler({Joystick_event::down, true});
            else {
                handler({Joystick_event::up, false});
                handler({Joystick_event::down, false});
            }
        }
    }

    void handle_joy_btn(bool btn_down) {
        SDL_JoyButtonEvent& joy_ev = sdl_ev.jbutton;
        int joy_idx = (joy_ev.which == sdl_joystick_id[0]) ? 0 : 1;
        Sig_joy& handler = *(joy_handler[joy_idx]);
        handler({Joystick_event::btn, btn_down});
    }
};


class Video_out {
public:
    void put(u8* line) {
        for (int p = 0; p < frame_width; ++p) {
            auto col_idx = line[p];
            *beam_1_pos++ = palette_1[col_idx];
            *beam_2_pos++ = palette_2[col_idx];
        }
        beam_1_pos = beam_2_pos;
        beam_2_pos = beam_1_pos + frame_width;
    }

    void frame_done() {
        SDL_UnlockTexture(texture);
        //SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);
        void* pixels;
        int pitch;
        SDL_LockTexture(texture, nullptr, &pixels, &pitch);
        beam_1_pos = (u32*)pixels;
        beam_2_pos = beam_1_pos + frame_width;
    }

    void frame_skip() {
        SDL_UnlockTexture(texture);
        void* pixels;
        int pitch;
        SDL_LockTexture(texture, nullptr, &pixels, &pitch);
        beam_1_pos = (u32*)pixels;
        beam_2_pos = beam_1_pos + frame_width;
    }

    Video_out(u16 frame_width_, u16 frame_height) : frame_width(frame_width_) { // TODO: error handling
        // TODO: bind these to host key(s)
        static const double aspect_ratio = 0.936;
        static const int scale = 3;

        u16 view_width = aspect_ratio * scale * frame_width;
        u16 view_height = scale * frame_height;

        window = SDL_CreateWindow("The display...",
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, view_width, view_height, SDL_WINDOW_RESIZABLE);
        if (!window) {
            SDL_Log("Failed to SDL_CreateWindow: %s", SDL_GetError());
            exit(1);
        }

        renderer = SDL_CreateRenderer(window, -1,
                        SDL_RENDERER_ACCELERATED /*| SDL_RENDERER_PRESENTVSYNC*/);
        if (!renderer) {
            SDL_Log("Failed to SDL_CreateRenderer: %s", SDL_GetError());
            exit(1);
        }

        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
        texture = SDL_CreateTexture(renderer,
                        SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
                        frame_width, 2 * frame_height);
        if (!texture) {
            SDL_Log("Failed to SDL_CreateTexture: %s", SDL_GetError());
            exit(1);
        }

        if (SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_NONE) != 0) {
            SDL_Log("Failed to SDL_SetTextureBlendMode: %s", SDL_GetError());
            // exit(1);
        }

        get_Colodore(palette_1, 60, 100, 75);
        get_Colodore(palette_2, 40, 80, 50);

        reset();
    }

    ~Video_out() {
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
    };

private:
    void reset() {
        void* pixels;
        int pitch;
        SDL_LockTexture(texture, nullptr, &pixels, &pitch);
        frame_done();
    }

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* texture = nullptr;

    u32* beam_1_pos;
    u32* beam_2_pos;

    u32 palette_1[16];
    u32 palette_2[16];

    u16 frame_width;
};


// TODO: error handling (e.g. on error just run without audio?)
class Audio_out {
public:
    void put(const i16* chunk, u32 sz) { SDL_QueueAudio(dev, chunk, 2 * sz); }

    void flush() { SDL_ClearQueuedAudio(dev); }

    Audio_out() {
        SDL_memset(&want, 0, sizeof(want));
        want.freq = 44100;
        want.format = AUDIO_S16LSB;
        want.channels = 1;

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
    SDL_AudioSpec want;
    SDL_AudioSpec have;
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
