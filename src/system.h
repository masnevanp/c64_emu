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

class VIC_II_out;

using CIA = CIA::Core;
using SID = SID::Wrapper;
using VIC = VIC_II::Core<VIC_II_out>;


static const u8 IO_PORT_INIT_DD = 0xef; // Source: Mapping the Commodore C64, by Sheldon Leemon


bool load_prg(const std::string filename, u8* ram);


struct ROM {
    const u8* basic;
    const u8* kernal;
    const u8* charr;
};


class IO_space {
public:
    IO_space(
        CIA& cia1_, CIA& cia2_,
        SID& sid_,
        VIC& vic_,
        VIC_II::Color_RAM& col_ram_)
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
    SID& sid;
    VIC& vic;
    VIC_II::Color_RAM& col_ram;
};


class Banker {
public:
    Banker(u8* ram_, const ROM& rom, IO_space& io_)
        : io_port(io_port_out, IO_PORT_INIT_DD),
          ram(ram_),
          bas0(rom.basic),   bas1(rom.basic + 0x1000),
          kern0(rom.kernal), kern1(rom.kernal + 0x1000),
          charr(rom.charr),
          io(io_)
    { reset(); }

    void reset() {
        io_port.reset();
        set_mode(0x1f, 0x1f);
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
                if (addr <= 0x1) {
                    data = (addr == 0x0) ? io_port.r_dd() : io_port.r_pd();
                } else { data = ram[addr]; }
                return;
            case ram0_w:
                if (addr <= 0x1) {
                    if (addr == 0x0) io_port.w_dd(data);
                    else io_port.w_pd(data);
                } else {
                    ram[addr] = data;
                }
                return;
            case ram_r:   data = ram[addr];             return;
            case ram_w:   ram[addr] = data;             return;
            case bas0_r:  data = bas0[addr & 0x0fff];   return;
            case bas1_r:  data = bas1[addr & 0x0fff];   return;
            case kern0_r: data = kern0[addr & 0x0fff];  return;
            case kern1_r: data = kern1[addr & 0x0fff];  return;
            case charr_r: data = charr[addr & 0x0fff];  return;
            case roml0_r: data = roml0[addr & 0x0fff];  return;
            case roml0_w:                               return; // TODO: what?
            case roml1_r: data = roml1[addr & 0x0fff];  return;
            case roml1_w:                               return; // TODO: what?
            case romh0_r: data = romh0[addr & 0x0fff];  return;
            case romh0_w:                               return; // TODO: what?
            case romh1_r: data = romh1[addr & 0x0fff];  return;
            case romh1_w:                               return; // TODO: what?
            case io_r:    io.r(addr & 0x0fff, data);    return;
            case io_w:    io.w(addr & 0x0fff, data);    return;
            case none_r: case none_w: return; // TODO: anything?
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
    // TODO: a separate IO port component? (or at least make these private?)
    void set_loram_hiram_charen(u8 bits, u8 bit_vals) { set_mode(bits & 0x07, bit_vals); }
    void set_game_exrom(u8 bits, u8 bit_vals)         { set_mode(bits & 0x18, bit_vals); }

    // TODO: should these include the game|exrom flags? or set them automagically?
    void attach_roml(const u8* roml) { roml0 = roml; roml1 = roml + 0x1000; }
    void attach_romh(const u8* romh) { romh0 = romh; romh1 = romh + 0x1000; }
    // TODO: detatch rom...

private:
    void set_mode(u8 bits, u8 bit_vals) {
        u8 new_mode = ((mode & ~bits) | (bit_vals & bits));
        if (new_mode != mode) {
            mode = new_mode;
            // std::cout << "\n[System::Banker]: mode " << (int)mode;
            pla = &PLA[Mode_to_PLA_idx[mode]];
        }
    }

    IO::Port::PD_out io_port_out {
        [this](u8 bits, u8 bit_vals) {
            set_loram_hiram_charen(bits, bit_vals);
        }
    };

    // There is a PLA_Line for each mode, and for each mode there are separate r/w configs
    struct PLA_Line {
        const u8 pl[2][16]; // [w/r][bank]
    };

    static const PLA_Line PLA[14];       // 14 unique configs
    static const u8 Mode_to_PLA_idx[32]; // map: mode --> mode_idx (in PLA[])

    // TODO: exrom&game bits handling
    u8 mode = 0x00;      // current mode
    const PLA_Line* pla; // the active PLA line (set based on mode)

    IO::Port io_port;

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

    IO_space& io;
};


class VIC_II_out {
public:
    VIC_II_out(Host::Input& host_input_, SID& sid_) :
        vid_out(VIC_II::VIEW_WIDTH, VIC_II::VIEW_HEIGHT),
        host_input(host_input_), sid(sid_) {}

    void reset() {
        frame_moment = 0;
        clock.reset();
    }

    void put(u8* line) { vid_out.put(line); }

    void line_done(u16 line) {
        if (line % (VIC_II::RASTER_LINE_COUNT / 8) == 0) host_input.poll(); // freq: 8 per frame ==> ~2.5ms
    }

    void frame_done() {
        frame_moment += VIC_II::FRAME_MS;
        auto wait_target = std::round(frame_moment);
        auto wait_ms = clock.wait_until(wait_target);

        if (wait_ms >= 0) {
            if (skip_frames > 0) {
                skip_frames = 0;
                sid.flush();
            }
            vid_out.frame_done();
        } else {
            ++skip_frames;
            if ((skip_frames % 3) == 0) vid_out.frame_done();
            else vid_out.frame_skip();
        }

        if (wait_target >= 6318000) reset();
    }

private:
    Host::Video_out vid_out;
    Host::Input& host_input;

    SID& sid;

    Clock clock;
    double frame_moment = 0;
    int skip_frames = 0;

};


class C64 {
public:
    C64(const ROM& rom) :
        cpu(on_cpu_halt_sig),
        cia1(1, sync_master, cia1_port_a_out, cia1_port_b_out, int_hub.irq),
        cia2(2, sync_master, cia2_port_a_out, cia2_port_b_out, int_hub.nmi),
        vic(sync_master, ram, col_ram, rom.charr, int_hub.irq, ba_low, vic_out),
        vic_out(host_input, sid),
        int_hub(cpu),
        kb_matrix(cia1.get_port_a_in(), cia1.get_port_b_in()),
        joy1(cia1.get_port_b_in()),
        joy2(cia1.get_port_a_in()),
        io_space(cia1, cia2, sid, vic, col_ram),
        sys_banker(ram, rom, io_space),
        host_input(host_input_handlers)
    {
        init_ram();
    }

    void reset_warm() {
        cia1.reset_warm(); // need to reset for correct irq handling
        cia2.reset_warm();
        vic.reset_warm();
        cpu.reset_warm();
        int_hub.reset();
        nmi_set = false;
    }

    void reset_cold() {
        cia1.reset_cold();
        cia2.reset_cold();
        sid.reset();
        vic.reset_cold();
        vic_out.reset();
        kb_matrix.reset();
        init_ram();
        col_ram.reset();
        sys_banker.reset();
        cpu.reset_cold();
        int_hub.reset();

        nmi_set = false;
    }

    void run() {
        vic_out.reset();
        for (;;) {
            sync_master.tick();
            if (!ba_low || cpu.mrw() == NMOS6502::RW::w) {
                sys_banker.access(cpu.mar(), cpu.mdr(), cpu.mrw());
                cpu.tick();
            }
            vic.tick();
            cia1.tick();
            cia2.tick();
            sid.tick();
            int_hub.tick();
        }
    }

    u8 ram[0x10000];
    NMOS6502::Core cpu; // 6510 IO port (addr 0&1) is handled externally (in banking)

    CIA cia1;
    CIA cia2;

    SID sid;

    VIC_II::Color_RAM col_ram;
    VIC vic;

private:
    VIC_II_out vic_out;

    IO::Sync::Master sync_master;

    IO::Int_hub int_hub;

    IO::Keyboard_matrix kb_matrix;
    IO::Joystick joy1;
    IO::Joystick joy2;

    IO_space io_space;
    Banker sys_banker;

    bool ba_low = false;

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
        }
    };
    IO::Port::PD_out cia2_port_a_out {
        [this](u8 bits, u8 bit_vals) {
            vic.banker.set_bank(bits, bit_vals);
        }
    };
    IO::Port::PD_out cia2_port_b_out {
        [this](u8 bits, u8 bit_vals) { UNUSED(bits); UNUSED(bit_vals); }
    };


    /* -------------------- Host input -------------------- */
    bool nmi_set;

    Host::Input::Handlers host_input_handlers {
        // client_key_handler
        kb_matrix.handler,

        // restore-key handler
        [this](const Key_event& k_ev) {
            if (k_ev.down != nmi_set) {
                nmi_set = k_ev.down;
                if (nmi_set) int_hub.nmi.set();
                else int_hub.nmi.clr();
            }
        },

        // host_key_handler
        [this](const Key_event& k_ev) {
            if (!k_ev.down) return;
            switch (k_ev.code) {
                case Key_event::KC::s_joy:
                    host_input.swap_joysticks();
                    break;
                case Key_event::KC::rst_w:
                    reset_warm();
                    break;
                case Key_event::KC::rst_c:
                    reset_cold();
                    break;
                case Key_event::KC::load:
                    load_prg("data/prg/bin.prg", ram);
                    break;
                case Key_event::KC::quit:
                    exit(0);
                    break;
            }
        },

        // joy_handlers
        joy1.handler, joy2.handler,
    };


    void init_ram() { // TODO: parameterize pattern (+ add 'randomness'?)
        for (int addr = 0; addr < 0x10000; ++addr)
            ram[addr] = (addr & 0x80) ? 0xff : 0x00;
    }


    Sig on_cpu_halt_sig {
        [this]() {
            std::cout << "\n****** CPU halted! ******\n";
            Dbg::print_status(cpu, ram);
        }
    };
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
        if (diff < 0.00001) {
            std::cout << (int)i << ": " << (double)diff << "\n";
        }
    }

    return 0;
}
*/

#endif // SYSTEM_H_INCLUDED
