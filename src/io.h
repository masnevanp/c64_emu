#ifndef IO_H_INCLUDED
#define IO_H_INCLUDED


#include "nmos6502/nmos6502_core.h"


namespace IO {


namespace Sync {
    /*class Master {
    public:
        void tick() { sync_ref = !sync_ref; }
    private:
        bool sync_ref = false;
        friend class Slave;
    };

    class Slave {
    public:
        Slave(const Master& m) : sync_ref(m.sync_ref), sync_s(m.sync_ref) {}
        bool tick() {
            if (sync_s == sync_ref) return true;
            sync_s = sync_ref;
            return false;
        }
    private:
        const bool& sync_ref;
        bool sync_s;
    };*/

    class Cycle {
    public:
        void tick() { ++cycle; }
        bool ticked(const u64& system_cycle) {
            if (cycle == system_cycle) return true;
            tick();
            return false;
        }

    private:
        u64 cycle = 0;
    };
}


class Int_hub {
public:
    // TODO: irq/nmi source: cart./exp. port
    enum Src : u8 {
        cia1 = 0x01, vic  = 0x02, // IRQ sources
        cia2 = 0x10, rstr = 0x20, // NMI sources
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
        set_dd(0xff);
        p_in     = 0xff;
        p_out    = 0x00;
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


class Keyboard_matrix {
public:
    Keyboard_matrix(Port::PD_in& port_a_in_, Port::PD_in& port_b_in_)
        : port_a_in(port_a_in_), port_b_in(port_b_in_) { }

    void reset() {
        for (auto& kd : k_down) kd = 0b00000000;
        pa_out = pb_out = 0b11111111;
    }

    void port_a_output(u8 state) {
        pa_out = state;
        if (any_key_down) signal_state();
    }
    void port_b_output(u8 state) {
        pb_out = state;
        if (any_key_down) signal_state();
    }

    Sig_key handler {
        [this](u8 code, u8 down) {
            u8 row = code / 8;
            u8 col_bit = 0b1 << (code % 8);
            k_down[row] = down ? k_down[row] | col_bit : k_down[row] & (~col_bit);
            signal_state();
        }
    };

private:
    void signal_state() {
        /* In short:
           If key down then do an AND between the corresponding pa & pb bits,
           and shove the resulting bit back to both ports
        */
        u8 state = 0b11111111;
        u8* kd_row = &k_down[0];
        any_key_down = false;
        for (u8 row_bit = 0b00000001; row_bit; row_bit <<= 1, ++kd_row) {
            if (*kd_row) { // any key on row down?
                any_key_down = true;
                u8 pa_bit = pa_out & row_bit;
                for (u8 col_bit = 0b00000001; col_bit; col_bit <<= 1) {
                    u8 kd = *kd_row & col_bit;
                    if (kd) {
                        u8 pb_bit = pb_out & col_bit;
                        if (!pa_bit || !pb_bit) state &= (~col_bit);
                    }
                }
            }
        }

        port_a_in(0b11111111, state);
        port_b_in(0b11111111, state);
    }

    // current outputs of the ports (any input bit will be set)
    u8 pa_out;
    u8 pb_out;

    u8 k_down[8]; // 8x8 bits (for 64 keys)
    bool any_key_down;

    Port::PD_in& port_a_in;
    Port::PD_in& port_b_in;
};


} // namespace IO


#endif // IO_H_INCLUDED
