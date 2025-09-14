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

class Bus;

using CPU = NMOS6502::Core; // 6510 IO port (addr 0&1) is implemented externally (in Address_space)
using CIA = typename CIA::Core;
using TheSID = reSID_Wrapper; // 'The' due to nameclash
using VIC = VIC_II::Core<Bus>;


namespace PLA {
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

    // There is an array for each mode, and for each mode there are separate r/w configs
    struct Array {
        const Mapping pl[2][16]; // [w/r][bank]
    };

    extern const Array array[14];       // 14 unique configs


    // TODO: combine .ultimax & .bank to a single index (0..7)
    //       (--> make vic_layouts an array of 8 entries...)
    enum VIC_mapping : u8 {
        ram_0, ram_1, ram_2, ram_3,
        chr_r, romh,
    };

    static constexpr u8 vic_layouts[2][4][4] = {
        {   // non-ultimax
            { ram_3, ram_3, ram_3, ram_3 },
            { ram_2, chr_r, ram_2, ram_2 },
            { ram_1, ram_1, ram_1, ram_1 },
            { ram_0, chr_r, ram_0, ram_0 },
        },
        {   // ultimax
            { ram_3, ram_3, ram_3, romh },
            { ram_2, ram_2, ram_2, romh },
            { ram_1, ram_1, ram_1, romh },
            { ram_0, ram_1, ram_0, romh },
        }
    };
}



class Bus {
public:
    Bus(
        State::System& s_,
        const State::System::ROM& rom_,
        CIA& cia1_, CIA& cia2_, TheSID& sid_, VIC& vic_)
      :
        s(s_), rom(rom_), cia1(cia1_), cia2(cia2_), sid(sid_), vic(vic_) {}

    void reset() {
        s.ba_low = false;
        s.dma_low = false;

        s.pla.io_port_pd = s.pla.io_port_state = 0x00;
        w_dd(0x00); // all inputs

        Expansion::reset(s);
    }

    void access(const u16& addr, u8& data, const State::System::Bus::RW rw) {
        do_access(addr, data, rw);
        // TODO: a better (a more efficient) way of maintaining bus state?
        s.bus.addr = addr;
        s.bus.data = data;
        s.bus.rw = rw;
    }

    u8 vic_access(const u16& addr) const { // addr is 14bits
        using m = PLA::VIC_mapping;
        // silence compiler (we do handle all the cases)
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wreturn-type"

        switch (PLA::vic_layouts[s.pla.ultimax][s.pla.vic_bank][addr >> 12]) {
            case m::ram_0: return s.ram[0x0000 | addr];
            case m::ram_1: return s.ram[0x4000 | addr];
            case m::ram_2: return s.ram[0x8000 | addr];
            case m::ram_3: return s.ram[0xc000 | addr];
            case m::chr_r: return rom.charr[0x0fff & addr];
            case m::romh: {
                u8 data = 0x00;
                //romh_r(addr & 0x01fff, data); // TODO
                Expansion::bus_op(s.expansion_type, Expansion::Bus_op::romh_r, addr & 0x1fff, data, s);
                return data;
            }
        }

        #pragma GCC diagnostic push
    }

    void col_ram_r(const u16& addr, u8& data) const { data = s.color_ram[addr]; }

private:
    enum IO_bits : u8 {
        loram_hiram_charen_bits = 0x07,
        cassette_switch_sense = 0x10,
        cassette_motor_control = 0x20,
    };

    void do_access(const u16& addr, u8& data, const State::System::Bus::RW rw) {
        using m = PLA::Mapping;
        namespace E = Expansion;

        switch (auto mapping = PLA::array[s.pla.active].pl[rw][addr >> 12]; mapping) {
            case m::ram0_r:
                data = (addr > 0x0001)
                            ? s.ram[addr]
                            : ((addr == 0x0000) ? r_dd() : r_pd());
                return;
            case m::ram0_w:
                if (addr > 0x0001) s.ram[addr] = data;
                else (addr == 0x0000) ? w_dd(data) : w_pd(data);
                return;
            case m::ram_r:   data = s.ram[addr];               return; // 64 KB
            case m::ram_w:   s.ram[addr] = data;               return;
            case m::bas_r:   data = rom.basic[addr & 0x1fff];  return; // 8 KB
            case m::kern_r:  data = rom.kernal[addr & 0x1fff]; return;
            case m::charr_r: data = rom.charr[addr & 0x0fff];  return; // 4 KB
            case m::roml_r: case m::roml_w: case m::romh_r: case m::romh_w: { // 8 KB
                const auto op = E::Bus_op::roml_r + (mapping - m::roml_r); // translate mapping to op
                E::bus_op(s.expansion_type, E::Bus_op(op), addr & 0x1fff, data, s);
                return;
            }
            case m::io_r:    r_io(addr & 0x0fff, data);        return; // 4 KB
            case m::io_w:    w_io(addr & 0x0fff, data);        return;
            case m::none_r:  // TODO: anything..? (return 'floating' state..)
            case m::none_w:                                    return;
        }
    }

    u8 r_dd() const { return s.pla.io_port_dd; }
    u8 r_pd() const {
        static constexpr u8 pull_up = IO_bits::loram_hiram_charen_bits | IO_bits::cassette_switch_sense;
        const u8 pulled_up = ~s.pla.io_port_dd & pull_up;
        const u8 cmc = ~cassette_motor_control | s.pla.io_port_dd; // dd input -> 0
        return (s.pla.io_port_state | pulled_up) & cmc;
    }

    void w_dd(const u8& dd) { s.pla.io_port_dd = dd; update_io_port_state(); }
    void w_pd(const u8& pd) { s.pla.io_port_pd = pd; update_io_port_state(); }

    void col_ram_w(const u16& addr, const u8& data) { s.color_ram[addr] = data & 0xf; } // masking the write is enough

    void r_io(const u16& addr, u8& data) {
        namespace E = Expansion;
        switch (addr >> 8) {
            case 0x0: case 0x1: case 0x2: case 0x3: vic.r(addr & 0x003f, data);     return;
            case 0x4: case 0x5: case 0x6: case 0x7: sid.r(addr & 0x001f, data);     return;
            case 0x8: case 0x9: case 0xa: case 0xb: col_ram_r(addr & 0x03ff, data); return;
            case 0xc:                               cia1.r(addr & 0x000f, data);    return;
            case 0xd:                               cia2.r(addr & 0x000f, data);    return;
            case 0xe: E::bus_op(s.expansion_type, E::Bus_op::io1_r, addr, data, s); return;
            case 0xf: E::bus_op(s.expansion_type, E::Bus_op::io2_r, addr, data, s); return;
        }
    }

    void w_io(const u16& addr, const u8& data) {
        namespace E = Expansion;
        switch (addr >> 8) {
            case 0x0: case 0x1: case 0x2: case 0x3: vic.w(addr & 0x003f, data);     return;
            case 0x4: case 0x5: case 0x6: case 0x7: sid.w(addr & 0x001f, data);     return;
            case 0x8: case 0x9: case 0xa: case 0xb: col_ram_w(addr & 0x03ff, data); return;
            case 0xc:                               cia1.w(addr & 0x000f, data);    return;
            case 0xd:                               cia2.w(addr & 0x000f, data);    return;
            case 0xe: E::bus_op(s.expansion_type, E::Bus_op::io1_w, addr, const_cast<u8&>(data), s); return;
            case 0xf: E::bus_op(s.expansion_type, E::Bus_op::io2_w, addr, const_cast<u8&>(data), s); return;
        }
    }

    void update_io_port_state() { // output bits set from 'pd', input bits unchanged
        s.pla.io_port_state = (s.pla.io_port_pd & s.pla.io_port_dd)
                                | (s.pla.io_port_state & ~s.pla.io_port_dd);
        System::update_pla(s);
    }

    State::System& s;

    const State::System::ROM& rom;

    CIA& cia1;
    CIA& cia2;
    TheSID& sid;
    VIC& vic;
};


class Int_hub {
private:
    using State = State::System::Int_hub;

    State& s;
public:
    Int_hub(State& s_) : s(s_) {}

    void reset() { s.state = s.old_state = 0x00; s.nmi_act = s.irq_act = false; }

    IO::Int_sig int_sig{s.state};

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


class Menu {
public:
    Menu(std::initializer_list<::Menu::Immediate_action> imm_actions_,
        std::initializer_list<::Menu::Action> actions_,
        std::initializer_list<::Menu::Group> subs_)
      :
        imm_actions(imm_actions_), actions(actions_), subs(subs_),
        root("", imm_actions, actions, subs) {}

    void handle_key(u8 code);
    std::string text() const { return root.text(); }

    bool active = false;

private:
    std::vector<::Menu::Immediate_action> imm_actions;
    std::vector<::Menu::Action> actions;
    std::vector<::Menu::Group> subs;

    ::Menu::Group root;
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
    using Mode = State::System::Mode;

    enum Trap_OPC { // Halting instuctions are used as traps
        //IEC_virtual_routine = 0x02,
        tape_routine = 0x12,
    };
    enum Trap_ID {
        load = 0x01, save = 0x02,
    };

    C64(const State::System::ROM& rom_) : rom(rom_)
    {
        // intercept load/save for tape device
        install_kernal_tape_traps(const_cast<u8*>(rom.kernal), Trap_OPC::tape_routine);

        Expansion::detach(s);
    }

    void run(Mode init_mode = Mode::clocked);

private:
    Files::System_snapshot sys_snap;

    // TODO: refactor this out?
    const State::System::ROM& rom; // TODO: include in sys_snap (but make it optional)?

    State::System& s{sys_snap.sys_state};

    CPU cpu{s.cpu, cpu_trap};

    CIA cia1{s.cia1, cia1_pa_out, cia1_pb_out, int_hub.int_sig, IO::Int_sig::Src::cia1, s.vic.cycle};
    CIA cia2{s.cia2, cia2_pa_out, cia2_pb_out, int_hub.int_sig, IO::Int_sig::Src::cia2, s.vic.cycle};

    TheSID sid{int(FRAME_RATE_MIN), Performance::min_sync_points, s.vic.cycle};

    VIC vic{s.vic, bus, s.ba_low, int_hub.int_sig};

    Bus bus{s, rom, cia1, cia2, sid, vic};

    Int_hub int_hub{s.int_hub};

    Input_matrix input_matrix{s.input_matrix, cia1.port_a.ext_in, cia1.port_b.ext_in, vic};

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
            s.pla.vic_bank = va14_va15;

            c1541.iec.cia2_pa_output(state);
            /*if (c1541.idle) {
                c1541.idle = false;
                run_cfg_change = true;
            }*/
        }
    };
    IO::Port::PD_out cia2_pb_out { [](u8 _) { UNUSED(_); } };

    Host::Input::Handlers host_input_handlers{
        // TODO: just-in-time polling for keyboard/ctrl-ports? (i.e. when CIA1 regs are read)
        // client keyboard & controllers (including lightpen)
        input_matrix.keyboard,
        input_matrix.ctrl_port_1,
        input_matrix.ctrl_port_2,

        // restore
        [this](u8 down) { if (!down) int_hub.int_sig.set(IO::Int_sig::Src::rstr); },
    
        // system keys
        [this](u8 code, u8 down) {
            using kc = Key_code::System;
    
            if (down) {
                switch (code) {
                    case kc::rst_cold:   reset_cold();                 break;
                    case kc::swap_joy:   host_input.swap_joysticks();  break;
                    case kc::tgl_fscr:   vid_out.toggle_fullscr_win(); break;
                    case kc::mode:
                        s.mode = (s.mode == Mode::clocked) ? Mode::stepped : Mode::clocked;
                        break;
                    case kc::step_cycle:
                    case kc::step_instr:
                    case kc::step_line:
                    case kc::step_frame: if (s.mode == Mode::stepped) step_forward(code); break;
                    case kc::menu_tgl:   menu.active = true;           break;
                    case kc::menu_ent:
                    case kc::menu_back:
                    case kc::menu_up:
                    case kc::menu_down:  menu.handle_key(code);        break;
                    case kc::rot_dsk:    c1541.disk_carousel.rotate(); break;
                    case kc::shutdown:   request_shutdown();           break;
                }
            } else {
                if (code == kc::menu_tgl) menu.active = false;
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

    _Stopwatch watch;
    Timer frame_timer;

    std::function<void()> deferred;
    void check_deferred();

    void pre_run();

    void run_cycle();

    void run_clocked();
    void run_stepped();

    void step_forward(u8 key_code);

    void output_frame();

    void reset_warm();
    void reset_cold();

    void save_state_req();

    bool handle_file(Files::File& file);

    void do_load();
    void do_save();

    void request_shutdown() {
        Log::info("Shutdown requested");
        s.mode = Mode::none;
    }

    std::vector<::Menu::Action> exp_menu_actions{
        {"DETACH ?", [&](){ Expansion::detach(s); reset_cold(); }},
        {"ATTACH REU ?", [&]() { Expansion::attach_REU(s); reset_cold(); }},
    };

    std::vector<::Menu::Knob> perf_menu_items{
        {"FPS",  perf.frame_rate,
            [&]() {
                // TODO: fade in sound after change (hide the blips...)?
                deferred = [&]() {
                    vid_out.reconfig();
                    sid.reconfig(perf.frame_rate, perf.audio_pitch_shift);
                    //pre_run();
                };
            }
        },
        // TODO: defer also the following two?
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
            {"EXPANSION / ", exp_menu_actions},
            {"PERFORMANCE / ", perf_menu_items},
        }
    };

    static void install_kernal_tape_traps(u8* kernal, u8 trap_opc);
};

inline
void C64::run_cycle() {
    vic.tick();

    const auto rw = cpu.mrw();
    const auto rdy_low = s.ba_low || s.dma_low;
    if (rdy_low && rw == NMOS6502::MC::RW::r) {
        c1541.tick();
    } else {
        // 'Slip in' the C1541 cycle
        if (rw == NMOS6502::MC::RW::w) c1541.tick();
        bus.access(cpu.mar(), cpu.mdr(), rw);
        cpu.tick();
        if (rw == NMOS6502::MC::RW::r) c1541.tick();
    }

    Expansion::tick(s, bus);

    cia1.tick();
    cia2.tick();

    int_hub.tick(cpu);

    if (s.vic.cycle % C1541::extra_cycle_freq == 0) c1541.tick();
}


} // namespace System


#endif // SYSTEM_H_INCLUDED
