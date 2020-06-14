#ifndef SYSTEM_H_INCLUDED
#define SYSTEM_H_INCLUDED


#include <cmath>
#include "common.h"
#include "utils.h"
#include "dbg.h"
#include "nmos6502/nmos6502_core.h"
#include "vic_ii.h"
#include "sid.h"
#include "cia.h"
#include "io.h"
#include "host.h"


namespace System {

class VIC_out;

using CPU = NMOS6502::Core; // 6510 IO port (addr 0&1) is implemented externally (in Banker)
using CIA = CIA::Core;
using TheSID = reSID_Wrapper; // 'The' due to nameclash
using VIC = VIC_II::Core<VIC_out>;
using Color_RAM = VIC_II::Color_RAM;


// sync.freq. in raster lines (must divide into total line count 312)
static const int SYNC_FREQ = 52;  // ==> ~3.3 ms


struct ROM {
    const u8* basic;
    const u8* kernal;
    const u8* charr;
};


class IO_space {
public:
    IO_space(
        CIA& cia1_, CIA& cia2_,
        TheSID& sid_,
        VIC& vic_,
        Color_RAM& col_ram_)
      :
        cia1(cia1_), cia2(cia2_),
        sid(sid_),
        vic(vic_),
        col_ram(col_ram_)
    { }

    void r(const u16& addr, u8& data) {
        switch (addr >> 8) {
            case 0x0: case 0x1: case 0x2: case 0x3: vic.r(addr & 0x003f, data);     return;
            case 0x4: case 0x5: case 0x6: case 0x7: sid.r(addr & 0x001f, data);     return;
            case 0x8: case 0x9: case 0xa: case 0xb: col_ram.r(addr & 0x03ff, data); return;
            case 0xc:                               cia1.r(addr & 0x000f, data);
                //if (data == 0x9) std::cout << "\nr: " << ((int)addr & 0x000f) << " " << " " << (int)data;
                return;
            case 0xd:                               cia2.r(addr & 0x000f, data);    return;
            case 0xe: case 0xf:                     /*TODO: IO areas 1&2*/          return;
        }
    }

    void w(const u16& addr, const u8& data) {
        switch (addr >> 8) {
            case 0x0: case 0x1: case 0x2: case 0x3: vic.w(addr & 0x003f, data);     return;
            case 0x4: case 0x5: case 0x6: case 0x7: sid.w(addr & 0x001f, data);     return;
            case 0x8: case 0x9: case 0xa: case 0xb: col_ram.w(addr & 0x03ff, data); return;
            case 0xc:                               cia1.w(addr & 0x000f, data);    return;
            case 0xd:                               cia2.w(addr & 0x000f, data);    return;
            case 0xe: case 0xf:                     // TODO: IO areas 1&2
                return;
        }
    }

private:
    CIA& cia1;
    CIA& cia2;
    TheSID& sid;
    VIC& vic;
    Color_RAM& col_ram;

};


class Banker {
public:
    Banker(u8* ram_, const ROM& rom, IO_space& io_space_)
        : ram(ram_),
          bas0(rom.basic),   bas1(rom.basic + 0x1000),
          kern0(rom.kernal), kern1(rom.kernal + 0x1000),
          charr(rom.charr),
          io_space(io_space_) { }

    void reset() {
        io_port_state = 0x00;
        w_dd(0x00); // all inputs
    }

    // NOTE: Ultimax config --> writes also directed to ROM
    enum Mapping : u8 {
        ram0_r,  ram0_w, // Special mapping for RAM bank 0 due to IO port handling (addr 0&1)
        ram_r,   ram_w,
        bas0_r,  bas1_r,
        kern0_r, kern1_r,
        charr_r,
        roml0_r, roml0_w,
        roml1_r, roml1_w,
        romh0_r, romh0_w,
        romh1_r, romh1_w,
        io_r,    io_w,
        none_r,  none_w,
    };


    void access(const u16& addr, u8& data, u8 rw) {
        // if (addr <= 1) std::cout << "a: " << (int)addr << " d: " << (int)data << " rw: " << (int)rw << "\n";
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
            case ram_r:   data = ram[addr];                return;
            case ram_w:   ram[addr] = data;                return;
            case bas0_r:  data = bas0[addr & 0x0fff];      return;
            case bas1_r:  data = bas1[addr & 0x0fff];      return;
            case kern0_r: data = kern0[addr & 0x0fff];     return;
            case kern1_r: data = kern1[addr & 0x0fff];     return;
            case charr_r: data = charr[addr & 0x0fff];     return;
            case roml0_r: data = roml0[addr & 0x0fff];     return;
            case roml0_w:                                  return; // TODO: what?
            case roml1_r: data = roml1[addr & 0x0fff];     return;
            case roml1_w:                                  return; // TODO: what?
            case romh0_r: data = romh0[addr & 0x0fff];     return;
            case romh0_w:                                  return; // TODO: what?
            case romh1_r: data = romh1[addr & 0x0fff];     return;
            case romh1_w:                                  return; // TODO: what?
            case io_r:    io_space.r(addr & 0x0fff, data); return;
            case io_w:    io_space.w(addr & 0x0fff, data); return;
            case none_r: case none_w:                      return; // TODO: anything?
        }
    }

    /*
        The C64 PLA Dissected:
            "The lines #LORAM (I1), #HIRAM (I2) and #CHAREN (I3) are connected to the pro-
            cessor port of the CPU, better known as bits 0..2 of $00/$01 in the 6510/8500.  After a
            reset these lines are all set to input mode by the CPU, which means that they are not
            driven.  To make sure the have a sane value when the machine is started, they are pulled
            up by R43, R44 and R45.  With all these values being 1, the C64 can start with KERNAL,
            I/O and BASIC banked in."
            ...
            "The two lines #EXROM and #GAME can be pulled down by cartridges to change the
            memory map of the C64, e.g., to map external ROM into the address space.  When they
            are not pulled down from the cartridge port, resistors in RP4 pull them up."
            ..."
    */

    // TODO: should these include the game|exrom flags? or set them automagically?
    void attach_roml(const u8* roml) { roml0 = roml; roml1 = roml + 0x1000; }
    void attach_romh(const u8* romh) { romh0 = romh; romh1 = romh + 0x1000; }
    // TODO: detatch rom...

private:
    /* TODO:
       - a separate IO port component...?
       - cassette (datasette) emulation?
       - exrom|game bits (harcoded for now)
            - update also to VIC banker
    */
    enum IO_bits : u8 {
        loram_hiram_charen_bits = 0x07,
        cassette_switch_sense = 0x10,
        cassette_motor_control = 0x20,
    };

    u8 io_port_dd;
    u8 io_port_pd;
    u8 io_port_state;

    u8 r_dd() const { return io_port_dd; }
    u8 r_pd() const {
        static const u8 pull_up = IO_bits::loram_hiram_charen_bits | IO_bits::cassette_switch_sense;
        u8 pulled_up = ~io_port_dd & pull_up;
        u8 cmc = ~cassette_motor_control | io_port_dd; // dd input -> 0
        return (io_port_state | pulled_up) & cmc;
    }

    void w_dd(const u8& dd) { io_port_dd = dd; update_state(); }
    void w_pd(const u8& pd) { io_port_pd = pd; update_state(); }

    void update_state() { // output bits set from 'pd', input bits unchanged
        io_port_state = (io_port_pd & io_port_dd) | (io_port_state & ~io_port_dd);
        set_pla();
    }

    void set_pla() {
        static const u8 exrom_game = 0x18; // harcoded
        u8 lhc = (io_port_state | ~io_port_dd) & loram_hiram_charen_bits; // inputs -> pulled up
        u8 mode = exrom_game | lhc;
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

    const u8* bas0;
    const u8* bas1;
    const u8* kern0;
    const u8* kern1;
    const u8* charr;

    const u8* roml0;
    const u8* roml1;
    const u8* romh0;
    const u8* romh1;

    IO_space& io_space;

};


class VIC_out {
public:
    VIC_out(
        IO::Int_hub& int_hub_,
        Host::Video_out& vid_out_, Host::Input& host_input_,
        TheSID& sid_
    ) :
        int_hub(int_hub_),
        vid_out(vid_out_), host_input(host_input_),
        sid(sid_) { }

    void set_irq() { int_hub.set(IO::Int_hub::Src::vic); }
    void clr_irq() { int_hub.clr(IO::Int_hub::Src::vic); }

    void init_sync() { // call if system has been 'paused'
        vid_out.put_frame();
        sid.flush();
        // sid.output(true); ??
        clock.reset();
    }

    void sync_line(u16 line) {
        if (line % SYNC_FREQ != 0) return;

        if (line == 0) { // frame done?
            host_input.poll();

            frame_moment += VIC_II::FRAME_MS;
            auto sync_moment = std::round(frame_moment);
            bool reset = sync_moment == 6318000; // since '316687 * FRAME_MS = 6318000'
            frame_moment = reset ? 0 : frame_moment;

            int diff_ms;
            if (vid_out.v_synced()) {
                if ((frame_skip & 0x3) == 0x0) vid_out.put_frame();
                diff_ms = clock.diff(sync_moment, reset);
            } else {
                diff_ms = clock.sync(sync_moment, reset);
                if ((frame_skip & 0x3) == 0x0) vid_out.put_frame();
            }

            if (diff_ms <= -VIC_II::FRAME_MS) {
                ++frame_skip;
            } else if (frame_skip) {
                frame_skip = 0;
                sid.flush();
            }

            sid.output(true);
        } else {
            if (!frame_skip) {
                host_input.poll();
                auto frame_progress = double(line) / double(VIC_II::RASTER_LINE_COUNT);
                auto sync_moment = frame_moment + (frame_progress * VIC_II::FRAME_MS);
                clock.sync(std::round(sync_moment));
            }
            sid.output();
        }
    }


private:
    IO::Int_hub& int_hub;

    Host::Video_out& vid_out;
    Host::Input& host_input;

    TheSID& sid;

    Clock clock;
    double frame_moment = 0;
    int frame_skip = 0;

};


struct State {
    u8 ram[0x10000];
    u8 color_ram[Color_RAM::size] = {};

    VIC::State vic;
};


class C64 {
public:
    C64(const ROM& rom) :
        cia1(sync_master, cia1_port_a_out, cia1_port_b_out, int_hub, IO::Int_hub::Src::cia1),
        cia2(sync_master, cia2_port_a_out, cia2_port_b_out, int_hub, IO::Int_hub::Src::cia2),
        sid(frame_cycle),
        col_ram(s.color_ram),
        vic(s.vic, s.ram, col_ram, rom.charr, rdy_low, vic_out),
        vid_out(VIC_II::VIEW_WIDTH, VIC_II::VIEW_HEIGHT, s.vic.frame),
        vic_out(int_hub, vid_out, host_input, sid),
        int_hub(cpu),
        kb_matrix(cia1.port_a.ext_in, cia1.port_b.ext_in),
        io_space(cia1, cia2, sid, vic, col_ram),
        sys_banker(s.ram, rom, io_space),
        host_input(host_input_handlers) { }

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
        kb_matrix.reset();
        sys_banker.reset();
        cpu.reset_cold();
        int_hub.reset();

        rdy_low = false;
    }

    void run() {
        reset_cold();
        vic_out.init_sync();

        for (frame_cycle = 1;;++frame_cycle) {
            sync_master.tick();
            vic.tick(frame_cycle);
            if (!rdy_low || cpu.mrw() == NMOS6502::RW::w) {
                sys_banker.access(cpu.mar(), cpu.mdr(), cpu.mrw());
                cpu.tick();
            }
            cia1.tick();
            cia2.tick();
            int_hub.tick();
        }
    }

    State s;

    CPU cpu;

    CIA cia1;
    CIA cia2;

    TheSID sid;

    Color_RAM col_ram;
    VIC vic;

private:
    static const u8 LP_BIT = 0x10;

    Host::Video_out vid_out;
    VIC_out vic_out;

    IO::Sync::Master sync_master;

    IO::Int_hub int_hub;

    IO::Keyboard_matrix kb_matrix;

    IO_space io_space;
    Banker sys_banker;

    u16 frame_cycle;

    u16 rdy_low;

    Host::Input host_input;

    /* -------------------- CIA port outputs -------------------- */
    // NOTE: there may be coming many updates per cycle (receiver may need to keep track)
    IO::Port::PD_out cia1_port_a_out {
        [this](u8 bits, u8 bit_vals) {
            kb_matrix.port_a_out(bits, bit_vals);
        }
    };
    IO::Port::PD_out cia1_port_b_out {
        [this](u8 bits, u8 bit_vals) {
            kb_matrix.port_b_out(bits, bit_vals);
            u8 lp_low = (bits & LP_BIT) & ~bit_vals;
            vic.set_lp(VIC::LP_src::cia, lp_low);
        }
    };
    IO::Port::PD_out cia2_port_a_out {
        [this](u8 bits, u8 bit_vals) {
            vic.set_va14_va15(((bit_vals & bits) | ~bits) & 0x3);
        }
    };
    IO::Port::PD_out cia2_port_b_out {
        [this](u8 bits, u8 bit_vals) {
            UNUSED(this); UNUSED(bits); UNUSED(bit_vals);
        }
    };

    /* -------------------- Host input -------------------- */
    Host::Input::Handlers host_input_handlers {
        // keyboard
        kb_matrix.handler,

        // system
        [this](u8 code, u8 down) {
            using kc = Key_code::System;

            if (!down) {
                if (code == kc::rstre) int_hub.set(IO::Int_hub::Src::rstr);
                return;
            }

            switch (code) {
                case kc::rst_w: reset_warm();                       break;
                case kc::rst_c: reset_cold();                       break;
                case kc::swp_j: host_input.swap_joysticks();        break;
                case kc::m_win: vid_out.toggle_windowed();          break;
                case kc::m_fsc: vid_out.toggle_fullscreen();        break;
                case kc::scl_u: vid_out.adjust_scale(+5);           break;
                case kc::scl_d: vid_out.adjust_scale(-5);           break;
                case kc::quit:  exit(0);                            break;
            }
        },

        // joy1
        [this](u8 code, u8 down) {
            const u8 bit_pos = 0x1 << code;
            const u8 bit_val = down ? 0x0 : bit_pos;
            cia1.port_b.ext_in(bit_pos, bit_val);
            if (bit_pos == LP_BIT) vic.set_lp(VIC::LP_src::ctrl_port, down);
        },

        // joy2
        [this](u8 code, u8 down) {
            const u8 bit_pos = 0x1 << code;
            const u8 bit_val = down ? 0x0 : bit_pos;
            cia1.port_a.ext_in(bit_pos, bit_val);
        }
    };

    void init_ram();

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
