
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
    ke::f8,    sy::rst_w, sy::rst_c, sy::swp_j, sy::quit,  sy::nop,   sy::vo_fsc, sy::rstre,
    // 90..9f
    sy::nop,   kb::home,  sy::menu_pl, sy::nop, sy::menu_ent, sy::menu_mi, kb::crs_r,  ke::crs_l,
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


void Video_out::Frame::upd_palette() {
    get_Colodore(palette, set.brightness, set.contrast, set.saturation);
    palette[0] = 0x020202; // keep it blackish...
}


void Video_out::Frame::upd_sharpness() {
    struct Sz { const u8 w; const u8 h; };

    static constexpr std::array<Sz, 5> px_sz{{ {1, 1}, {1, 2}, {2, 2}, {3, 3} }};

    frame.srcrect.w = VIC_II::FRAME_WIDTH * px_sz[set.sharpness].w;
    frame.srcrect.h = VIC_II::FRAME_HEIGHT * px_sz[set.sharpness].h;

    switch (set.sharpness) {
        case 0: // {1, 1}
            draw = [&]() {
                for (int pos = 0; pos < VIC_II::FRAME_SIZE; ++pos) {
                    frame.pixels[pos] = col(vic_frame[pos]);
                }
                const auto bytes_per_row = 1 * (bytes_per_pixel * VIC_II::FRAME_WIDTH);
                SDL_UpdateTexture(frame.texture, 0, (void*)frame.pixels, bytes_per_row);
            };
            break;
        case 1: // {1, 2}
            draw = [&]() {
                const u8* vic_frame_pos = vic_frame;
                u32* scan = frame.pixels;

                for (int y = 0; y < VIC_II::FRAME_HEIGHT; ++y) {
                    for (int x = 0; x < VIC_II::FRAME_WIDTH; ++x, ++scan) {
                        const auto c = col(*vic_frame_pos++);
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
            draw = [&]() {
                const u8* vic_frame_pos = vic_frame;
                u32* scan = frame.pixels;

                for (int y = 0; y < VIC_II::FRAME_HEIGHT; ++y) {
                    for (int x = 0; x < VIC_II::FRAME_WIDTH; ++x) {
                        const auto c = col(vic_frame_pos[x]);
                        *scan++ = c;
                        *scan++ = c;
                    }
                    for (int x = 0; x < VIC_II::FRAME_WIDTH; ++x) {
                        const auto c = col(vic_frame_pos[x]);
                        *scan++ = c;
                        *scan++ = c;
                    }
                    vic_frame_pos += VIC_II::FRAME_WIDTH;
                }

                const auto bytes_per_row = 2 * (bytes_per_pixel * VIC_II::FRAME_WIDTH);
                SDL_UpdateTexture(frame.texture, 0, (void*)frame.pixels, bytes_per_row);
            };
            break;
        case 3: // {3, 3}
            draw = [&]() {
                const u8* vic_frame_pos = vic_frame;
                u32* scan = frame.pixels;

                auto draw_row = [&]() {
                    for (int x = 0; x < VIC_II::FRAME_WIDTH; ++x) {
                        const auto c = col(vic_frame_pos[x]);
                        *scan++ = c;
                        *scan++ = c;
                        *scan++ = c;
                    }
                };

                for (int y = 0; y < VIC_II::FRAME_HEIGHT; ++y) {
                    draw_row();
                    draw_row();
                    draw_row();
                    vic_frame_pos += VIC_II::FRAME_WIDTH;
                }

                const auto bytes_per_row = 3 * (bytes_per_pixel * VIC_II::FRAME_WIDTH);
                SDL_UpdateTexture(frame.texture, 0, (void*)frame.pixels, bytes_per_row);
            };
            break;
    }
}


const std::vector<Video_out::Filter::Pattern> Video_out::Filter::patterns = {
    { 1, 2, {
        { 0x000000 },
        { 0x111111 },
    }},
    { 2, 2, {
        { 0x000000, 0x001111 },
        { 0x110011, 0x111100 },
    }},
    { 3, 2, {
        { 0x000000, 0x001111, 0x110011 },
        { 0x000000, 0x000000, 0x111100 },
    }},
    { 3, 3, {
        { 0x000011, 0x001100, 0x110000 },
        { 0x0a0303, 0x030a03, 0x03030a },
    }},
    { 3, 2, {
        { 0x000011, 0x001100, 0x110000 },
        { 0x0a0303, 0x030a03, 0x03030a },
    }},
    { 3, 2, {
        { 0x000000, 0x000000, 0x001111 },
        { 0x000000, 0x110011, 0x111100 },
    }},
    { 3, 3, {
        { 0x000000, 0x000000, 0x050505 },
        { 0x000000, 0x000000, 0x050505 },
        { 0x0b0b0b, 0x0b0b0b, 0x111111 },
    }},
    { 3, 3, {
        { 0x111111, 0x050505, 0x050505 },
        { 0x0b0b0b, 0x000000, 0x000000 },
        { 0x0b0b0b, 0x000000, 0x000000 },
    }},
    { 4, 3, {
        { 0x000000, 0x000000, 0x050505 },
        { 0x000000, 0x000000, 0x050505 },
        { 0x0b0b0b, 0x0b0b0b, 0x111111 },
    }},
    { 4, 3, {
        { 0x000000, 0x050505, 0x000000 },
        { 0x000000, 0x050505, 0x000000 },
        { 0x0b0b0b, 0x111111, 0x0b0b0b },
    }},
};


void Video_out::Filter::upd() {
    static constexpr u32 all_pass = 0xffffffff;

    const auto& ptrn = patterns[set.filter_pattern % patterns.size()];

    auto p = frame.pixels;
    const auto ptrn_rows = ptrn.shape.size();
    for (int y = 0; y < VIC_II::FRAME_HEIGHT * ptrn.px_h; ++y) {
        const auto& row = ptrn.shape[y % ptrn_rows];
        const auto ptrn_row_cols = row.size();
        for (int x = 0; x < VIC_II::FRAME_WIDTH * ptrn.px_w; ++x) {
            *p++ = all_pass - (set.filter_level * row[x % ptrn_row_cols]);
        }
    }

    const auto bytes_per_row = VIC_II::FRAME_WIDTH * bytes_per_pixel * ptrn.px_w;
    SDL_UpdateTexture(frame.texture, nullptr, (void*)frame.pixels, bytes_per_row);

    frame.srcrect.w = VIC_II::FRAME_WIDTH * ptrn.px_w;
    frame.srcrect.h = VIC_II::FRAME_HEIGHT * ptrn.px_h;
}



Video_out::~Video_out() {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
}


std::vector<Menu::Item*> Video_out::settings_menu() {
    std::vector<Menu::Item*> m;
    for (auto& i : _settings_menu) m.push_back(&i);
    return m;
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
            auto fr = int(FRAME_RATE);
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

    u32 v_sync_flag = sdl_mode.refresh_rate == FRAME_RATE ? SDL_RENDERER_PRESENTVSYNC : 0;
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | v_sync_flag);
    if (!renderer) {
        SDL_Log("Failed to SDL_CreateRenderer: %s", SDL_GetError());
        exit(1);
    }
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2");
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

    frame.frame.connect(renderer);
    filter.frame.connect(renderer);

    filter.upd();

    upd_dimensions();

    SDL_ShowCursor(set.mode == Mode::win);
}


void Video_out::upd_dimensions() {
    const double aspect_ratio = set.aspect_ratio / 200.0;

    if (set.mode == Mode::win) {
        int w = aspect_ratio * (set.window_scale / 10.0) * VIC_II::FRAME_WIDTH;
        int h = (set.window_scale / 10.0) * VIC_II::FRAME_HEIGHT;
        SDL_SetWindowSize(window, w, h);
        dstrect = nullptr;
    } else {
        int win_w; int win_h;
        SDL_GetRendererOutputSize(renderer, &win_w, &win_h);
        int w = aspect_ratio * (((double)win_h / VIC_II::FRAME_HEIGHT) * VIC_II::FRAME_WIDTH);
        if (w < win_w) {
            fullscreen_dstrect.x = (win_w - w) / 2;
            fullscreen_dstrect.y = 0;
            fullscreen_dstrect.w = w;
            fullscreen_dstrect.h = win_h;
        } else {
            int h =  (((double)win_w / VIC_II::FRAME_WIDTH) * VIC_II::FRAME_HEIGHT) / aspect_ratio;
            fullscreen_dstrect.x = 0;
            fullscreen_dstrect.y = (win_h - h) / 2;
            fullscreen_dstrect.w = win_w;
            fullscreen_dstrect.h = h;
        }
        dstrect = &fullscreen_dstrect;
    }

    SDL_RenderClear(renderer);
}


Audio_out::Audio_out() {
    SDL_AudioSpec want;
    SDL_AudioSpec have;

    SDL_memset(&want, 0, sizeof(want));
    want.freq = 44100;
    want.format = AUDIO_S16LSB;
    want.channels = 1;
    want.samples = 256;

    dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (dev == 0) {
        SDL_Log("Unable to SDL_OpenAudioDevice: %s", SDL_GetError());
        exit(1);
    }
    //std::cout << (int)have.freq << " " << (int)have.channels << " ";
    //std::cout << (int)want.format << " " << (int)have.format;

    SDL_PauseAudioDevice(dev, 0);
}

Audio_out::~Audio_out() {
    SDL_CloseAudioDevice(dev);
}


_SDL& _sdl = _SDL::instance();
