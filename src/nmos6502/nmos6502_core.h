#ifndef NMOS6502_CORE_H_INCLUDED
#define NMOS6502_CORE_H_INCLUDED

#include "nmos6502.h"
#include "nmos6502_mc.h"


namespace NMOS6502 {

// TODO: SO (set overflow) pin, used by 1541 (is there a delay before it is detected?)

struct Core {
public:
    struct State {
        Reg16 zp16;
        Reg16 pc;
        Reg8 d; Reg8 ir;
        Reg16 a1;
        Reg16 a2;
        Reg16 sp16;
        Reg8 p; Reg8 a;
        Reg8 x; Reg8 y;
        Reg16 a3;
        Reg16 a4;

        u8 nmi_act;
        u8 nmi_timer;
        u8 irq_act;
        u8 irq_timer;
        u8 brk_srcs;

        const Reg16& r16(Ri16 ri) const { return (&zp16)[ri]; }
        Reg8& r8(Ri8 ri) const { return ((Reg8*)(&zp16))[ri]; }

        Reg8& zp{r8(Ri8::zp)};
        Reg8& pcl{r8(Ri8::pcl)}; Reg8& pch{r8(Ri8::pch)};
        Reg8& a1l{r8(Ri8::a1l)}; Reg8& a1h{r8(Ri8::a1h)};
        Reg8& sp{r8(Ri8::sp)};

        void set(Flag f, bool set = true) { p = set ? p | f : p & ~f; }
        void clr(Flag f) { p &= ~f; }
        bool is_set(Flag f) const { return p & f; }
        bool is_clr(Flag f) const { return !is_set(f); }
    };

    State& s;

    Core(State& s_, Sig& sig_halt_);

    const MC::MOP* mcp; // micro-code pointer ('mc pc')

    void reset();

    bool halted() const { return mcp->mopc == MC::hlt; }
    void resume() { if (halted()) ++mcp; }

    /* Address space access params (addr, data, r/w).
       The read/write is expected to happen between ticks, e.g.:
           for (;;) {
               do_access(core.mar(), core.mdr(), core.mrw());
               core.tick();
           }
    */
    u16 mar() const { return s.r16(mcp->ar); }
    u8& mdr() const { return s.r8(mcp->dr); }
    u8  mrw() const { return mcp->rw; }

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

        s.pc += mcp->pc_inc;

        const auto mopc = (mcp++)->mopc;
        if(mopc != NMOS6502::MC::nmop) exec(mopc);
    }

private:
    void exec(const u8 mop);

    Sig& sig_halt;
};


}


#endif // NMOS6502_CORE_H_INCLUDED
