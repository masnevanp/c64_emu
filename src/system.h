#ifndef SYSTEM_H_INCLUDED
#define SYSTEM_H_INCLUDED


#include <cmath>
#include <vector>
#include <filesystem>
#include "common.h"
#include "utils.h"
#include "dbg.h"
#include "nmos6502/nmos6502_core.h"
#include "vic_ii.h"
#include "sid.h"
#include "cia.h"
#include "io.h"
#include "c1541.h"
#include "host.h"
#include "menu.h"
#include "files.h"
#include "cartridge.h"



namespace System {

class VIC_out;

using CPU = NMOS6502::Core; // 6510 IO port (addr 0&1) is implemented externally (in Address_space)
using CIA = CIA::Core;
using TheSID = reSIDfp_Wrapper; // 'The' due to nameclash
using VIC = VIC_II::Core<VIC_out>;
using Color_RAM = VIC_II::Color_RAM;



// sync.freq. in raster lines (must divide into total line count 312)
static const int SYNC_FREQ = 52;  // ==> ~3.3 ms

static const int MENU_Y = (VIC_II::FRAME_HEIGHT - VIC_II::BORDER_SZ_H) + 3;


struct ROM {
    const u8* basic;
    const u8* kernal;
    const u8* charr;
    const u8* c1541;
};


class Address_space {
public:
    Address_space(
        u8* ram_, const ROM& rom,
        CIA& cia1_, CIA& cia2_, TheSID& sid_, VIC& vic_,
        Color_RAM& col_ram_)
      :
        ram(ram_), bas(rom.basic), kern(rom.kernal), charr(rom.charr),
        cia1(cia1_), cia2(cia2_), sid(sid_), vic(vic_),
        col_ram(col_ram_)
    {
        exp.exrom_game = [&](bool e, bool g) { set_exrom_game(e, g); };
    }

    void reset() {
        io_port_state = 0x00;
        w_dd(0x00); // all inputs
    }

    // NOTE: Ultimax config --> writes also directed to ROM
    enum Mapping : u8 {
        ram0_r,  ram0_w, // Special mapping for RAM bank 0 due to IO port handling (addr 0&1)
        ram_r,   ram_w,
        bas_r,
        kern_r,
        charr_r,
        roml_r,  roml_w,
        romh_r,  romh_w,
        io_r,    io_w,
        none_r,  none_w,
    };


    void access(const u16& addr, u8& data, const u8 rw) {
        switch (pla->pl[rw][addr >> 12]) {
            case ram0_r:
                data = (addr > 0x0001)
                            ? ram[addr]
                            : ((addr == 0x0000) ? r_dd() : r_pd());
                return;
            case ram0_w:
                if (addr > 0x0001) ram[addr] = data;
                else (addr == 0x0000) ? w_dd(data) : w_pd(data);
                return;
            case ram_r:   data = ram[addr];                return; // 64 KB
            case ram_w:   ram[addr] = data;                return;
            case bas_r:   data = bas[addr & 0x1fff];       return; // 8 KB
            case kern_r:  data = kern[addr & 0x1fff];      return;
            case charr_r: data = charr[addr & 0x0fff];     return; // 4 KB
            case roml_r:  exp.roml_r(addr & 0x1fff, data); return; // 8 KB
            case roml_w:  exp.roml_w(addr & 0x1fff, data); return;
            case romh_r:  exp.romh_r(addr & 0x1fff, data); return;
            case romh_w:  exp.romh_w(addr & 0x1fff, data); return;
            case io_r:    r_io(addr & 0x0fff, data);       return; // 4 KB
            case io_w:    w_io(addr & 0x0fff, data);       return;
            case none_r:  // TODO: anything..? (return 'floating' state..)
            case none_w:                                   return;
        }
    }

    Expansion_ctx::Address_space exp;

private:
    enum IO_bits : u8 {
        loram_hiram_charen_bits = 0x07,
        cassette_switch_sense = 0x10,
        cassette_motor_control = 0x20,
    };

    u8 io_port_dd;
    u8 io_port_pd;
    u8 io_port_state;

    u8 exrom_game;

    u8 r_dd() const { return io_port_dd; }
    u8 r_pd() const {
        static const u8 pull_up = IO_bits::loram_hiram_charen_bits | IO_bits::cassette_switch_sense;
        u8 pulled_up = ~io_port_dd & pull_up;
        u8 cmc = ~cassette_motor_control | io_port_dd; // dd input -> 0
        return (io_port_state | pulled_up) & cmc;
    }

    void w_dd(const u8& dd) { io_port_dd = dd; update_state(); }
    void w_pd(const u8& pd) { io_port_pd = pd; update_state(); }

    void r_io(const u16& addr, u8& data) {
        switch (addr >> 8) {
            case 0x0: case 0x1: case 0x2: case 0x3: vic.r(addr & 0x003f, data);     return;
            case 0x4: case 0x5: case 0x6: case 0x7: sid.r(addr & 0x001f, data);     return;
            case 0x8: case 0x9: case 0xa: case 0xb: col_ram.r(addr & 0x03ff, data); return;
            case 0xc:                               cia1.r(addr & 0x000f, data);    return;
            case 0xd:                               cia2.r(addr & 0x000f, data);    return;
            case 0xe:                               exp.io1_r(addr, data);          return;
            case 0xf:                               exp.io2_r(addr, data);          return;
        }
    }

    void w_io(const u16& addr, const u8& data) {
        switch (addr >> 8) {
            case 0x0: case 0x1: case 0x2: case 0x3: vic.w(addr & 0x003f, data);     return;
            case 0x4: case 0x5: case 0x6: case 0x7: sid.w(addr & 0x001f, data);     return;
            case 0x8: case 0x9: case 0xa: case 0xb: col_ram.w(addr & 0x03ff, data); return;
            case 0xc:                               cia1.w(addr & 0x000f, data);    return;
            case 0xd:                               cia2.w(addr & 0x000f, data);    return;
            case 0xe:                               exp.io1_w(addr, data);          return;
            case 0xf:                               exp.io2_w(addr, data);          return;
        }
    }

    void set_exrom_game(bool exrom, bool game) {
        exrom_game = (exrom << 4) | (game << 3);
        set_pla();

        vic.set_ultimax(exrom && !game);
    }

    void update_state() { // output bits set from 'pd', input bits unchanged
        io_port_state = (io_port_pd & io_port_dd) | (io_port_state & ~io_port_dd);
        set_pla();
    }

    void set_pla() {
        const u8 lhc = (io_port_state | ~io_port_dd) & loram_hiram_charen_bits; // inputs -> pulled up
        const u8 mode = exrom_game | lhc;
        pla = &PLA[Mode_to_PLA_idx[mode]];
    }

    // There is a PLA_Line for each mode, and for each mode there are separate r/w configs
    struct PLA_Line {
        const u8 pl[2][16]; // [w/r][bank]
    };

    static const PLA_Line PLA[14];       // 14 unique configs
    static const u8 Mode_to_PLA_idx[32]; // map: mode --> mode_idx (in PLA[])

    const PLA_Line* pla; // the active PLA line (set based on mode)

    u8* ram;

    const u8* bas;
    const u8* kern;
    const u8* charr;

    CIA& cia1;
    CIA& cia2;
    TheSID& sid;
    VIC& vic;
    Color_RAM& col_ram;
};


struct VIC_out_overlay {
    // static constexpr u8 transparent = 0xff;

    const u16 x; const u16 y; const u16 w; const u16 h;
    bool visible = false;

    u8* pixels;

    VIC_out_overlay(u16 x_, u16 y_, u16 w_, u16 h_) : x(x_), y(y_), w(w_), h(h_), pixels(new u8[w * h]) {}
    ~VIC_out_overlay() { delete[] pixels; }

    void clear(u8 col) { for (int p = 0; p < w * h; ++p) pixels[p] = col; }
};

using Overlay = VIC_out_overlay;


class C1541_status_panel {
public:
    static constexpr Color col_bg = Color::black;

    C1541_status_panel(const C1541::Disk_ctrl::Status& status_, const u8* charrom_)
        : status(status_), charrom(charrom_) { overlay.clear(col_bg); }

    void update() {
        if (overlay.visible) draw();
        overlay.visible = status.head.active();
    }

private:
    friend class VIC_out;

    const C1541::Disk_ctrl::Status& status;
    const u8* charrom;

    Overlay overlay{
        VIC_II::FRAME_WIDTH - VIC_II::BORDER_SZ_V - 4 * 8 - 2, MENU_Y + 5,
        4 * 8, (1 * 8) + 1
    };

    void draw();
};


class VIC_out {
public:
    VIC_out(
        IO::Int_hub& int_hub_, Host::Video_out& vid_out_, Host::Input& host_input_,
        TheSID& sid_, Overlay& menu_overlay_, C1541_status_panel& c1541_status_
    ) :
        int_hub(int_hub_), vid_out(vid_out_), host_input(host_input_),
        sid(sid_), menu_overlay(menu_overlay_), c1541_status(c1541_status_) { }

    void set_irq() { int_hub.set(IO::Int_hub::Src::vic); }
    void clr_irq() { int_hub.clr(IO::Int_hub::Src::vic); }

    _Stopwatch watch;
    void init_sync() { // call if system has been 'paused'
        vid_out.flip();
        sid.flush();
        frame_timer.reset();
        watch.start();
    }

    void frame(u8* frame);

    void sync(u16 line) { // NOTE: not called on the 'frame done' line
        if (line % SYNC_FREQ == 0) {
            if (!frame_skip) {
                host_input.poll();
                auto frame_progress = double(line) / double(VIC_II::FRAME_LINE_COUNT);
                auto sync_moment = frame_moment + (frame_progress * VIC_II::FRAME_MS);
                watch.stop();
                frame_timer.sync(std::round(sync_moment));
                watch.start();
            }
            sid.output();
        }
    }

private:
    IO::Int_hub& int_hub;

    Host::Video_out& vid_out;
    Host::Input& host_input;

    TheSID& sid;

    Overlay& menu_overlay;
    C1541_status_panel& c1541_status;

    Timer frame_timer;
    double frame_moment = 0;
    int frame_skip = 0;
};


class Input_matrix {
public:
    Input_matrix(IO::Port::PD_in& pa_in_, IO::Port::PD_in& pb_in_, VIC& vic_)
        : pa_in(pa_in_), pb_in(pb_in_), vic(vic_) {}

    void reset() {
        key_states = kb_matrix = 0;
        pa_state = pb_state = cp2_state = cp1_state = 0b11111111;
    }

    void cia1_pa_out(u8 state) { pa_state = state; output(); }
    void cia1_pb_out(u8 state) { pb_state = state; output(); }

    Sig_key keyboard {
        [this](u8 code, u8 down) {
            const auto key = u64{0b1} << (63 - code);
            key_states = down ? key_states | key : key_states & ~key;
            update_matrix();
            output();
        }
    };

    Sig_key ctrl_port_1 {
        [this](u8 code, u8 down) {
            const u8 bit_pos = 0b1 << code;
            const u8 bit_val = down ? 0 : bit_pos;
            cp1_state = (cp1_state & ~bit_pos) | bit_val;
            output();
        }
    };

    Sig_key ctrl_port_2 {
        [this](u8 code, u8 down) {
            const u8 bit_pos = 0b1 << code;
            const u8 bit_val = down ? 0 : bit_pos;
            cp2_state = (cp2_state & ~bit_pos) | bit_val;
            output();
        }
    };

private:
    void update_matrix() {
        auto get_row = [](int n, u64& from) -> u64 { return (from >> (8 * n)) & 0xff; };
        auto set_row = [](int n, u64& to, u64 val) { to |= (val << (8 * n)); };

        kb_matrix = key_states;

        if (kb_matrix) { // any key down?
            // emulate GND propagation in the matrix (can produce 'ghost' keys)
            for (int ra = 0; ra < 7; ++ra) for (int rb = ra + 1; rb < 8; ++rb) {
                const u64 a = get_row(ra, kb_matrix);
                const u64 b = get_row(rb, kb_matrix);
                if (a & b) {
                    const u64 r = a | b;
                    set_row(ra, kb_matrix, r);
                    set_row(rb, kb_matrix, r);
                }
            }
        }
    }

    void output() {
        u8 pa = pa_state & cp2_state;
        u8 pb = pb_state & cp1_state;

        if (kb_matrix) { // any key down?
            u64 key = 0b1;
            for (int n = 0; n < 64; ++n, key <<= 1) {
                const bool key_down = kb_matrix & key;
                if (key_down) {
                    // figure out connected lines & states
                    const auto row_bit = (0b1 << (n / 8));
                    const auto pa_bit = pa & row_bit;
                    const auto col_bit = (0b1 << (n % 8));
                    const auto pb_bit = pb & col_bit;

                    if (!pa_bit || !pb_bit) { // at least one connected line is low?
                        // pull both lines low
                        pa &= (~row_bit);
                        pb &= (~col_bit);
                    }
                }
            }
        }

        pa_in(0b11111111, pa);
        pb_in(0b11111111, pb);

        vic.set_lp(VIC::LP_src::cia, pb & VIC_II::CIA1_PB_LP_BIT);
    }

    // key states, 8x8 bits (64 keys)
    u64 key_states; // 'actual' state
    u64 kb_matrix;  // key states in the matrix (includes possible 'ghost' keys)

    // current output state of the cia-ports (any input bit will be set)
    u8 pa_state;
    u8 pb_state;

    // current state of controllers
    u8 cp2_state;
    u8 cp1_state;

    IO::Port::PD_in& pa_in;
    IO::Port::PD_in& pb_in;
    VIC& vic;
};


class Menu {
public:
    static constexpr u8 col[2] = {Color::gray_1, Color::light_green}; // bg, fg

    Menu(std::initializer_list<std::pair<std::string, std::function<void ()>>> imm_actions_,
        std::initializer_list<std::pair<std::string, std::function<void ()>>> actions_,
        std::initializer_list<::Menu::Group> subs_,
        const u8* charrom);

    void handle_key(u8 code);
    void toggle_visibility();

private:
    friend class C64;

    ::Menu::Group main_menu{"^"};
    std::vector<::Menu::Kludge> imm_actions;
    std::vector<::Menu::Action> actions;

    std::vector<::Menu::Group> subs;

    ::Menu::Controller ctrl{&main_menu};

    const u8* charset;

    Overlay overlay{
        VIC_II::BORDER_SZ_V, MENU_Y,
        40 * 8, 2 * 8
    };

    void update();
};


struct State {
    u8 ram[0x10000];
    u8 color_ram[Color_RAM::size] = {};

    u16 rdy_low;

    VIC::State vic;

    // holds all the memory that an expansion (e.g. a cart) requires: ROM, RAM,
    // emulation control data, ...
    // TODO: dynamic, or meh..? (although 640k is enough for everyone...except easyflash..)
    //       (nasty, if state saving is implemented)
    u8 exp_mem[640 * 1024];
};


/*
    // intercepts kernal calls: untlk, talk, unlsn, listen, tksa, second, acptr, ciout
    // (fast but very low compatibility)
    IEC_virtual::Controller iec_ctrl;
    IEC_virtual::Volatile_disk vol_disk;
    IEC_virtual::Dummy_device dd;
    IEC_virtual::Host_drive hd(load_file);
    iec_ctrl.attach(vol_disk, 10);
    iec_ctrl.attach(dd, 30);
    iec_ctrl.attach(hd, 8);
    IEC_virtual::install_kernal_traps(kernal, Trap_OPC::IEC_virtual_routine);
*/

class C64 {
public:
    enum Trap_OPC { // Halting instuctions are used as traps
        //IEC_virtual_routine = 0x02,
        tape_routine = 0x12,
    };
    enum Trap_ID {
        load = 0x01, save = 0x02,
    };

    C64(const ROM& rom) :
        cia1(cia1_port_a_out, cia1_port_b_out, int_hub, IO::Int_hub::Src::cia1),
        cia2(cia2_port_a_out, cia2_port_b_out, int_hub, IO::Int_hub::Src::cia2),
        sid(s.vic.cycle),
        col_ram(s.color_ram),
        vic(s.vic, s.ram, col_ram, rom.charr, addr_space.exp.romh_r, s.rdy_low, vic_out),
        c1541(cia2.port_a.ext_in, rom.c1541/*, run_cfg_change*/),
        c1541_status_panel{c1541.dc.status, rom.charr},
        vid_out(),
        vic_out(int_hub, vid_out, host_input, sid, menu.overlay, c1541_status_panel),
        int_hub(cpu),
        input_matrix(cia1.port_a.ext_in, cia1.port_b.ext_in, vic),
        addr_space(s.ram, rom, cia1, cia2, sid, vic, col_ram),
        host_input(host_input_handlers),
        menu(
            {
                {"RESET WARM !", [&](){ reset_warm(); } },
                {"RESET COLD !", [&](){ reset_cold(); } },
                {"SWAP JOYS !",  [&](){ host_input.swap_joysticks(); } },
            },
            {
                {"SHUTDOWN ?",   [&](){ signal_shutdown(); } },
            },
            {
                vid_out.settings_menu(),
                sid.settings_menu(),
                c1541.menu(),
                ::Menu::Group("CARTRIDGE /", cart_menu_actions),
            },
            rom.charr
        )
    {
        // intercept load/save for tape device
        install_tape_kernal_traps(const_cast<u8*>(rom.kernal), Trap_OPC::tape_routine);

        loader = Files::Loader("data/prg", img_consumer); // TODO: init_dir configurable (program arg?)

        cpu.sig_halt = cpu_trap;

        Cartridge::detach(exp_ctx);
    }

    void reset_warm() {
        cia1.reset_warm(); // need to reset for correct irq handling
        cia2.reset_warm();
        cpu.reset_warm();
        int_hub.reset();
    }

    void reset_cold() {
        init_ram();
        cia1.reset_cold();
        cia2.reset_cold();
        sid.reset();
        vic.reset();
        input_matrix.reset();
        addr_space.reset();
        cpu.reset_cold();
        int_hub.reset();
        c1541.reset();
        exp_ctx.reset();

        s.rdy_low = false;
    }

    void run() {
        reset_cold();
        vic_out.init_sync();

        for (shutdown = false; !shutdown;) {
            vic.tick();

            const auto rw = cpu.mrw();
            if (s.rdy_low && rw == NMOS6502::MC::RW::r) {
                c1541.tick();
            } else {
                // 'Slip in' the C1541 cycle
                if (rw == NMOS6502::MC::RW::w) c1541.tick();
                addr_space.access(cpu.mar(), cpu.mdr(), rw);
                cpu.tick();
                if (rw == NMOS6502::MC::RW::r) c1541.tick();
            }

            cia1.tick(s.vic.cycle);
            cia2.tick(s.vic.cycle);
            int_hub.tick();
            if (s.vic.cycle % C1541::extra_cycle_freq == 0) c1541.tick();
        }

        /*for (shutdown = false; !shutdown;) {
            keep_running();
        }*/
    }

    State s;

    CPU cpu;

    CIA cia1;
    CIA cia2;

    TheSID sid;

    Color_RAM col_ram;
    VIC vic;

    C1541::System c1541;

private:
    C1541_status_panel c1541_status_panel;

    Host::Video_out vid_out;
    VIC_out vic_out;

    IO::Int_hub int_hub;

    Input_matrix input_matrix;

    Address_space addr_space;

    // TODO: lump these (and future related things) together
    //       (a simple 'Run_cfg' class that accepts requests to change the cfg (or to quit..))
    bool shutdown;
    // bool run_cfg_change;

    Host::Input host_input;

    /* -------------------- CIA port outputs -------------------- */
    IO::Port::PD_out cia1_port_a_out {
        [this](u8 state) { input_matrix.cia1_pa_out(state); }
    };
    IO::Port::PD_out cia1_port_b_out {
        [this](u8 state) { input_matrix.cia1_pb_out(state); }
    };
    IO::Port::PD_out cia2_port_a_out {
        [this](u8 state) {
            const u8 va14_va15 = state & 0b11;
            vic.set_bank(va14_va15);

            c1541.iec.cia2_pa_output(state);
            /*if (c1541.idle) {
                c1541.idle = false;
                run_cfg_change = true;
            }*/
        }
    };
    IO::Port::PD_out cia2_port_b_out { [](u8 _) { UNUSED(_); } };

    /* -------------------- Host input -------------------- */
    Host::Input::Handlers host_input_handlers {
        // client keyboard & controllers (including lightpen)
        input_matrix.keyboard,
        input_matrix.ctrl_port_1,
        input_matrix.ctrl_port_2,

        // system keys
        [this](u8 code, u8 down) {
            using kc = Key_code::System;

            if (!down) {
                if (code == kc::rstre) int_hub.set(IO::Int_hub::Src::rstr);
                return;
            }

            switch (code) {
                case kc::quit:      signal_shutdown();            break;
                case kc::rst_c:     reset_cold();                 break;
                case kc::v_fsc:     vid_out.toggle_fullscr_win(); break;
                case kc::swp_j:     host_input.swap_joysticks();  break;
                case kc::menu_tgl:  menu.toggle_visibility();     break;
                case kc::rot_dsk:   c1541.disk_carousel.rotate(); break;
                case kc::menu_ent:
                case kc::menu_up:
                case kc::menu_down: menu.handle_key(code);        break;
            }
        }
    };

    // TODO: verify that it is a valid kernal trap (e.g. 'addr_space.mapping(cpu.pc) == kernal')
    NMOS6502::Sig cpu_trap {
        [this]() {
            bool proceed = true;

            const auto trap_opc = cpu.ir;
            switch (trap_opc) {
                /*case Trap_OPC::IEC_virtual_routine:
                    handled = IEC_virtual::on_trap(c64.cpu, c64.s.ram, iec_ctrl);
                    break;*/
                case Trap_OPC::tape_routine: {
                    const auto routine_id = cpu.d;

                    switch (routine_id) {
                        case Trap_ID::load: do_load(); break;
                        case Trap_ID::save: do_save(); break;
                        default:
                            proceed = false;
                            std::cout << "\nUnknown tape routine: " << (int)routine_id;
                            break;
                    }
                    break;
                }
                default:
                    proceed = false;
                    break;
            }

            if (proceed) {
                ++cpu.mcp; // bump to recover from halt
            } else {
                std::cout << "\n****** CPU halted! ******\n";
                Dbg::print_status(cpu, s.ram);
            }
        }
    };

    Files::consumer img_consumer {
        [this](std::string name, Files::Type type, std::vector<u8>& data) {
            // std::cout << "File: " << path << ' ' << (int)data.size() << std::endl;

            switch (type) {
                case Files::Type::crt: {
                    Files::CRT crt{data};
                    if (crt.header().valid()) {
                        if (Cartridge::attach(crt, exp_ctx)) reset_cold();
                    } else {
                        std::cout << "CRT: invalid header" << std::endl;
                    }
                    break;
                }
                case Files::Type::d64:
                    // auto slot = s.ram[0xb9]; // secondary address
                    // slot=0 --> first free slot
                    c1541.disk_carousel.insert(0, new C1541::D64_disk(Files::D64{data}), name);
                    break;
                case Files::Type::g64:
                    c1541.disk_carousel.insert(0, new C1541::G64_disk(std::move(data)), name);
                    break;
                default:
                    return;
            }

        }
    };

    //void keep_running();

    void do_load();
    void do_save();

    void init_ram();
    void signal_shutdown() { /*run_cfg_change =*/ shutdown = true; }


    std::vector<::Menu::Action> cart_menu_actions{
        {"CARTRIDGE / DETACH ?", [&](){ Cartridge::detach(exp_ctx); reset_cold(); }},
    };

    Menu menu;

    Files::loader loader;

    Expansion_ctx exp_ctx{
        addr_space.exp, s.ram, s.vic.cycle, s.exp_mem, nullptr
    };

    static void install_tape_kernal_traps(u8* kernal, u8 trap_opc);
};


} // namespace System

/*
#include <iostream>
#include <cmath>

using namespace std;

double yy1 = 17734472.0;
double f   = yy1 / 18.0;
int cf     = 312 * 63;
double pf  = f / cf;
double fd  = 1000.0 / pf;

int main()
{
    for (int i = 0; i < 1000000; ++i) {
        double d = i * fd;
        double diff = std::abs(d - std::round(d));
        if (diff < 0.000001) {
            std::cout << (int)i << ": (" << (int)d << ")\n";
        }
    }

    return 0;
}
*/

#endif // SYSTEM_H_INCLUDED
