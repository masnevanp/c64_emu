#ifndef IO_H_INCLUDED
#define IO_H_INCLUDED


#include "nmos6502/nmos6502_core.h"


namespace IO {


namespace Sync {
    class Master {
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
    };
}


class Int_hub {
public:
    Int_hub(NMOS6502::Core& cpu_) : cpu(cpu_) { reset(); }

    void reset() {
        nmi_state = irq_state = false;
        nmi_req_cnt = irq_req_cnt = 0x00;
    }

    Int_sig nmi {
        [this]() { ++nmi_req_cnt; }, // set
        [this]() { --nmi_req_cnt; }, // clr
    };
    Int_sig irq {
        [this]() { ++irq_req_cnt; }, // set
        [this]() { --irq_req_cnt; }, // clr
    };

    void tick() {
        bool new_state = (nmi_req_cnt > 0);
        if (nmi_state != new_state) {
            nmi_state = new_state;
            cpu.set_nmi(nmi_state);
        }

        new_state = (irq_req_cnt > 0);
        if (irq_state != new_state) {
            irq_state = new_state;
            cpu.set_irq(irq_state);
        }
    }

private:
    NMOS6502::Core& cpu;
    bool nmi_state;
    bool irq_state;
    u8 nmi_req_cnt;
    u8 irq_req_cnt;
};


class Port {
public:
    // connections for external input/output
    using PD_in  = Sig2<u8, u8>; // bits, bit_vals
    using PD_out = Sig2<u8, u8>; // bits, bit_vals

    Port(const PD_out& ext_out_, u8 init_dd_ = 0xff) : ext_out(ext_out_), init_dd(init_dd_)
    {
        reset();
    }

    /*  The port pins are set as inputs and port registers to
        zero (although a read of the ports will return all high
        because of passive pullups).
    */
    void reset() {
        set_dd(init_dd);
        p_in     = 0xff;
        p_out    = 0x00;
    }

    u8   r_dd() const     { return out_bits; }
    void w_dd(u8 dd)      { set_dd(dd); output(); }

    //u8   r_pd() const     { return (p_out & out_bits & p_in) | (p_in & in_bits); }
    u8   r_pd() const     { return (((p_out & out_bits) | in_bits) & p_in); }
    void w_pd(u8 d)       { p_out = d; output(); }

    void _set_p_out(u8 bits, u8 vals) { // for CIA to directly set port value (timer PB usage)
        p_out = (p_out & ~bits) | (vals & bits);
    }

    PD_in ext_in {
        [this](u8 e_in_bits, u8 e_bit_vals) {
            p_in = (p_in & ~e_in_bits) | (e_in_bits & e_bit_vals);
        }
    };

private:
    void set_dd(u8 dd) {
        out_bits = dd;
        in_bits = ~dd;
    }

    void output() const { ext_out(out_bits, p_out & out_bits); }

    const PD_out& ext_out;

    u8 init_dd;

    u8 in_bits;
    u8 out_bits;
    u8 p_in;
    u8 p_out;
};


class Keyboard_matrix {
public:
    Keyboard_matrix(Port::PD_in& port_a_in_, Port::PD_in& port_b_in_)
        : port_a_in(port_a_in_), port_b_in(port_b_in_) { reset(); }

    void reset() {
        for (auto& kd : k_down) kd = 0x00;
        //pa_in_bits = pb_in_bits = 0xff;
        pa_out_vals = pb_out_vals = 0xff;
    }

    // Ports on CIA1 shall output to these
    void port_a_out(u8 bits, u8 bit_vals) {
        //pa_in_bits = ~bits;
        pa_out_vals = (bits & bit_vals) | (~bits); // in-bits will be set
        if (any_key_down) signal_state();
    }
    void port_b_out(u8 bits, u8 bit_vals) {
        //pb_in_bits = ~bits;
        pb_out_vals = (bits & bit_vals) | (~bits); // in-bits will be set
        if (any_key_down) signal_state();
    }

    Sig_key handler {
        [this](const Key_event& ev) {
            u8 row = ev.code / 8;
            u8 col_bit = 0x01 << (ev.code & 0x7);
            k_down[row] = ev.down ? k_down[row] | col_bit : k_down[row] & (~col_bit);
            signal_state();
        }
    };

private:
    void signal_state() {
        /* In short:
           If key down then do an AND between the corresponding pa & pb bits,
           and shove the resulting bit back to both ports
        */
        u8 state = 0xff;
        u8* kd_row = &k_down[0];
        any_key_down = false;
        for (u8 row_bit = 0x01; row_bit > 0x00; row_bit <<= 1, ++kd_row) {
            if (*kd_row) { // any key on row down?
                any_key_down = true;
                u8 pa_bit = pa_out_vals & row_bit;
                for (u8 col_bit = 0x01; col_bit > 0x00; col_bit <<= 1) {
                    u8 kd = *kd_row & col_bit;
                    if (kd) {
                        u8 pb_bit = pb_out_vals & col_bit;
                        if (!pa_bit || !pb_bit) state &= (~col_bit);
                    }
                }
            }
        }
        // std::cout << "Result: " << (int)result_val << "\n";
        //port_a_in(pa_in_bits, state);
        //port_b_in(pb_in_bits, state);
        port_a_in(0xff, state);
        port_b_in(0xff, state);
    }

    // current output value of the ports (any input bits will be set to 1)
    //u8 pa_in_bits;
    u8 pa_out_vals;
    //u8 pb_in_bits;
    u8 pb_out_vals;

    u8 k_down[8]; // 8x8 bits (for 64 keys)
    bool any_key_down;

    Port::PD_in& port_a_in;
    Port::PD_in& port_b_in;
};


class Joystick {
public:
    Joystick(Port::PD_in& port_in_) : port_in(port_in_) { }

    Sig_joy handler {
        [this](const Joystick_event& ev) {
            if (has_state_changed(ev)) {
                const u8 port_bit = 0x1 << ev.code;
                const u8 bit_val = ev.active ? 0x0 : port_bit;
                port_in(port_bit, bit_val);
                state = ev;
            }
        }
    };

private:
    bool has_state_changed(const Joystick_event& cmp) {
        return (cmp.active != state.active) || (cmp.code != state.code);
    }

    Port::PD_in& port_in;
    Joystick_event state;
};

} // namespace IO


#endif // IO_H_INCLUDED
