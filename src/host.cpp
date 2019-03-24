
#include "host.h"



using kb = Key_code::Keyboard;
using ke = Key_code::Keyboard_ext;
using js = Key_code::Joystick;
using sy = Key_code::System;


static const u8 j1 = 0x00;
static const u8 j2 = Host::Input::JOY_ID_BIT;


// TODO: analyze... use more scancodes?
u8 Host::KC_LU_TBL[] = {
    /* 00..7f : SDL_Keycode 00..7f  --> Key_event::KC */
    // 00..0f
    sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,
    kb::del,   kb::ar_l,  sy::nop,   sy::nop,   sy::nop,   kb::ret,   sy::nop,   sy::nop,
    // 10..1f
    sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,
    sy::nop,   sy::nop,   sy::nop,   kb::r_stp, sy::nop,   sy::nop,   sy::nop,   sy::nop,
    // 20..2f
    kb::space, sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   kb::eq,
    sy::nop,   sy::nop,   sy::nop,   kb::plus,  kb::comma, kb::div,   kb::dot,   sy::nop,
    // 30..3f
    kb::num_0, kb::num_1, kb::num_2, kb::num_3, kb::num_4, kb::num_5, kb::num_6, kb::num_7,
    kb::num_8, kb::num_9, sy::nop,   sy::nop,   kb::ar_l,  sy::nop,   sy::nop,   sy::nop,
    // 40..4f
    sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,
    sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,
    // 50..5f
    sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,
    sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,
    // 60..6f
    sy::nop,   kb::a,     kb::b,     kb::c,     kb::d,     kb::e,     kb::f,     kb::g,
    kb::h,     kb::i,     kb::j,     kb::k,     kb::l,     kb::m,     kb::n,     kb::o,
    // 70..7f
    kb::p,     kb::q,     kb::r,     kb::s,     kb::t,     kb::u,     kb::v,     kb::w,
    kb::x,     kb::y,     kb::z,     sy::nop,   sy::nop,   sy::nop,   sy::nop,   kb::del,

    /* 80..   : SDL_Keycode 40000039.. --> Key_event::KC (80 = kc 40000039, 81 = kc 4000003a, ...) */
    // 80..8f
    sy::nop,   kb::f1,    ke::f2,    kb::f3,    ke::f4,    kb::f5,    ke::f6,    kb::f7,
    ke::f8,    sy::rst_w, sy::rst_c, sy::load,  sy::quit,  sy::nop,   sy::nop,   sy::rstre,
    // 90..9f
    sy::nop,   kb::home,  sy::nop,   sy::nop,   sy::rstre, sy::swp_j, kb::crs_r, ke::crs_l,
    kb::crs_d, ke::crs_u, sy::nop,   j1|js::ju, kb::mul,   kb::minus, kb::plus,  kb::ret,
    // a0..af
    j2|js::jl, j2|js::jd, j2|js::jr, j1|js::jb, j2|js::ju, sy::nop,   j1|js::jl, j1|js::jd,
    j1|js::jr, j2|js::jb, sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,
    // b0..bf
    sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,
    sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,
    // c0..cf
    sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,
    sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,
    // d0..df
    sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,
    sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,
    // e0..ef
    sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,
    sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,
    // f0..ff
    sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,
    sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,
    // 100..10f
    sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,
    sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,
    // 110..11f
    sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,
    sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,
    // 120..12f
    sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   kb::ctrl,
    kb::sh_l,  kb::cmdre, sy::nop,   kb::ctrl,  kb::sh_r,  sy::nop,   sy::nop,   sy::nop,
    // 130..13f
    sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,
    sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,   sy::nop,
};


const u8 Host::SC_LU_TBL[] = {
    /* SDL_Scancode 2e..35 --> Key_event::KC */
    kb::minus, kb::at,    kb::ar_up, sy::nop,   sy::nop,   kb::colon, kb::s_col, kb::pound,
};


Host::_SDL& _sdl = Host::_SDL::instance();
