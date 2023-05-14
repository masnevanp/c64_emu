
#include <array>
#include <map>
#include "host.h"



using namespace Host;

using kb = Key_code::Keyboard;
using ke = Key_code::Keyboard_ext;
using js = Key_code::Joystick;
using sy = Key_code::System;


static const u8 J1 = 0x00;
static const u8 J2 = Input::JOY_ID_BIT;


// TODO: analyze... use more scancodes?
u8 Input::KC_LU_TBL[] = {
    /* 00..7f : SDL_Keycode 00..7f  --> Key_code */
    // 00..0f
    sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,
    kb::del,   kb::ar_l,  sy::nop,   sy::nop,   sy::nop,   kb::ret,   sy::nop,   sy::nop,
    // 10..1f
    sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,    sy::nop,
    sy::nop,   sy::nop,   sy::nop,   kb::r_stp, sy::nop,   sy::nop,   sy::nop,    sy::nop,
    // 20..2f
    kb::space, sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,    kb::eq,
    sy::nop,   sy::nop,   sy::nop,   kb::plus,  kb::comma, kb::div,   kb::dot,    sy::nop,
    // 30..3f
    kb::num_0, kb::num_1, kb::num_2, kb::num_3, kb::num_4, kb::num_5, kb::num_6,  kb::num_7,
    kb::num_8, kb::num_9, sy::nop,   sy::nop,   kb::ar_l,  sy::nop,   sy::nop,    sy::nop,
    // 40..4f
    sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,    sy::nop,
    sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,    sy::nop,
    // 50..5f
    sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,    sy::nop,
    sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,    sy::nop,
    // 60..6f
    sy::nop,   kb::a,     kb::b,     kb::c,     kb::d,     kb::e,     kb::f,      kb::g,
    kb::h,     kb::i,     kb::j,     kb::k,     kb::l,     kb::m,     kb::n,      kb::o,
    // 70..7f
    kb::p,     kb::q,     kb::r,     kb::s,     kb::t,     kb::u,     kb::v,      kb::w,
    kb::x,     kb::y,     kb::z,     sy::nop,   sy::nop,   sy::nop,   sy::nop,    kb::del,

    /* 80..   : SDL_Keycode 40000039.. --> Key_code (80 = kc 40000039, 81 = kc 4000003a, ...) */
    // 80..8f
    ke::s_lck, kb::f1,    ke::f2,    kb::f3,    ke::f4,    kb::f5,    ke::f6,     kb::f7,
    ke::f8,    sy::rst_c, sy::swp_j, sy::v_fsc, sy::menu_tgl,sy::nop, sy::nop, sy::rstre,
    // 90..9f
    sy::rot_dsk,kb::home, sy::menu_up, sy::nop, sy::menu_ent, sy::menu_down, kb::crs_r, ke::crs_l,
    kb::crs_d, ke::crs_u, sy::nop,   J1|js::ju, kb::mul,   kb::minus, kb::plus,   kb::ret,
    // a0..af
    J2|js::jl, J2|js::jd, J2|js::jr, J1|js::jb, J2|js::ju, sy::nop,   J1|js::jl,  J1|js::jd,
    J1|js::jr, J2|js::jb, sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,    sy::nop,
    // b0..bf
    sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,    sy::nop,
    sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,    sy::nop,
    // c0..cf
    sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,    sy::nop,
    sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,    sy::nop,
    // d0..df
    sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,    sy::nop,
    sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,    sy::nop,
    // e0..ef
    sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,    sy::nop,
    sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,    sy::nop,
    // f0..ff
    sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,    sy::nop,
    sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,    sy::nop,
    // 100..10f
    sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,    sy::nop,
    sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,    sy::nop,
    // 110..11f
    sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,    sy::nop,
    sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,    sy::nop,
    // 120..12f
    sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,    kb::ctrl,
    kb::sh_l,  kb::cmdre, sy::nop,   kb::ctrl,  kb::sh_r,  sy::nop,   sy::nop,    sy::nop,
    // 130..13f
    sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,    sy::nop,
    sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,    sy::nop,
};


const u8 Input::SC_LU_TBL[] = {
    /* SDL_Scancode 2e..35 --> Key_code */
    kb::minus, kb::at,    kb::ar_up, sy::nop,   sy::nop,   kb::colon, kb::s_col, kb::pound,
};


Input::Input(Handlers& handlers_)
    : handlers(handlers_), joy_handler{ &handlers_.controller_1, &handlers_.controller_2 }
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


void Input::poll() { // TODO: filtering?
    while (SDL_PollEvent(&sdl_ev)) {
        switch (sdl_ev.type) {
            case SDL_KEYDOWN:       handle_key(true);      break;
            case SDL_KEYUP:         handle_key(false);     break;
            case SDL_JOYAXISMOTION: handle_joy_axis();     break;
            case SDL_JOYBUTTONDOWN: handle_joy_btn(true);  break;
            case SDL_JOYBUTTONUP:   handle_joy_btn(false); break;
            case SDL_WINDOWEVENT:   handle_win_ev();       break;
            case SDL_DROPFILE:      handle_dropfile();     break;
        }
    }
}


void Input::swap_joysticks() {
    Sig_key* t = joy_handler[0];
    joy_handler[0] = joy_handler[1];
    joy_handler[1] = t;
}


u8 Input::translate_sdl_key() {
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


void Input::output_key(u8 code, u8 down) {
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


void Input::handle_joy_axis() {
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



Video_out::SDL_frame::SDL_frame(int max_w_, int max_h_, SDL_TextureAccess ta_, SDL_BlendMode bm_)
    : max_w(max_w_), max_h(max_h_), ta(ta_), bm(bm_), pixels(new u32[max_w * max_h])
{
}


Video_out::SDL_frame::~SDL_frame() {
    if (texture) SDL_DestroyTexture(texture);
    delete[] pixels;
}


void Video_out::SDL_frame::connect(SDL_Renderer* renderer) {
    if (texture) SDL_DestroyTexture(texture);
    texture = create_texture(renderer, ta, bm, max_w, max_h);
}


void Video_out::Frame::upd_palette(const Settings& set) {
    get_Colodore(palette, set.brightness, set.contrast, set.saturation);
    palette[0] = 0x020202; // keep it blackish...
}


void Video_out::Frame::upd_sharpness(const Settings& set) {
    struct Sz { const u8 w; const u8 h; };

    static constexpr std::array<Sz, 5> px_sz{{ {1, 1}, {1, 2}, {2, 2}, {3, 3} }};

    frame.srcrect.w = VIC_II::FRAME_WIDTH * px_sz[set.sharpness].w;
    frame.srcrect.h = VIC_II::FRAME_HEIGHT * px_sz[set.sharpness].h;

    switch (set.sharpness) {
        case 0: // {1, 1}
            draw = [&](const u8* vic_frame) {
                for (int pos = 0; pos < VIC_II::FRAME_SIZE; ++pos) {
                    frame.pixels[pos] = col(vic_frame[pos]);
                }
                const auto bytes_per_row = 1 * (bytes_per_pixel * VIC_II::FRAME_WIDTH);
                SDL_UpdateTexture(frame.texture, 0, (void*)frame.pixels, bytes_per_row);
            };
            break;
        case 1: // {1, 2}
            draw = [&](const u8* vic_frame) {
                u32* scan = frame.pixels;

                for (int y = 0; y < VIC_II::FRAME_HEIGHT; ++y) {
                    for (int x = 0; x < VIC_II::FRAME_WIDTH; ++x, ++scan) {
                        const auto c = col(*vic_frame++);
                        *scan = c;
                        *(scan + VIC_II::FRAME_WIDTH) = c;
                    }
                    scan += VIC_II::FRAME_WIDTH;
                }

                const auto bytes_per_row = 1 * (bytes_per_pixel * VIC_II::FRAME_WIDTH);
                SDL_UpdateTexture(frame.texture, 0, (void*)frame.pixels, bytes_per_row);
            };
            break;
        case 2: // {2, 2}
            draw = [&](const u8* vic_frame) {
                u32* scan = frame.pixels;

                for (int y = 0; y < VIC_II::FRAME_HEIGHT; ++y) {
                    for (int x = 0; x < VIC_II::FRAME_WIDTH; ++x) {
                        const auto c = col(vic_frame[x]);
                        *scan++ = c;
                        *scan++ = c;
                    }
                    for (int x = 0; x < VIC_II::FRAME_WIDTH; ++x) {
                        const auto c = col(vic_frame[x]);
                        *scan++ = c;
                        *scan++ = c;
                    }
                    vic_frame += VIC_II::FRAME_WIDTH;
                }

                const auto bytes_per_row = 2 * (bytes_per_pixel * VIC_II::FRAME_WIDTH);
                SDL_UpdateTexture(frame.texture, 0, (void*)frame.pixels, bytes_per_row);
            };
            break;
        case 3: // {3, 3}
            draw = [&](const u8* vic_frame) {
                u32* scan = frame.pixels;

                auto draw_row = [&]() {
                    for (int x = 0; x < VIC_II::FRAME_WIDTH; ++x) {
                        const auto c = col(vic_frame[x]);
                        *scan++ = c;
                        *scan++ = c;
                        *scan++ = c;
                    }
                };

                for (int y = 0; y < VIC_II::FRAME_HEIGHT; ++y) {
                    draw_row();
                    draw_row();
                    draw_row();
                    vic_frame += VIC_II::FRAME_WIDTH;
                }

                const auto bytes_per_row = 3 * (bytes_per_pixel * VIC_II::FRAME_WIDTH);
                SDL_UpdateTexture(frame.texture, 0, (void*)frame.pixels, bytes_per_row);
            };
            break;
    }
}


const std::vector<Video_out::Mask::Pattern> Video_out::Mask::patterns = {
    {
        //{ 0x000000, 0x000000, 0x050505, },
        { 0x000000, 0x000000, 0x000000, },
        { 0x000000, 0x000000, 0x0a0a0a, },
        { 0x0a0a0a, 0x0a0a0a, 0x111111, },
        { 0x050505, 0x050505, 0x0a0a0a, },
    },
    {
        { 0x000000, 0x050505, 0x111111, 0x000000 },
        { 0x000000, 0x050505, 0x111111, 0x000000 },
        { 0x050505, 0x050505, 0x111111, 0x050505 },
    },
    {
        { 0x000000, 0x111111, 0x000000, },
        { 0x000000, 0x111111, 0x000000, },
        { 0x0a0a0a, 0x0a0a0a, 0x0a0a0a, },
        { 0x000000, 0x111111, 0x000000, },
    },
    {
        { 0x050505, 0x111111, 0x050505, 0x050505, 0x050505, 0x111111, 0x000000, 0x000000 },
        { 0x050505, 0x111111, 0x000000, 0x000000, 0x050505, 0x111111, 0x050505, 0x050505 },
        { 0x050505, 0x111111, 0x000000, 0x000000, 0x050505, 0x111111, 0x000000, 0x000000 },
    },
    {
        { 0x000000, 0x111111, 0x000000, 0x000000 },
        { 0x000000, 0x111111, 0x000000, 0x000000 },
        { 0x050505, 0x0a0a0a, 0x050505, 0x050505 },
    },
};


void Video_out::Mask::upd(const Settings& set) {
    static constexpr u32 all_pass = 0xffffffff;

    const auto& pattern = patterns[set.mask_pattern % patterns.size()];

    auto p = frame.pixels;
    const auto row_cnt = pattern.size();
    for (int y = 0; y < max_h; ++y) {
        const auto& row = pattern[y % row_cnt];
        const auto col_cnt = row.size();
        for (int x = 0; x < max_w; ++x) {
            *p++ = all_pass - (set.mask_level * row[x % col_cnt]);
        }
    }

    const auto bytes_per_row = max_w * bytes_per_pixel;
    SDL_UpdateTexture(frame.texture, nullptr, (void*)frame.pixels, bytes_per_row);
}



Video_out::~Video_out() {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
}


SDL_Texture* Video_out::create_texture(SDL_Renderer* r, SDL_TextureAccess ta, SDL_BlendMode bm, int w, int h)
{
    SDL_Texture* t = SDL_CreateTexture(r, Video_out::pixel_format, ta, w, h);
    if (!t) {
        SDL_Log("Failed to SDL_CreateTexture: %s", SDL_GetError());
        exit(1);
    }

    if (SDL_SetTextureBlendMode(t, bm) != 0) {
        SDL_Log("Failed to SDL_SetTextureBlendMode: %s", SDL_GetError());
    }

    return t;
}


void Video_out::upd_mode() {
    if (!window) {
        window = SDL_CreateWindow("WIP #$40", 400, 100, 100, 100, 0);
        if (!window) {
            SDL_Log("Failed to SDL_CreateWindow: %s", SDL_GetError());
            exit(1);
        }
    }

    switch (set.mode) {
        case Mode::win:
            if (SDL_SetWindowFullscreen(window, 0) != 0) {
                SDL_Log("Failed to SDL_SetWindowFullscreen: %s", SDL_GetError());
                exit(1);
            }
            break;
        case Mode::fullscr_win:
            if (SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP) != 0) {
                SDL_Log("Failed to SDL_SetWindowFullscreen: %s", SDL_GetError());
                exit(1);
            }
            break;
        case Mode::fullscr: { // TODO: cycle through supported modes?
            auto fr = int(frame_rate_in);
            SDL_DisplayMode sdm = { pixel_format, 1920, 1080, fr, 0 };
            if (SDL_SetWindowDisplayMode(window, &sdm) != 0) {
                SDL_Log("Failed to SDL_SetWindowDisplayMode: %s", SDL_GetError());
                exit(1);
            }
            if (SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN) != 0) {
                SDL_Log("Failed to SDL_SetWindowFullscreen: %s", SDL_GetError());
                exit(1);
            }
            break;
        }
    }

    int disp_idx = SDL_GetWindowDisplayIndex(window);
    if (SDL_GetCurrentDisplayMode(disp_idx, &sdl_mode) != 0) {
        SDL_Log("Failed to SDL_GetCurrentDisplayMode: %s", SDL_GetError());
        exit(1);
    }

    if (renderer) SDL_DestroyRenderer(renderer);

    const u32 v_sync_flag = v_synced() ? SDL_RENDERER_PRESENTVSYNC : 0;
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | v_sync_flag);
    if (!renderer) {
        SDL_Log("Failed to SDL_CreateRenderer: %s", SDL_GetError());
        exit(1);
    }
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2");
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

    frame.frame.connect(renderer);
    mask.frame.connect(renderer);

    mask.upd(set);

    upd_dimensions();

    SDL_ShowCursor(set.mode == Mode::win);
}


void Video_out::upd_dimensions() {
    if (set.mode == Mode::win) {
        const int w = set.aspect_ratio * (set.window_scale * VIC_II::FRAME_WIDTH);
        const int h = set.window_scale * VIC_II::FRAME_HEIGHT;
        SDL_SetWindowSize(window, w, h);
        frame.frame.dstrect = SDL_Rect{0, 0, w, h};
        mask.frame.srcrect = mask.frame.dstrect = SDL_Rect{0, 0, w, h};;
    } else {
        int win_w; int win_h;
        SDL_GetRendererOutputSize(renderer, &win_w, &win_h);
        int w = set.aspect_ratio * (((double)win_h / VIC_II::FRAME_HEIGHT) * VIC_II::FRAME_WIDTH);
        if (w < win_w) {
            frame.frame.dstrect.x = (win_w - w) / 2;
            frame.frame.dstrect.y = 0;
            frame.frame.dstrect.w = w;
            frame.frame.dstrect.h = win_h;
        } else {
            int h =  (((double)win_w / VIC_II::FRAME_WIDTH) * VIC_II::FRAME_HEIGHT) / set.aspect_ratio;
            frame.frame.dstrect.x = 0;
            frame.frame.dstrect.y = (win_h - h) / 2;
            frame.frame.dstrect.w = win_w;
            frame.frame.dstrect.h = h;
        }

        mask.frame.srcrect = mask.frame.dstrect = SDL_Rect{0, 0, win_w, win_h};;
    }

    SDL_RenderClear(renderer);
}


u16 Audio_out::config(u16 buf_sz) {
    SDL_AudioSpec want;
    SDL_AudioSpec have;

    if (dev) {
        flush();
        SDL_CloseAudioDevice(dev);
    }

    SDL_memset(&want, 0, sizeof(want));
    want.freq = AUDIO_OUTPUT_FREQ;
    want.format = AUDIO_S16LSB;
    want.channels = 1;
    want.samples = buf_sz;

    dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (dev == 0) {
        SDL_Log("Unable to SDL_OpenAudioDevice: %s", SDL_GetError());
        exit(1);
    }
    //std::cout << (int)have.freq << " " << (int)have.channels << " ";
    //std::cout << (int)want.format << " " << (int)have.format;

    SDL_PauseAudioDevice(dev, 0);

    return have.samples;
}

Audio_out::~Audio_out() {
    if (dev) SDL_CloseAudioDevice(dev);
}


_SDL& _sdl = _SDL::instance();
