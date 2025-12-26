
#include "expansion.h"
#include "utils.h"
#include "nmos6502/nmos6502_mc.h"



/*
  TODO:
    - handle .CRTs with banks out-of-order (although .CRTs seem to have the chips always
      in order)
*/

using namespace Expansion;


// for the basic 'no fancy dynamic banking' carts
int Expansion::CRT::load_static_chips(const Files::CRT& crt, u8* tgt_mem) {
    int count = 0;

    for (auto cp : crt.chip_packets()) {
        /*if (cp->load_addr == 0x8000) tgt_addr = 0x0000;
        else if (cp->load_addr == 0xa000 || cp->load_addr == 0xe000) tgt_addr = 0x2000;
        else {
            Log::error("Expansion: invalid CRT CHIP packet address - %d", (int)cp->load_addr);
            continue;
        }*/

        const auto tgt_addr = cp->load_addr;
        std::copy(cp->data(), cp->data() + cp->data_size, tgt_mem + tgt_addr);

        ++count;
    }

    return count;
}

// just load all the chips in the order found (TODO: ever need to check the order?)
int Expansion::CRT::load_chips(const Files::CRT& crt, u8* tgt_mem) {
    int count = 0;

    for (auto cp : crt.chip_packets()) {
        std::copy(cp->data(), cp->data() + cp->data_size, tgt_mem);
        tgt_mem += cp->data_size;
        ++count;
    }

    return count;
}


void Expansion::detach(State::System& s) {
    s.exp.type = Expansion::Type::none;
    s.exp.ticker = Expansion::Ticker::idle;
    System::set_exrom_game(true, true, s);
}


bool Expansion::attach(State::System& s, const Files::CRT& crt) {
    if (!crt.header().valid()) {
        Log::error("Expansion: invalid CRT img header");
        return false;
    }

    detach(s);

    bool success;
    const auto type = crt.header().hw_type + Type::generic; // apply the offset
    switch (type) {
        #define T(t) case t: success = T##t{s}.attach(crt); break;

        T(2) T(6) T(12) T(21)

        default:
            Log::error("Expansion: unsupported CRT HW type: %d", (int)type);
            success = false;
            break;

        #undef T
    }

    if (success) {
        s.exp.type = type;
        Log::info("Expansion: attached CRT, type: %d", type);
        return true;
    } else {
        Log::error("Expansion: failed to attach CRT");
        return false;
    }
}


bool Expansion::attach_REU(State::System& s) {
    detach(s);
    s.exp.type = Type::REU;
    Log::info("Expansion: REU attached");
    return true;
}


void Expansion::reset(State::System& s) {
    switch (s.exp.type) {
        #define T(t) case t: T##t{s}.reset(); break;

        T(0) T(1) T(2) T(12) T(21)

        #undef T
    }
}


/*
Result T5_Ocean_type_1(const Files::CRT& crt, Ctx& ctx) {
    u32 exp_mem_addr = 0x0001; // current bank stored at 0x0000
    int banks = 0;

    for (auto cp : crt.chip_packets()) {
        auto la = cp->load_addr;
        if ((la != 0x8000 && la != 0xa000) || cp->data_size != 0x2000) {
            Log::error("CRT: invalid CHIP packet");
            return not_attached;
        }

        std::copy(cp->data(), cp->data() + cp->data_size, &ctx.ram[exp_mem_addr]);

        exp_mem_addr += 0x2000;
        ++banks;
    }

    if (banks == 0) return not_attached;

    struct bank {
        void operator=(u8 b) { _c.ram[0x0000] = b & 0b00111111; }
        operator u32() const { return (_c.ram[0x0000] * 0x2000) + 1; }
        Ctx& _c;
    };

    const bool game = (banks == 64);

    if (!game) {
        ctx.io.romh_r = [&](const u16& a, u8& d) { d = ctx.ram[bank{ctx} + a]; };
    }

    ctx.io.roml_r = [&](const u16& a, u8& d) { d = ctx.ram[bank{ctx} + a]; };

    ctx.io.io1_w = [&](const u16& a, const u8& d) {
        if (a == 0x0e00) bank{ctx} = d & 0b00111111;
    };

    ctx.reset = [&]() { bank{ctx} = 0; };

    return exrom_game(false, game);
}


Result T16_Warp_Speed(const Files::CRT& crt, Ctx& ctx) {
    if (load_static_chips(crt, ctx) != 1) return not_attached; // TODO: verify the size (0x4000)

    struct active {
        operator bool() const { return _c.ram[0x4000]; }
        void operator=(bool act) {
            _c.ram[0x4000] = act;
            _c.io.exrom_game(!act, !act);
        }

        Ctx& _c;
    };

    ctx.io.roml_r = [&](const u16& a, u8& d) {
        d = active{ctx}
                ? ctx.ram[0x0000 + a]
                : ctx.sys_ram[0x8000 + a];
    };
    ctx.io.romh_r = [&](const u16& a, u8& d) {
        d = active{ctx}
                ? ctx.ram[0x2000 + a]
                : ctx.sys_ram[0xa000 + a];
    };

    ctx.io.io1_r = [&](const u16& a, u8& d) {
        d = ctx.ram[0x1e00 | (a & 0x00ff)];
    };
    ctx.io.io2_r = [&](const u16& a, u8& d) {
        d = ctx.ram[0x1f00 | (a & 0x00ff)];
    };
    ctx.io.io1_w = [&](const u16& a, const u8& d) { UNUSED(a); UNUSED(d);
        active{ctx} = true;
    };
    ctx.io.io2_w = [&](const u16& a, const u8& d) { UNUSED(a); UNUSED(d);
        active{ctx} = false;
    };

    ctx.reset = [&]() { active{ctx} = true; };

    return exrom_game(false, false);
}


Result T18_Zaxxon_Super_Zaxxon_SEGA(const Files::CRT& crt, Ctx& ctx) {
    if (load_chips(crt, ctx) != 3) return not_attached;

    struct romh_base {
        void operator=(u16 b) { _base() = b; }
        operator u16() const  { return _base(); }
        u16& _base() const    { return *(u16*)&_c.ram[0x5000]; }
        Ctx& _c;
    };

    ctx.io.roml_r = [&](const u16& a, u8& d) {
        romh_base{ctx} = 0x1000 | ((a & 0x1000) << 1);
        d = ctx.ram[a & 0x0fff];
    };
    ctx.io.romh_r = [&](const u16& a, u8& d) {
        d = ctx.ram[romh_base{ctx} + a];
    };

    return exrom_game(crt);
}


Result T51_MACH_5(const Files::CRT& crt, Ctx& ctx) {
    if (load_static_chips(crt, ctx) != 1) return not_attached;

    struct active {
        void operator=(bool act) { _c.io.exrom_game(!act, true); }
        Ctx& _c;
    };

    ctx.io.roml_r = [&](const u16& a, u8& d) { d = ctx.ram[a]; };

    ctx.io.io1_r = [&](const u16& a, u8& d) { d = ctx.ram[0x1e00 | (a & 0x00ff)]; };
    ctx.io.io2_r = [&](const u16& a, u8& d) { d = ctx.ram[0x1f00 | (a & 0x00ff)]; };
    ctx.io.io1_w = [&](const u16& a, const u8& d) { UNUSED(a); UNUSED(d);
        active{ctx} = true;
    };
    ctx.io.io2_w = [&](const u16& a, const u8& d) { UNUSED(a); UNUSED(d);
        active{ctx} = false;
    };

    ctx.reset = [&]() { active{ctx} = true; };

    return exrom_game(crt);
}
*/