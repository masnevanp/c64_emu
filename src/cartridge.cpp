
#include <iostream>
#include <optional>
#include "cartridge.h"
#include "utils.h"



using Ctx = Expansion_ctx;

// loads all the chips in the order found (so not suitable for every cart type)
int load_chips(const Files::CRT& crt, Ctx& ctx) {
    int count = 0;

    for (auto cp : crt.chip_packets()) {
        u32 exp_mem_addr;

        if (cp->load_addr == 0x8000) exp_mem_addr = 0x0000;
        else if (cp->load_addr == 0xa000 || cp->load_addr == 0xe000) exp_mem_addr = 0x2000;
        else {
            std::cout << "CRT: invalid CHIP packet address - " << (int)cp->load_addr << std::endl;
            continue;
        }

        std::copy(cp->data(), cp->data() + cp->data_size, &ctx.mem[exp_mem_addr]);

        ++count;
    }

    return count;
}


struct exrom_game {
    bool exrom; bool game;
    exrom_game(bool e, bool g) : exrom(e), game(g) {}
    exrom_game(const Files::CRT& crt)
        : exrom(bool(crt.header().exrom)), game(bool(crt.header().game)) {} 
};

using Result = std::optional<exrom_game>;


static const auto& not_attached = std::nullopt;


Result T0_Normal_cartridge(const Files::CRT& crt, Ctx& ctx) {
    if (load_chips(crt, ctx) == 0) return not_attached;

    ctx.as.roml_r = [&](const u16& a, u8& d) { d = ctx.mem[a]; };
    ctx.as.romh_r = [&](const u16& a, u8& d) { d = ctx.mem[0x2000 + a]; };

    return exrom_game(crt);
}


Result T4_Simons_BASIC(const Files::CRT& crt, Ctx& ctx) {
    if (load_chips(crt, ctx) != 2) return not_attached;

    struct romh_active {
        void operator=(bool act) { _c.as.exrom_game(false, !act); }
        Ctx& _c;
    };

    ctx.as.roml_r = [&](const u16& a, u8& d) {
        d = ctx.mem[a];
    };
    ctx.as.romh_r = [&](const u16& a, u8& d) {
        d = ctx.mem[0x2000 + a];
    };

    ctx.as.io1_r = [&](const u16& a, u8& d) { UNUSED(a); UNUSED(d);
        romh_active{ctx} = false;
    };
    ctx.as.io1_w = [&](const u16& a, const u8& d) { UNUSED(a); UNUSED(d);
        romh_active{ctx} = true;
    };

    ctx.reset = [&]() { romh_active{ctx} = false; };

    return exrom_game(crt);
}


Result T5_Ocean_type_1(const Files::CRT& crt, Ctx& ctx) {
    u32 exp_mem_addr = 0x0001;
    int banks = 0;

    for (auto cp : crt.chip_packets()) {
        auto la = cp->load_addr;
        if ((la != 0x8000 && la != 0xa000) || cp->data_size != 0x2000) {
            std::cout << "CRT: invalid CHIP packet" << std::endl;
            return not_attached;
        }

        std::copy(cp->data(), cp->data() + cp->data_size, &ctx.mem[exp_mem_addr]);

        exp_mem_addr += 0x2000;
        ++banks;
    }

    if (banks == 0) return not_attached;

    enum { bank = 0x0000 };

    ctx.as.roml_r = [&](const u16& a, u8& d) {
        u32 bank_base = (ctx.mem[bank] * 0x2000) + 1;
        d = ctx.mem[bank_base + a];
    };

    bool exrom;
    bool game;

    const bool type_a = banks < 64;
    if (type_a) {
        ctx.as.romh_r = [&](const u16& a, u8& d) {
            u32 bank_base = (ctx.mem[bank] * 0x2000) + 1;
            d = ctx.mem[bank_base + a];
        };
        exrom = false;
        game = false;
    } else {
        exrom = false;
        game = true;
    }

    ctx.as.io1_w = [&](const u16& a, const u8& d) {
        if (a == 0x0e00) ctx.mem[bank] = d & 0b00111111;
    };

    ctx.reset = [&]() { ctx.mem[bank] = 0x00; };

    return exrom_game(exrom, game);
}


Result T10_Epyx_Fastload(const Files::CRT& crt, Ctx& ctx) {
    if (load_chips(crt, ctx) != 1) return not_attached;

    struct status {
        bool inactive() const { return _c.sys_cycle >= _inact_at(); }
        void activate()       { _inact_at() = _c.sys_cycle + 512; }

        Ctx& _c;
        u64& _inact_at() const { return *(u64*)&_c.mem[0x2000]; }
    };

    ctx.as.roml_r = [&](const u16& a, u8& d) {
        if (status{ctx}.inactive()) {
            d = ctx.sys_ram[0x8000 + a];
        } else {
            d = ctx.mem[a];
            status{ctx}.activate();
        }
    };

    ctx.as.io1_r = [&](const u16& a, u8& d) { UNUSED(a); UNUSED(d);
        status{ctx}.activate();
    };
    ctx.as.io2_r = [&](const u16& a, u8& d) {
        d = ctx.mem[0x1f00 | (a & 0x00ff)];
    };

    ctx.reset = [&]() { status{ctx}.activate(); };

    return exrom_game(false, bool(crt.header().game));
}


Result T16_Warp_Speed(const Files::CRT& crt, Ctx& ctx) {
    if (load_chips(crt, ctx) != 1) return not_attached; // TODO: verify the size (0x4000)

    struct active {
        operator bool() const { return _c.mem[0x4000]; }
        void operator=(bool act) {
            _c.mem[0x4000] = act;
            _c.as.exrom_game(!act, !act);
        }

        Ctx& _c;
    };

    ctx.as.roml_r = [&](const u16& a, u8& d) {
        d = active{ctx}
                ? ctx.mem[0x0000 + a]
                : ctx.sys_ram[0x8000 + a];
    };
    ctx.as.romh_r = [&](const u16& a, u8& d) {
        d = active{ctx}
                ? ctx.mem[0x2000 + a]
                : ctx.sys_ram[0xa000 + a];
    };

    ctx.as.io1_r = [&](const u16& a, u8& d) {
        d = ctx.mem[0x1e00 | (a & 0x00ff)];
    };
    ctx.as.io2_r = [&](const u16& a, u8& d) {
        d = ctx.mem[0x1f00 | (a & 0x00ff)];
    };
    ctx.as.io1_w = [&](const u16& a, const u8& d) { UNUSED(a); UNUSED(d);
        active{ctx} = true;
    };
    ctx.as.io2_w = [&](const u16& a, const u8& d) { UNUSED(a); UNUSED(d);
        active{ctx} = false;
    };

    ctx.reset = [&]() { active{ctx} = true; };

    return exrom_game(false, false);
}


Result T51_MACH_5(const Files::CRT& crt, Ctx& ctx) {
    if (load_chips(crt, ctx) != 1) return not_attached;

    struct active {
        void operator=(bool act) { _c.as.exrom_game(!act, true); }
        Ctx& _c;
    };

    ctx.as.roml_r = [&](const u16& a, u8& d) { d = ctx.mem[a]; };

    ctx.as.io1_r = [&](const u16& a, u8& d) { d = ctx.mem[0x1e00 | (a & 0x00ff)]; };
    ctx.as.io2_r = [&](const u16& a, u8& d) { d = ctx.mem[0x1f00 | (a & 0x00ff)]; };
    ctx.as.io1_w = [&](const u16& a, const u8& d) { UNUSED(a); UNUSED(d);
        active{ctx} = true;
    };
    ctx.as.io2_w = [&](const u16& a, const u8& d) { UNUSED(a); UNUSED(d);
        active{ctx} = false;
    };

    ctx.reset = [&]() { active{ctx} = true; };

    return exrom_game(crt);
}


bool Cartridge::attach(const Files::CRT& crt, Ctx& exp_ctx) {
    detach(exp_ctx);

    Result result;
    switch (const auto type = crt.header().hw_type; type) {
        using T = Files::CRT::Cartridge_HW_type;

        case T::T0_Normal_cartridge:
            result = T0_Normal_cartridge(crt, exp_ctx);
            break;
        case T::T4_Simons_BASIC:
            result = T4_Simons_BASIC(crt, exp_ctx);
            break;
        case T::T5_Ocean_type_1:
            result = T5_Ocean_type_1(crt, exp_ctx);
            break;
        case T::T10_Epyx_Fastload:
            result = T10_Epyx_Fastload(crt, exp_ctx);
            break;
        case T::T16_Warp_Speed:
            result = T16_Warp_Speed(crt, exp_ctx);
            break;
        case T::T51_MACH_5:
            result = T51_MACH_5(crt, exp_ctx);
            break;
        default:
            std::cout << "CRT: unsupported HW type: " << (int)type << std::endl;
            break;
    }

    if (result) {
        exp_ctx.as.exrom_game((*result).exrom, (*result).game);
        return true;
    }

    return false;
}


void Cartridge::detach(Ctx& exp_ctx) {
    // reading unconnected areas should return whatever is 'floating'
    // on the bus, so this is not 100% accurate, but meh....
    static Expansion_ctx::Address_space::r null_r { [](const u16& a, u8& d) { UNUSED(a); UNUSED(d); } };
    static Expansion_ctx::Address_space::w null_w { [](const u16& a, const u8& d) { UNUSED(a); UNUSED(d); } };

    exp_ctx.as.roml_r = null_r;
    exp_ctx.as.roml_w = null_w;
    exp_ctx.as.romh_r = null_r;
    exp_ctx.as.romh_w = null_w;

    exp_ctx.as.io1_r = null_r;
    exp_ctx.as.io1_w = null_w;
    exp_ctx.as.io2_r = null_r;
    exp_ctx.as.io2_w = null_w;

    exp_ctx.as.exrom_game(true, true);

    exp_ctx.reset = [](){};
}
