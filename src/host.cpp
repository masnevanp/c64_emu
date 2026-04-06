
#include <array>
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
    ke::f8, sy::rst_warm, sy::rst_cold, sy::tgl_fscr, sy::mode, sy::nop, sy::nop, ke::rstre,
    // 90..9f
    sy::rot_dsk,kb::home, sy::nop,   sy::nop,   sy::nop,   sy::nop,   kb::crs_r,  ke::crs_l,
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
    kb::sh_l,  kb::cmdre, sy::nop,   kb::ctrl,  kb::sh_r,  sy::sys,   sy::nop,  sy::nop,
    // 130..13f
    sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,    sy::nop,
    sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,    sy::nop,
};


const u8 Input::SC_LU_TBL[] = {
    /* SDL_Scancode 2e..35 --> Key_code */
    kb::minus, kb::at,    kb::ar_up, sy::nop,   sy::nop,   kb::colon, kb::s_col, kb::pound,
};


const u8 Input::SC_RALT_LU_TBL[] = { // SDL_Scancode with RALT modifier */
    // 00..0f
    sy::nop, sy::nop, sy::nop, sy::nop, sy::menu_audio,sy::nop, sy::menu_vid_col, sy::menu_disk,
    sy::menu_exp ,sy::exp_btn_1,sy::nop,   sy::nop,   sy::nop, sy::swap_joy, sy::nop,   sy::nop,
    // 10..1f
    sy::menu_root, sy::nop, sy::nop, sy::menu_perf, sy::menu_quit, sy::menu_att_reu, sy::save_state, sy::nop,
    sy::nop, sy::menu_video,sy::tgl_wp, sy::menu_xtra, sy::nop, sy::nop, sy::step_cycle, sy::step_instr,
    // 20..2f
    sy::step_line, sy::step_frame,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   kb::eq,
    sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,
    // 30..3f
    sy::nop,   kb::mul,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,
    sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,
    // 40..4f
    sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,
    sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::menu_ent,
    // 50..5f
    sy::menu_exit, sy::menu_down, sy::menu_up,
                                     sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,
    sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,
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
    /*for (int j = 0; j < SDL_NumJoysticks() && open_joys < 2; ++j) {
        SDL_Joystick* sj = SDL_JoystickOpen(j);
        if(!sj) Log::error("Unable to SDL_JoystickOpen: %s", SDL_GetError());
        else {
            sdl_joystick_id[open_joys] = SDL_JoystickInstanceID(sj);
            sdl_joystick[open_joys++] = sj;
        }
    }*/
    Log::info("%d joysticks attached", open_joys);
}


void Input::poll() {
    while (SDL_PollEvent(&sdl_ev)) {
        switch (sdl_ev.type) {
            case SDL_EVENT_KEY_DOWN:       handle_key(true);      break;
            case SDL_EVENT_KEY_UP:         handle_key(false);     break;
            //case SDL_JOYAXISMOTION: handle_joy_axis();     break;
            //case SDL_JOYBUTTONDOWN: handle_joy_btn(true);  break;
            //case SDL_JOYBUTTONUP:   handle_joy_btn(false); break;
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                handlers.sys(Key_code::System::shutdown, true);
                break;
            case SDL_EVENT_WINDOW_FOCUS_GAINED:
                set_shift_lock();
                break;
            case SDL_EVENT_WINDOW_RESIZED:
                handlers.window_resized(sdl_ev.window.data1, sdl_ev.window.data2);
                break;
            case SDL_EVENT_DROP_FILE:
                handlers.filedrop(sdl_ev.drop.data);
                break;
        }
    }
}


void Input::swap_joysticks() {
    Sig_key* t = joy_handler[0];
    joy_handler[0] = joy_handler[1];
    joy_handler[1] = t;
    Log::info("Joysticks swapped");
}


u8 Input::translate_sdl_key() {
    static const i32 MAX_KC             = SDLK_SLEEP;
    static const i32 LAST_CHAR_KC       = SDLK_DELETE;
    static const i32 FIRST_NON_CHAR_KC  = SDLK_CAPSLOCK;
    static const i32 OFFSET_NON_CHAR_KC = FIRST_NON_CHAR_KC - (LAST_CHAR_KC + 1);

    const auto& k = sdl_ev.key;

    if (k.mod & SDL_KMOD_RALT) {
        if (k.scancode < std::size(SC_RALT_LU_TBL)) {
            if (auto kc = SC_RALT_LU_TBL[k.scancode]; kc != sy::nop) return kc;
        }
    }

    if (k.key <= MAX_KC) {
        // most mapped on keycode, some on scancode
        if (k.key <= LAST_CHAR_KC)
            return KC_LU_TBL[k.key];
        else if (k.key >= FIRST_NON_CHAR_KC)
            return KC_LU_TBL[k.key - OFFSET_NON_CHAR_KC];
        else if (k.scancode >= 0x2e && k.scancode <= 0x35)
            return SC_LU_TBL[k.scancode - 0x2e];
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
            using ke = Key_code::Keyboard_ext;
            switch (code) {
                case ke::crs_u: case ke::f6: case ke::f4:
                case ke::f2:    case ke::f8: case ke::crs_l: {
                    // 6 of these are actually used (for crs_u, f6, f4, f2, f8, crs_l)
                    static const u8 BIT[] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 };

                    if (down) {
                        if (sh_r_down == 0x00) {
                            // first 'auto shifted' key held down
                            //   --> set 'right shift' key down & disable the 'real' one
                            handlers.keyboard(Key_code::Keyboard::sh_r, true);
                            disable_sh_r();
                        }
                        sh_r_down |= BIT[code & 0x7]; // keep track of key presses
                    } else {
                        sh_r_down &= (~BIT[code & 0x7]);
                        if (sh_r_down == 0x00) {
                            // last 'auto shifted' key released
                            //   --> set 'right shift' key up & enable the 'real' one
                            handlers.keyboard(Key_code::Keyboard::sh_r, false);
                            enable_sh_r();
                        }
                    }

                    const u8 kc_offset = Key_code::Keyboard_ext::crs_l - Key_code::Keyboard::crs_r;
                    handlers.keyboard(code - kc_offset, down); // send the corresponding 'real' code

                    break;
                }
                case ke::s_lck: set_shift_lock();       break;
                case ke::rstre: handlers.restore(down); break;
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



void Video_out::put(const u8* vic_frame) {
    //SDL_RenderClear(renderer);
    frame.put(vic_frame, renderer);
    mask.put(renderer);
    flip();
}


Video_out::SDL_frame::SDL_frame(int max_w_, int max_h_, SDL_TextureAccess ta_, SDL_BlendMode bm_)
    : max_w(max_w_), max_h(max_h_), ta(ta_), bm(bm_), pixels(new u32[max_w * max_h])
{
}


Video_out::SDL_frame::~SDL_frame() {
    delete[] pixels;
}


void Video_out::SDL_frame::connect(SDL_Renderer* renderer) {
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
        Log::error("Failed to SDL_CreateTexture: %s", SDL_GetError());
        exit(1);
    }

    if (!SDL_SetTextureBlendMode(t, bm)) {
        Log::error("Failed to SDL_SetTextureBlendMode: %s", SDL_GetError());
    }

    // linear is the default mode
    // SDL_SetTextureScaleMode(t, SDL_ScaleMode::SDL_SCALEMODE_LINEAR);

    return t;
}


void Video_out::upd_mode() {
    if (!window) {
        window = SDL_CreateWindow("WIP #$40", 100, 100, SDL_WINDOW_RESIZABLE);
        if (!window) {
            Log::error("Failed to SDL_CreateWindow: %s", SDL_GetError());
            exit(1);
        }
    }

    if (renderer) SDL_DestroyRenderer(renderer);

    switch (set.mode) {
        case Mode::win:
            SDL_SetWindowFullscreen(window, false); // TODO: somehow check success?
            SDL_SyncWindow(window);
            break;
        case Mode::fullscr_win:
            SDL_SetWindowFullscreen(window, true); // TODO: somehow check success?
            SDL_SyncWindow(window);
            break;
        case Mode::fullscr:
            Log::info("TODO: Mode::fullscr");
            break;
        /*case Mode::fullscr: { // TODO: cycle through supported modes?
            auto fr = int(frame_rate_in);
            SDL_DisplayMode sdm = { pixel_format, 1920, 1080, fr, 0 };
            if (SDL_SetWindowDisplayMode(window, &sdm) != 0) {
                Log::error("Failed to SDL_SetWindowDisplayMode: %s", SDL_GetError());
                exit(1);
            }
            if (SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN) != 0) {
                Log::error("Failed to SDL_SetWindowFullscreen: %s", SDL_GetError());
                exit(1);
            }
            break;
        }*/
    }

    const auto disp_id = SDL_GetDisplayForWindow(window);
    if (sdl_mode = SDL_GetCurrentDisplayMode(disp_id); !sdl_mode) {
        Log::error("Failed to SDL_GetCurrentDisplayMode: %s", SDL_GetError());
        exit(1);
    }

    // const u32 v_sync_flag = v_synced() ? SDL_RENDERER_PRESENTVSYNC : 0;
    Log::info("TODO: vsync support");
    renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer) {
        Log::error("Failed to SDL_CreateRenderer: %s", SDL_GetError());
        exit(1);
    }

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

    frame.frame.connect(renderer);
    mask.frame.connect(renderer);

    mask.upd(set);

    upd_dimensions();

    if(set.mode == Mode::win) SDL_ShowCursor(); else SDL_HideCursor();
}


void Video_out::upd_dimensions() {
    const auto aspect_ratio = set.aspect_ratio / 1000.0;
    const auto window_scale = set.window_scale / 100.0;

    if (set.mode == Mode::win) {
        const float w = aspect_ratio * (window_scale * VIC_II::FRAME_WIDTH);
        const float h = window_scale * VIC_II::FRAME_HEIGHT;
        SDL_SetWindowSize(window, w, h);
        frame.frame.dstrect = SDL_FRect{0, 0, w, h};
        mask.frame.srcrect = mask.frame.dstrect = SDL_FRect{0, 0, w, h};;
    } else {
        int win_w; int win_h;
        SDL_GetCurrentRenderOutputSize(renderer, &win_w, &win_h);
        int w = aspect_ratio * (((double)win_h / VIC_II::FRAME_HEIGHT) * VIC_II::FRAME_WIDTH);
        if (w < win_w) {
            frame.frame.dstrect.x = (win_w - w) / 2;
            frame.frame.dstrect.y = 0;
            frame.frame.dstrect.w = w;
            frame.frame.dstrect.h = win_h;
        } else {
            int h =  (((double)win_w / VIC_II::FRAME_WIDTH) * VIC_II::FRAME_HEIGHT) / aspect_ratio;
            frame.frame.dstrect.x = 0;
            frame.frame.dstrect.y = (win_h - h) / 2;
            frame.frame.dstrect.w = win_w;
            frame.frame.dstrect.h = h;
        }

        mask.frame.srcrect = mask.frame.dstrect = SDL_FRect{0, 0, float(win_w), float(win_h)};;
    }

    SDL_RenderClear(renderer);
}


void Video_out::resize_window(int w, int h) {
    if (set.mode != Mode::win) return;

    const int old_w = frame.frame.dstrect.w;
    const int old_h = frame.frame.dstrect.h;

    if (w != old_w && h != old_h) {
        // pick the bigger change (keeping aspect ratio)
        if (std::abs(w - old_w) > std::abs(h - old_h)) h = old_h;
        else w = old_w;
    }

    if (w != old_w) {
        set.window_scale.set(100 * (double(w) / VIC_II::FRAME_WIDTH));
    } else if (h != old_h) {
        set.window_scale.set(100 * (double(h) / VIC_II::FRAME_HEIGHT));
    }

    /*
    if (w != old_w && h != old_h) { // update aspect ratio
        const auto window_scale = double(h) / VIC_II::FRAME_HEIGHT;
        const auto aspect_ratio = double(w) / (window_scale * VIC_II::FRAME_WIDTH);
        set.aspect_ratio.set(1000 * aspect_ratio);
        set.window_scale.set(100 * window_scale);
    } else if (w != old_w) { // keep aspect ratio
        set.window_scale.set(100 * (double(w) / VIC_II::FRAME_WIDTH));
    } else if (h != old_h) { // keep aspect ratio
        set.window_scale.set(100 * (double(h) / VIC_II::FRAME_HEIGHT));
    }
    */

    upd_dimensions();
}


u16 Audio_out::config(u16 buf_sz) {
    /*SDL_AudioSpec want;
    SDL_AudioSpec have;

    Log::info("Configuring audio device...");

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
        Log::error("Fail: '%s'", SDL_GetError());
        return 0;
    }

    SDL_PauseAudioDevice(dev, 0);

    Log::info("Done.");

    return have.samples;*/
    return 0;
}


_SDL& _sdl = _SDL::instance();
