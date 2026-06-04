#ifndef MOS6502_CORE_H_INCLUDED
#define MOS6502_CORE_H_INCLUDED

#include <functional>
#include <cstdint>

/*
Invaluable resources (thanks!):
    http://visual6502.org/JSSim/
    http://retro.hansotten.nl/uploads/6502docs/hwman.htm
    https://www.nesdev.org/wiki/Visual6502wiki
    https://www.nesdev.org/wiki/Visual6502wiki/6502_all_256_Opcodes
    https://www.masswerk.at/6502/6502_instruction_set.html

    THE C64 Emulator Test Suite 2.15 by Wolfgang Lorenz
*/

namespace MOS6502 {

using u8 = uint8_t;
using i8 = int8_t;
using u16 = uint16_t;

using Sig_halt = std::function<void (u8, u8)>;

enum Vec : u16 { nmi = 0xfffa, rst = 0xfffc, irq = 0xfffe, };

enum Flag : u8 { // nvubdizc, u = unused (bit 5)
    N = 0b10000000, V = 0b01000000, u = 0b00100000, B = 0b00010000,
    D = 0b00001000, I = 0b00000100, Z = 0b00000010, C = 0b00000001,
    all = 0b11111111,
};

enum OPC : u16 {
    brk = 0x00, rts = 0x60,
    bra = 0x100,
    dispatch_post_cli, dispatch_post_sei, dispatch, dispatch_post_brk,
    halt, reset
};


struct Core {
public:    

    // TODO: SO (set overflow) pin, used by 1541 (is there a delay before it is detected?)

    struct State {
        struct Bus {
            enum RW : u8 { w = 0, r = 1 };

            /*
                Bus access params (addr, data, r/w). The read/write is expected
                to happen before each tick(), e.g.:
                    for (;;) {
                        handle_bus_access(core.s.bus);
                        core.tick();
                    }
            */
            u16 a;
            u8 d;
            RW rw;

            void operator()(const u16 a_, const u8 d_, const RW rw_) { a = a_; d = d_; rw = rw_; }
            void operator()(const u16 a_, const u8 d_) { a = a_; d = d_; }
            void operator()(const u16 a_, const RW rw_) { a = a_; rw = rw_; }
            void operator()(const u16 a_) { a = a_; }
            void operator()(const RW rw_) { rw = rw_; }
        };

        Bus bus;

        u16 mcc; // micro-code counter

        u16 pc;
        u16 aux;
        u16 sp;

        u8 p;
        u8 a;
        u8 x;
        u8 y;

        u8 nmi_act;
        u8 irq_act;
        u8 nmi_timer;
        u8 irq_timer;

        u16 brk_vec;

        OPC opc() const { return OPC(mcc >> 3); }

        void set(Flag f, bool set = true) { p = set ? p | f : p & ~f; }
        void clr(Flag f) { p &= ~f; }
    };

    State& s;

    Core(State& s_, Sig_halt& sig_halt_);

    void reset();

    void set_nmi(bool act);
    void set_irq(bool act);

    void tick();

    bool at_fetch() const { return s.opc() >= OPC::dispatch_post_cli && s.opc() <= OPC::dispatch_post_brk; }
    bool halted() const   { return s.opc() == OPC::halt; }
    void resume();

private:
    Sig_halt& sig_halt;

};

} // namespace MOS6502


#endif // MOS6502_CORE_H_INCLUDED
