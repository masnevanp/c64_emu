
#include "host.h"


using kc = Key_event::KC;

// TODO: analyze... use more scancodes?
const u8 Host::KC_LU_TBL[] = {
    /* 00..7f : SDL_Keycode 00..7f  --> Key_event::KC */
    // 00..0f
    kc::none,  kc::none,  kc::none,  kc::none,  kc::none,  kc::none,  kc::none,  kc::none,
    kc::del,   kc::ar_l,  kc::none,  kc::none,  kc::none,  kc::ret,   kc::none,  kc::none,
    // 10..1f
    kc::none,  kc::none,  kc::none,  kc::none,  kc::none,  kc::none,  kc::none,  kc::none,
    kc::none,  kc::none,  kc::none,  kc::r_stp, kc::none,  kc::none,  kc::none,  kc::none,
    // 20..2f
    kc::space, kc::none,  kc::none,  kc::none,  kc::none,  kc::none,  kc::none,  kc::eq,
    kc::none,  kc::none,  kc::none,  kc::plus,  kc::comma, kc::div,   kc::dot,   kc::none,
    // 30..3f
    kc::num_0, kc::num_1, kc::num_2, kc::num_3, kc::num_4, kc::num_5, kc::num_6, kc::num_7,
    kc::num_8, kc::num_9, kc::none,  kc::none,  kc::ar_l,  kc::none,  kc::none,  kc::none,
    // 40..4f
    kc::none,  kc::none,  kc::none,  kc::none,  kc::none,  kc::none,  kc::none,  kc::none,
    kc::none,  kc::none,  kc::none,  kc::none,  kc::none,  kc::none,  kc::none,  kc::none,
    // 50..5f
    kc::none,  kc::none,  kc::none,  kc::none,  kc::none,  kc::none,  kc::none,  kc::none,
    kc::none,  kc::none,  kc::none,  kc::none,  kc::none,  kc::none,  kc::none,  kc::none,
    // 60..6f
    kc::none,  kc::a,     kc::b,     kc::c,     kc::d,     kc::e,     kc::f,     kc::g,
    kc::h,     kc::i,     kc::j,     kc::k,     kc::l,     kc::m,     kc::n,     kc::o,
    // 70..7f
    kc::p,     kc::q,     kc::r,     kc::s,     kc::t,     kc::u,     kc::v,     kc::w,
    kc::x,     kc::y,     kc::z,     kc::none,  kc::none,  kc::none,  kc::none,  kc::del,

    /* 80..   : SDL_Keycode 40000039.. --> Key_event::KC (80 = kc 40000039, 81 = kc 4000003a, ...) */
    // 80..8f
    kc::none,  kc::f1,    kc::none,  kc::f3,    kc::none,  kc::f5,    kc::none,  kc::f7,
    kc::s_joy, kc::rst_w, kc::rst_c, kc::load,  kc::quit,  kc::none,  kc::none,  kc::rstre,
    // 90..9f
    kc::none,  kc::home,  kc::none,  kc::none,  kc::rstre, kc::none,  kc::crs_r, kc::none,
    kc::crs_d, kc::none,  kc::none,  kc::j1_u,  kc::mul,   kc::minus, kc::plus,  kc::ret,
    // a0..af
    kc::j2_l,  kc::j2_d,  kc::j2_r,  kc::j1_b,  kc::j2_u,  kc::none,  kc::j1_l,  kc::j1_d,
    kc::j1_r,  kc::j2_b,  kc::none,  kc::none,  kc::none,  kc::none,  kc::none,  kc::none,
    // b0..bf
    kc::none,  kc::none,  kc::none,  kc::none,  kc::none,  kc::none,  kc::none,  kc::none,
    kc::none,  kc::none,  kc::none,  kc::none,  kc::none,  kc::none,  kc::none,  kc::none,
    // c0..cf
    kc::none,  kc::none,  kc::none,  kc::none,  kc::none,  kc::none,  kc::none,  kc::none,
    kc::none,  kc::none,  kc::none,  kc::none,  kc::none,  kc::none,  kc::none,  kc::none,
    // d0..df
    kc::none,  kc::none,  kc::none,  kc::none,  kc::none,  kc::none,  kc::none,  kc::none,
    kc::none,  kc::none,  kc::none,  kc::none,  kc::none,  kc::none,  kc::none,  kc::none,
    // e0..ef
    kc::none,  kc::none,  kc::none,  kc::none,  kc::none,  kc::none,  kc::none,  kc::none,
    kc::none,  kc::none,  kc::none,  kc::none,  kc::none,  kc::none,  kc::none,  kc::none,
    // f0..ff
    kc::none,  kc::none,  kc::none,  kc::none,  kc::none,  kc::none,  kc::none,  kc::none,
    kc::none,  kc::none,  kc::none,  kc::none,  kc::none,  kc::none,  kc::none,  kc::none,
    // 100..10f
    kc::none,  kc::none,  kc::none,  kc::none,  kc::none,  kc::none,  kc::none,  kc::none,
    kc::none,  kc::none,  kc::none,  kc::none,  kc::none,  kc::none,  kc::none,  kc::none,
    // 110..11f
    kc::none,  kc::none,  kc::none,  kc::none,  kc::none,  kc::none,  kc::none,  kc::none,
    kc::none,  kc::none,  kc::none,  kc::none,  kc::none,  kc::none,  kc::none,  kc::none,
    // 120..12f
    kc::none,  kc::none,  kc::none,  kc::none,  kc::none,  kc::none,  kc::none,  kc::ctrl,
    kc::sh_l,  kc::cmdre, kc::none,  kc::ctrl,  kc::sh_r,  kc::none,  kc::none,  kc::none,
    // 130..13f
    kc::none,  kc::none,  kc::none,  kc::none,  kc::none,  kc::none,  kc::none,  kc::none,
    kc::none,  kc::none,  kc::none,  kc::none,  kc::none,  kc::none,  kc::none,  kc::none,
};


const u8 Host::SC_LU_TBL[] = {
    /* SDL_Scancode 2e..35 --> Key_event::KC */
    kc::minus, kc::at,    kc::ar_up, kc::none,  kc::none,  kc::colon, kc::s_col, kc::pound,
};


Host::_SDL& _sdl = Host::_SDL::instance();
