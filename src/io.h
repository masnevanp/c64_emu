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


class Input_matrix {
public:
    Input_matrix(Port::PD_in& pa_in_, Port::PD_in& pb_in_) : pa_in(pa_in_), pb_in(pb_in_) {}

    void reset() {
        key_states = kb_matrix = 0;
        pa_state = pb_state = cp2_state = cp1_state = 0b11111111;
    }

    void cia1_pa_out(u8 state) { pa_state = state; output(); }
    void cia1_pb_out(u8 state) { pb_state = state; output(); }

    Sig_key keyboard {
        [this](u8 code, u8 down) {
            const auto key = u64{0b1} << (63 - code);
            key_states = down ? key_states | key : key_states & ~key;
            update_matrix();
            output();
        }
    };

    Sig_key ctrl_port_1 {
        [this](u8 code, u8 down) {
            const u8 bit_pos = 0b1 << code;
            const u8 bit_val = down ? 0 : bit_pos;
            cp1_state = (cp1_state & ~bit_pos) | bit_val;
            output();
        }
    };

    Sig_key ctrl_port_2 {
        [this](u8 code, u8 down) {
            const u8 bit_pos = 0b1 << code;
            const u8 bit_val = down ? 0 : bit_pos;
            cp2_state = (cp2_state & ~bit_pos) | bit_val;
            output();
        }
    };

private:
    void update_matrix() {
        auto get_row = [](int n, u64& from) -> u64 { return (from >> (8 * n)) & 0xff; };
        auto set_row = [](int n, u64& to, u64 val) { to |= (val << (8 * n)); };

        kb_matrix = key_states;

        //if (kb_matrix) { // any key down?
            // emulate GND propagation in the matrix (can produce 'ghost' keys)
            for (int ra = 0; ra < 7; ++ra) for (int rb = ra + 1; rb < 8; ++rb) {
                const u64 a = get_row(ra, kb_matrix);
                const u64 b = get_row(rb, kb_matrix);
                if (a & b) {
                    const u64 r = a | b;
                    set_row(ra, kb_matrix, r);
                    set_row(rb, kb_matrix, r);
                }
            }
        //}
    }

    void output() {
        u8 pa = pa_state & cp2_state;
        u8 pb = pb_state & cp1_state;

        //if (kb_matrix) { // any key down?
            u64 key = 0b1;
            for (int n = 0; n < 64; ++n, key <<= 1) {
                const bool key_down = kb_matrix & key;
                if (key_down) {
                    // figure out connected lines & states
                    const auto row_bit = (0b1 << (n / 8));
                    const auto pa_bit = pa & row_bit;
                    const auto col_bit = (0b1 << (n % 8));
                    const auto pb_bit = pb & col_bit;

                    if (!pa_bit || !pb_bit) { // at least one connected line is low?
                        // pull both lines low
                        pa &= (~row_bit);
                        pb &= (~col_bit);
                    }
                }
            }
        //}

        pa_in(0b11111111, pa);
        pb_in(0b11111111, pb); // TODO: vic.set_lp(VIC::LP_src::cia, pb & VIC_II::CIA1_PB_LP_BIT);
    }

    // key states, 8x8 bits (64 keys)
    u64 key_states; // 'actual' state
    u64 kb_matrix;  // key states in the matrix (includes possible 'ghost' keys)

    // current output state of the cia-ports (any input bit will be set)
    u8 pa_state;
    u8 pb_state;

    // current state of controllers
    u8 cp2_state;
    u8 cp1_state;

    Port::PD_in& pa_in;
    Port::PD_in& pb_in;
};


} // namespace IO


#endif // IO_H_INCLUDED
