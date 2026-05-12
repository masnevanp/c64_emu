#ifndef NMOS6502_CORE_H_INCLUDED
#define NMOS6502_CORE_H_INCLUDED

#include "nmos6502.h"
#include "nmos6502_mc.h"


namespace NMOS6502 {

// TODO: SO (set overflow) pin, used by 1541 (is there a delay before it is detected?)

struct Core {
public:

    struct State {
        struct Bus {
            enum RW : u8 { w = 0, r = 1 };

            /* Bus access params (addr, data, r/w).
                The read/write is expected to happen between ticks, e.g.:
                for (;;) {
                    do_access(core.s.bus_a, core.s.bus_d, core.s.bus_rw);
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

        u16 mcc; // micro-code counter

        u16 pc;
        u16 sp;
        u16 ad; // TODO: is the high-byte really needed?
    
        Bus bus;
    
        u8 p;
        u8 a;
        u8 x;
        u8 y;

        u8 nmi_act;
        u8 nmi_timer;
        u8 irq_act;
        u8 irq_timer;
        u8 brk_srcs;

        u16 opc() const { return mcc >> 3; }

        void set(Flag f, bool set = true) { p = set ? p | f : p & ~f; }
        void clr(Flag f) { p &= ~f; }
        bool is_set(Flag f) const { return p & f; }
        bool is_clr(Flag f) const { return !is_set(f); }
    };

    State& s;

    Core(State& s_, Sig& sig_halt_);

    void reset();

    void set_nmi(bool act);
    void set_irq(bool act);

    void tick() {
        if (s.irq_timer || s.nmi_timer) {
            if (s.nmi_timer == 0x01) s.nmi_timer = 0x02;
            else if (s.nmi_timer == 0x02) {
                s.nmi_act = true;
                s.nmi_timer = 0x03;
            }

            if (s.irq_timer & 0b10) s.irq_act = true;
            s.irq_timer <<= 1;
        }

        exec_cycle();
    }

    bool halted() const { /*return mop().mopc == MC::hlt;*/ return false; } // ### TODO ###
    void resume() { /*if (halted()) ++s.mcc;*/ } // ### TODO ###

private:
    void exec_cycle();

    void schedule(u16 opc) { s.mcc = opc << 3; } // bits 0-2 encode the cycle (0..7)

    Sig& sig_halt;
};


}


#endif // NMOS6502_CORE_H_INCLUDED
