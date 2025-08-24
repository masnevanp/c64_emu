#ifndef BUS_H_INCLUDED
#define BUS_H_INCLUDED

#include "common.h"
#include "state.h"
#include "expansion.h"



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

    // There is a PLA Line for each mode, and for each mode there are separate r/w configs
    struct Line {
        const Mapping pl[2][16]; // [w/r][bank]
    };

    extern const Line line[14];       // 14 unique configs
    extern const u8 Mode_to_line[32]; // map: mode --> line idx (in line[])
}


template<typename CPU, typename CIA, typename SID, typename VIC, typename Color_RAM>  
class Bus {
public:
    Bus(
        State::System& s_, const State::System::ROM& rom_,
        CIA& cia1_, CIA& cia2_, SID& sid_, VIC& vic_,
        Color_RAM& col_ram_, Expansion_ctx::IO& exp_)
      :
        s(s_),
        rom(rom_),
        cia1(cia1_), cia2(cia2_), sid(sid_), vic(vic_),
        col_ram(col_ram_), exp(exp_) {}

    void reset() {
        s.banking.io_port_pd = s.banking.io_port_state = 0x00;
        w_dd(0x00); // all inputs
    }

    void access(const u16& addr, u8& data, const u8 rw) {
        using m = PLA::Mapping;

        switch (PLA::line[s.banking.pla_line].pl[rw][addr >> 12]) {
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
            case m::roml_r:  exp.roml_r(addr & 0x1fff, data);  return; // 8 KB
            case m::roml_w:  exp.roml_w(addr & 0x1fff, data);  return;
            case m::romh_r:  exp.romh_r(addr & 0x1fff, data);  return;
            case m::romh_w:  exp.romh_w(addr & 0x1fff, data);  return;
            case m::io_r:    r_io(addr & 0x0fff, data);        return; // 4 KB
            case m::io_w:    w_io(addr & 0x0fff, data);        return;
            case m::none_r:  // TODO: anything..? (return 'floating' state..)
            case m::none_w:                                    return;
        }
    }

    void set_exrom_game(bool exrom, bool game) {
        s.banking.exrom_game = (exrom << 4) | (game << 3);
        set_pla();

        vic.set_ultimax(exrom && !game);
    }

private:
    enum IO_bits : u8 {
        loram_hiram_charen_bits = 0x07,
        cassette_switch_sense = 0x10,
        cassette_motor_control = 0x20,
    };

    State::System& s;

    u8 r_dd() const { return s.banking.io_port_dd; }
    u8 r_pd() const {
        static constexpr u8 pull_up = IO_bits::loram_hiram_charen_bits | IO_bits::cassette_switch_sense;
        const u8 pulled_up = ~s.banking.io_port_dd & pull_up;
        const u8 cmc = ~cassette_motor_control | s.banking.io_port_dd; // dd input -> 0
        return (s.banking.io_port_state | pulled_up) & cmc;
    }

    void w_dd(const u8& dd) { s.banking.io_port_dd = dd; update_state(); }
    void w_pd(const u8& pd) { s.banking.io_port_pd = pd; update_state(); }

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

    void update_state() { // output bits set from 'pd', input bits unchanged
        auto& b{s.banking};
        b.io_port_state = (b.io_port_pd & b.io_port_dd) | (b.io_port_state & ~b.io_port_dd);
        set_pla();
    }

    void set_pla() {
        auto& b{s.banking};
        const u8 lhc = (b.io_port_state | ~b.io_port_dd) & loram_hiram_charen_bits; // inputs -> pulled up
        const u8 mode = b.exrom_game | lhc;
        b.pla_line = PLA::Mode_to_line[mode];
    }

    const State::System::ROM& rom;

    CIA& cia1;
    CIA& cia2;
    SID& sid;
    VIC& vic;
    Color_RAM& col_ram;

    Expansion_ctx::IO& exp;
};


#endif // BUS_H_INCLUDED