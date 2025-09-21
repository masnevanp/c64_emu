#ifndef NMOS6502_CORE_H_INCLUDED
#define NMOS6502_CORE_H_INCLUDED

#include "nmos6502.h"
#include "nmos6502_mc.h"


namespace NMOS6502 {

// TODO: SO (set overflow) pin, used by 1541 (is there a delay before it is detected?)

struct Core {
public:
    struct State {
        Reg8 zp;
        Reg8 zph{0x00};
        Reg16 pc;
        Reg8 d;
        Reg8 ir;
        Reg8 a1l;
        Reg8 a1h;
        Reg16 a2;
        Reg16 sp{0x1ff};
        Reg8 p;
        Reg8 a;
        Reg8 x;
        Reg8 y;
        Reg16 a3;
        Reg16 a4;

        u8 mcc; // micro-code counter

        u8 nmi_act;
        u8 nmi_timer;
        u8 irq_act;
        u8 irq_timer;
        u8 brk_srcs;

        Reg16& r16(Ri16 ri) const { return ((Reg16*)(&zp))[ri]; }
        Reg8& r8(Ri8 ri) const { return ((Reg8*)(&zp))[ri]; }

        void set(Flag f, bool set = true) { p = set ? p | f : p & ~f; }
        void clr(Flag f) { p &= ~f; }
        bool is_set(Flag f) const { return p & f; }
        bool is_clr(Flag f) const { return !is_set(f); }

        void set_sp(u8 v) { sp = 0x100 | v; }
        void dec_sp() { set_sp((sp - 1) & 0xff); }
        void inc_sp() { set_sp((sp + 1) & 0xff); }
    };

    State& s;

    Core(State& s_, Sig& sig_halt_);

    void reset();

    const MC::MOP& mop() const { return MC::code[s.mcc]; }

    /* Address space access params (addr, data, r/w).
       The read/write is expected to happen between ticks, e.g.:
           for (;;) {
               do_access(core.mar(), core.mdr(), core.mrw());
               core.tick();
           }
    */
    using RW = NMOS6502::MC::RW;

    u16 mar() const { return s.r16(mop().ar); } // memory address reg
    u8& mdr() const { return s.r8(mop().dr); }  // memory data reg
    RW  mrw() const { return mop().rw; }        // memory read/write

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

        s.pc += mop().pc_inc;

        const auto mopc = mop().mopc;
        ++s.mcc;
        exec(mopc);
    }

    bool halted() const { return mop().mopc == MC::hlt; }
    void resume() { if (halted()) ++s.mcc; }

private:
    void exec(const u8 mop);

    Sig& sig_halt;
};


}


#endif // NMOS6502_CORE_H_INCLUDED
