#ifndef SYSTEM_H_INCLUDED
#define SYSTEM_H_INCLUDED


#include <vector>
#include "common.h"
#include "state.h"
#include "utils.h"
#include "dbg.h"
#include "nmos6502/nmos6502_core.h"
#include "vic_ii.h"
#include "sid.h"
#include "cia.h"
#include "c1541.h"
#include "host.h"
#include "menu.h"
#include "files.h"
#include "expansion.h"



namespace System {

using CPU = NMOS6502::Core; // 6510 IO port (addr 0&1) is implemented externally (in Address_space)
using CIA = typename CIA::Core;
using TheSID = reSID_Wrapper; // 'The' due to nameclash
using VIC = VIC_II::Core;
using Color_RAM = VIC_II::Color_RAM;


struct ROM {
    const u8* basic;
    const u8* kernal;
    const u8* charr;
    const u8* c1541;
};


class Address_space {
public:
    using State = State::System::Adress_space;

    Address_space(
        State& s_,
        u8* ram_, const ROM& rom_,
        CIA& cia1_, CIA& cia2_, TheSID& sid_, VIC& vic_,
        Color_RAM& col_ram_, Expansion_ctx::IO& exp_)
      :
        s(s_),
        ram(ram_), rom(rom_),
        cia1(cia1_), cia2(cia2_), sid(sid_), vic(vic_),
        col_ram(col_ram_), exp(exp_) {}

    void reset() {
        s.io_port_pd = s.io_port_state = 0x00;
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
        switch (PLA[s.pla_line].pl[rw][addr >> 12]) {
            case ram0_r:
                data = (addr > 0x0001)
                            ? ram[addr]
                            : ((addr == 0x0000) ? r_dd() : r_pd());
                return;
            case ram0_w:
                if (addr > 0x0001) ram[addr] = data;
                else (addr == 0x0000) ? w_dd(data) : w_pd(data);
                return;
            case ram_r:   data = ram[addr];                 return; // 64 KB
            case ram_w:   ram[addr] = data;                 return;
            case bas_r:   data = rom.basic[addr & 0x1fff];  return; // 8 KB
            case kern_r:  data = rom.kernal[addr & 0x1fff]; return;
            case charr_r: data = rom.charr[addr & 0x0fff];  return; // 4 KB
            case roml_r:  exp.roml_r(addr & 0x1fff, data);  return; // 8 KB
            case roml_w:  exp.roml_w(addr & 0x1fff, data);  return;
            case romh_r:  exp.romh_r(addr & 0x1fff, data);  return;
            case romh_w:  exp.romh_w(addr & 0x1fff, data);  return;
            case io_r:    r_io(addr & 0x0fff, data);        return; // 4 KB
            case io_w:    w_io(addr & 0x0fff, data);        return;
            case none_r:  // TODO: anything..? (return 'floating' state..)
            case none_w:                                    return;
        }
    }

    void set_exrom_game(bool exrom, bool game) {
        s.exrom_game = (exrom << 4) | (game << 3);
        set_pla();

        vic.set_ultimax(exrom && !game);
    }

private:
    enum IO_bits : u8 {
        loram_hiram_charen_bits = 0x07,
        cassette_switch_sense = 0x10,
        cassette_motor_control = 0x20,
    };

    State& s;

    u8 r_dd() const { return s.io_port_dd; }
    u8 r_pd() const {
        static constexpr u8 pull_up = IO_bits::loram_hiram_charen_bits | IO_bits::cassette_switch_sense;
        const u8 pulled_up = ~s.io_port_dd & pull_up;
        const u8 cmc = ~cassette_motor_control | s.io_port_dd; // dd input -> 0
        return (s.io_port_state | pulled_up) & cmc;
    }

    void w_dd(const u8& dd) { s.io_port_dd = dd; update_state(); }
    void w_pd(const u8& pd) { s.io_port_pd = pd; update_state(); }

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

    void update_state();

    void set_pla();

    // There is a PLA_Line for each mode, and for each mode there are separate r/w configs
    struct PLA_Line {
        const u8 pl[2][16]; // [w/r][bank]
    };

    static const PLA_Line PLA[14];       // 14 unique configs
    static const u8 Mode_to_PLA_line[32]; // map: mode --> mode_idx (in PLA[])

    u8* ram;
    const ROM& rom;

    CIA& cia1;
    CIA& cia2;
    TheSID& sid;
    VIC& vic;
    Color_RAM& col_ram;

    Expansion_ctx::IO& exp;
};


class Int_hub {
private:
    using State = State::System::Int_hub;

    State& s;
public:
    Int_hub(State& s_) : s(s_), int_sig(s_.state) {}

    void reset() { s.state = s.old_state = 0x00; s.nmi_act = s.irq_act = false; }

    IO::Int_sig int_sig;

    void tick(CPU& cpu) {
        if (s.state != s.old_state) {
            s.old_state = s.state;

            bool act = (s.state & IO::Int_sig::Src::nmi);
            if (s.nmi_act != act) {
                s.nmi_act = act;
                cpu.set_nmi(s.nmi_act);
                int_sig.clr(IO::Int_sig::Src::rstr); // auto clr (pulse)
            }

            act = (s.state & IO::Int_sig::Src::irq);
            if (s.irq_act != act) {
                s.irq_act = act;
                cpu.set_irq(s.irq_act);
            }
        }
    }
};


struct Performance {
    struct Latency_settings {
        // @sync point: time syncing, input polling, and audio output
        u16 sync_freq; // divisor of FRAME_CYCLE_COUNT
        u16 audio_buf_sz;

        bool operator==(const Latency_settings& l) const {
            return (l.sync_freq == sync_freq) && (l.audio_buf_sz == audio_buf_sz);
        }
    };

    Choice<double> frame_rate{
        {FRAME_RATE_PAL, FRAME_RATE_MAX, FRAME_RATE_MIN},
        {
            to_string(FRAME_RATE_PAL, 3) + " (PAL)",
            to_string(FRAME_RATE_MAX, 3),
            to_string(FRAME_RATE_MIN, 3),
        },
    };

    Choice<bool> audio_pitch_shift{
        {false, true},
        {"FIXED", "SHIFT"},
    };

    static constexpr int min_sync_points = 1;
    Choice<Latency_settings> latency{
        {
            { FRAME_CYCLE_COUNT / 8, 128 },
            { FRAME_CYCLE_COUNT / 4, 256 },
            { FRAME_CYCLE_COUNT / 2, 512 },
            { FRAME_CYCLE_COUNT / min_sync_points, 1024 },
            //{ FRAME_CYCLE_COUNT / 12, 128 },
            //{ FRAME_CYCLE_COUNT / 24, 128 },
        },
        {"FRAME/8", "FRAME/4", "FRAME/2", "1 FRAME"/*, "FRAME/12"*//*, "FRAME/24"*/},
    };
};


class Input_matrix {
public:
    using State = State::System::Input_matrix;

    Input_matrix(State& s_, IO::Port::PD_in& pa_in_, IO::Port::PD_in& pb_in_, VIC& vic_)
        : s(s_), pa_in(pa_in_), pb_in(pb_in_), vic(vic_) {}

    void reset() {
        s.key_states = s.kb_matrix = 0;
        s.pa_state = s.pb_state = s.cp2_state = s.cp1_state = 0b11111111;
    }

    void cia1_pa_out(u8 state) { s.pa_state = state; output(); }
    void cia1_pb_out(u8 state) { s.pb_state = state; output(); }

    Sig_key keyboard {
        [this](u8 code, u8 down) {
            const auto key = u64{0b1} << (63 - code);
            s.key_states = down ? s.key_states | key : s.key_states & ~key;
            update_matrix();
            output();
        }
    };

    Sig_key ctrl_port_1 {
        [this](u8 code, u8 down) {
            const u8 bit_pos = 0b1 << code;
            const u8 bit_val = down ? 0 : bit_pos;
            s.cp1_state = (s.cp1_state & ~bit_pos) | bit_val;
            output();
        }
    };

    Sig_key ctrl_port_2 {
        [this](u8 code, u8 down) {
            const u8 bit_pos = 0b1 << code;
            const u8 bit_val = down ? 0 : bit_pos;
            s.cp2_state = (s.cp2_state & ~bit_pos) | bit_val;
            output();
        }
    };

private:
    void update_matrix();
    void output();

    State& s;

    IO::Port::PD_in& pa_in;
    IO::Port::PD_in& pb_in;
    VIC& vic; // TODO: replace with a signal?
};


struct Video_overlay {
    Video_overlay(u16 x_, u16 y_, u16 w_, u16 h_, const u8* charrom_)
        : x(x_), y(y_), w(w_), h(h_), pixels(new u8[w * h]), charrom(charrom_) {}
    ~Video_overlay() { delete[] pixels; }

    // static constexpr u8 transparent = 0xff;

    const u16 x; const u16 y; const u16 w; const u16 h;

    void clear(u8 col) { for (int p = 0; p < w * h; ++p) pixels[p] = col; }
    void draw_chr(u16 chr, u16 cx, u16 cy, Color fg, Color bg);

    bool visible = false;
    u8* pixels;
    const u8* charrom;
};


class Menu {
public:
    static const int pos_x = VIC_II::BORDER_SZ_V + 1 * 8;
    static const int pos_y = (VIC_II::FRAME_HEIGHT - VIC_II::BORDER_SZ_H) + 12;

    static const Color col_fg = Color::light_green;
    static const Color col_bg = Color::gray_1;

    Menu(std::initializer_list<std::pair<std::string, std::function<void ()>>> imm_actions_,
        std::initializer_list<std::pair<std::string, std::function<void ()>>> actions_,
        std::initializer_list<::Menu::Group> subs_,
        const u8* charrom);

    void handle_key(u8 code);
    void toggle_visibility();

    Video_overlay video_overlay;

private:
    ::Menu::Group root{""};
    std::vector<::Menu::Kludge> imm_actions;
    std::vector<::Menu::Action> actions;

    std::vector<::Menu::Group> subs;

    void update();
};


struct C1541_status_panel {
    static const int pos_x = VIC_II::FRAME_WIDTH - VIC_II::BORDER_SZ_V - 16;
    static const int pos_y = (VIC_II::FRAME_HEIGHT - VIC_II::BORDER_SZ_H) + 12;

    static const Color col_bg = Color::black;
    static const Color col_led_on = Color::light_green;
    static const Color col_led_off = Color::gray_1;

    Video_overlay video_overlay;

    C1541_status_panel(const u8* charrom) : video_overlay(pos_x, pos_y, 8, 8, charrom) {}

    void update(const C1541::Disk_ctrl::Status& c1541_status);
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

    C64(const ROM& rom_) : rom(rom_)
    {
        // intercept load/save for tape device
        install_kernal_tape_traps(const_cast<u8*>(rom.kernal), Trap_OPC::tape_routine);

        Cartridge::detach(exp_ctx);
    }

    void run() {
        reset_cold();

        // TODO: consider special loops for different configs (e.g. REU with no 1541)
        for (shutdown = false; !shutdown;) {
            vic.tick();

            const auto rw = cpu.mrw();
            const auto rdy_low = s.ba_low || s.dma_low;
            if (rdy_low && rw == NMOS6502::MC::RW::r) {
                c1541.tick();
            } else {
                // 'Slip in' the C1541 cycle
                if (rw == NMOS6502::MC::RW::w) c1541.tick();
                addr_space.access(cpu.mar(), cpu.mdr(), rw);
                cpu.tick();
                if (rw == NMOS6502::MC::RW::r) c1541.tick();
            }

            if (exp_ctx.tick) exp_ctx.tick();

            cia1.tick();
            cia2.tick();

            int_hub.tick(cpu);

            if (s.vic.cycle % C1541::extra_cycle_freq == 0) c1541.tick();

            check_sync();
        }
    }

private:
    using Action = std::function<void()>;

    Files::System_snapshot sys_snap;

    const ROM& rom;

    State::System& s{sys_snap.sys_state};

    CPU cpu{s.cpu, cpu_trap};

    CIA cia1{s.cia1, cia1_pa_out, cia1_pb_out, int_hub.int_sig, IO::Int_sig::Src::cia1, s.vic.cycle};
    CIA cia2{s.cia2, cia2_pa_out, cia2_pb_out, int_hub.int_sig, IO::Int_sig::Src::cia2, s.vic.cycle};

    TheSID sid{int(FRAME_RATE_MIN), Performance::min_sync_points, s.vic.cycle};

    Color_RAM col_ram{s.color_ram};

    VIC vic{s.vic, s.ram, col_ram, rom.charr, exp_io.romh_r, s.ba_low, int_hub.int_sig};

    Address_space addr_space{s.addr_space, s.ram, rom, cia1, cia2, sid, vic, col_ram, exp_io};

    Int_hub int_hub{s.int_hub};

    IO::Bus bus{cpu};

    Input_matrix input_matrix{s.input_matrix, cia1.port_a.ext_in, cia1.port_b.ext_in, vic};

    Expansion_ctx::IO exp_io{
        s.ba_low,
        //std::bind(&Address_space::access, addr_space, std::placeholders::_1),
        [this](const u16& a, u8& d, const u8 rw) { addr_space.access(a, d, rw); },
        [this](bool e, bool g) { addr_space.set_exrom_game(e, g); },
        int_hub.int_sig,
        s.dma_low
    };
    Expansion_ctx exp_ctx{
        exp_io, bus, s.ram, s.vic.cycle, s.exp_ram, nullptr, nullptr
    };

    C1541::System c1541{cia2.port_a.ext_in, rom.c1541};

    IO::Port::PD_out cia1_pa_out {
        [this](u8 state) { input_matrix.cia1_pa_out(state); }
    };
    IO::Port::PD_out cia1_pb_out {
        [this](u8 state) { input_matrix.cia1_pb_out(state); }
    };
    IO::Port::PD_out cia2_pa_out {
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
    IO::Port::PD_out cia2_pb_out { [](u8 _) { UNUSED(_); } };

    Host::Input::Handlers host_input_handlers {
        // client keyboard & controllers (including lightpen)
        input_matrix.keyboard,
        input_matrix.ctrl_port_1,
        input_matrix.ctrl_port_2,

        // system keys
        [this](u8 code, u8 down) {
            using kc = Key_code::System;

            if (!down) {
                if (code == kc::rstre) int_hub.int_sig.set(IO::Int_sig::Src::rstr);
                return;
            }

            switch (code) {
                case kc::shutdown:  request_shutdown();           break;
                case kc::rst_c:     reset_cold();                 break;
                case kc::v_fsc:     vid_out.toggle_fullscr_win(); break;
                case kc::swp_j:     host_input.swap_joysticks();  break;
                case kc::menu_tgl:  menu.toggle_visibility();     break;
                case kc::rot_dsk:   c1541.disk_carousel.rotate(); break;
                case kc::menu_ent:
                case kc::menu_back:
                case kc::menu_up:
                case kc::menu_down: menu.handle_key(code);        break;
            }
        },

        // file drop
        [this](const char* filepath) {
            Log::info("Incoming: '%s'", filepath);
            auto file = Files::read(filepath);
            if (file.identified() && file.type != Files::File::Type::c64_bin) {
                handle_file(file);
            } else {
                Log::info("Ignored: '%s'", filepath);
            }
        }
    };
    Host::Input host_input{host_input_handlers};

    Host::Video_out vid_out{perf.frame_rate.chosen};

    // TODO: verify that it is a valid kernal trap (e.g. 'addr_space.mapping(cpu.pc) == kernal')
    NMOS6502::Sig cpu_trap {
        [this]() {
            bool proceed = true;

            const auto trap_opc = cpu.s.ir;
            switch (trap_opc) {
                /*case Trap_OPC::IEC_virtual_routine:
                    handled = IEC_virtual::on_trap(c64.cpu, c64.s.ram, iec_ctrl);
                    break;*/
                case Trap_OPC::tape_routine: {
                    const auto routine_id = cpu.s.d;

                    switch (routine_id) {
                        case Trap_ID::load: do_load(); break;
                        case Trap_ID::save: do_save(); break;
                        default:
                            proceed = false;
                            Log::error("Unknown tape routine: %d", (int)routine_id);
                            break;
                    }
                    break;
                }
                default:
                    proceed = false;
                    break;
            }

            if (proceed) {
                cpu.resume();
            } else {
                Log::error("****** CPU halted! ******");
                Dbg::print_status(cpu, s.ram);
            }
        }
    };

    Files::Loader loader{Files::init_loader("./_local")}; // TODO: init_path configurable (program arg?)

    Performance perf{};

    bool shutdown;

    Action when_frame_done;

    _Stopwatch watch;
    Timer frame_timer;

    void init_sync(); // call if system has been 'paused'
    void check_sync() {
        const bool sync_point = (s.vic.cycle % perf.latency.chosen.sync_freq) == 0;
        if (sync_point) sync();
    }
    void sync();

    void output_frame();

    void reset_warm();
    void reset_cold();

    void save_state_req();

    bool handle_file(Files::File& file);

    void do_load();
    void do_save();

    void init_ram();

    void request_shutdown() {
        Log::info("Shutdown requested");
        shutdown = true;
    }

    std::vector<::Menu::Action> cart_menu_actions{
        {"DETACH ?", [&](){ Cartridge::detach(exp_ctx); reset_cold(); }},
        {"ATTACH REU ?", [&]() { if ( Cartridge::attach_REU(exp_ctx)) reset_cold(); }},
    };

    std::vector<::Menu::Knob> perf_menu_items{
        {"FPS",  perf.frame_rate,
            [&]() {
                vid_out.reconfig();
                sid.reconfig(perf.frame_rate, perf.audio_pitch_shift);
            }
        },
        {"AUDIO PITCH", perf.audio_pitch_shift,
            [&]() {
                sid.reconfig(perf.frame_rate, perf.audio_pitch_shift);
            }
        },
        {"LATENCY", perf.latency,
            [&]() {
                sid.reconfig(perf.latency.chosen.audio_buf_sz);
            }
        },
    };

    Menu menu{
        {
            {"RESET WARM !", [&](){ reset_warm(); } },
            {"RESET COLD !", [&](){ reset_cold(); } },
            {"SWAP JOYS !",  [&](){ host_input.swap_joysticks(); } },
            {"SAVE STATE !", [&](){ save_state_req(); } },
        },
        {
            {"SHUTDOWN ?",   [&](){ request_shutdown(); } },
        },
        {
            vid_out.settings_menu(),
            sid.settings_menu(),
            c1541.menu(),
            ::Menu::Group("CARTRIDGE / ", cart_menu_actions),
            ::Menu::Group("PERFORMANCE / ", perf_menu_items),
        },
        rom.charr
    };

    C1541_status_panel c1541_status_panel{rom.charr};

    static void install_kernal_tape_traps(u8* kernal, u8 trap_opc);
};


} // namespace System


#endif // SYSTEM_H_INCLUDED
