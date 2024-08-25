#ifndef IO_H_INCLUDED
#define IO_H_INCLUDED


#include "nmos6502/nmos6502_core.h"


namespace IO {


class Int_hub {
public:
    enum Src : u8 {
        cia1 = 0x01, vic  = 0x02, exp_i = 0x04, // IRQ sources
        cia2 = 0x10, rstr = 0x20, exp_n = 0x40, // NMI sources
        irq  = 0x0f, nmi  = 0xf0, // source mask
    };

    Int_hub(NMOS6502::Core& cpu_) : cpu(cpu_) { }

    void reset() { state = old_state = 0x00; nmi_act = irq_act = false; }

    void set(Src s) { state |= s;  }
    void clr(Src s) { state &= ~s; }

    void tick() {
        if (state != old_state) {
            old_state = state;

            bool act = (state & Src::nmi);
            if (nmi_act != act) {
                nmi_act = act;
                cpu.set_nmi(nmi_act);
                clr(Src::rstr); // auto clr (pulse)
            }

            act = (state & Src::irq);
            if (irq_act != act) {
                irq_act = act;
                cpu.set_irq(irq_act);
            }
        }
    }

private:
    NMOS6502::Core& cpu; // TODO: replace with a generic signal?
    u8 state;
    u8 old_state;
    bool nmi_act;
    bool irq_act;
};


class Port {
public:
    // connections for external input/output
    using PD_in  = Sig2<u8, u8>; // bits, bit_vals
    using PD_out = Sig1<u8>; // bit_vals

    Port(const PD_out& ext_out_) : ext_out(ext_out_) { reset(); }

    /*  The port pins are set as inputs and port registers to
        zero (although a read of the ports will return all high
        because of passive pullups).
    */
    void reset() {
        set_dd(0b00000000);
        p_in  = 0b11111111;
        p_out = 0b00000000;
    }

    u8   r_dd() const     { return out_bits; }
    void w_dd(u8 dd)      { set_dd(dd); output(); }

    u8   r_pd() const     { return (((p_out & out_bits) | in_bits()) & p_in); }
    void w_pd(u8 d)       { p_out = d; output(); }

    void _set_p_out(u8 bits, u8 vals) { // for CIA to directly set port value (timer PB usage)
        p_out = (p_out & ~bits) | (vals & bits);
        output();
    }

    PD_in ext_in {
        [this](u8 e_in_bits, u8 e_bit_vals) {
            p_in = (p_in & ~e_in_bits) | (e_in_bits & e_bit_vals);
        }
    };

private:
    void set_dd(u8 dd) { out_bits = dd; }

    u8 in_bits() const { return ~out_bits; }

    void output() const { ext_out((p_out & out_bits) | in_bits()); }

    const PD_out& ext_out;

    u8 out_bits;
    u8 p_in;
    u8 p_out;
};


} // namespace IO


#endif // IO_H_INCLUDED
