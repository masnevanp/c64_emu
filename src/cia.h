#ifndef CIA_H_INCLUDED
#define CIA_H_INCLUDED

#include "common.h"


namespace CIA {


// TODO: SDR
class Core {
public:
    using PD_in  = IO::Port::PD_in;
    using PD_out = IO::Port::PD_out;

    enum Reg : u8 {
        pra = 0,  prb,     ddra,    ddrb,   ta_lo, ta_hi, tb_lo, tb_hi,
        tod_10th, tod_sec, tod_min, tod_hr, sdr,   icr,   cra,   crb,
    };

    enum CRA : u8 { inmode_mask_a = 0x20, todin = 0x80, };
    enum CRB : u8 { inmode_mask_b = 0x60, alarm = 0x80, };

    enum Timer_PB_bit : u8 { ta_pb_bit = 0x40, tb_pb_bit = 0x80 };

    IO::Port port_a;
    IO::Port port_b;

    Core(
        const PD_out& port_out_a, const PD_out& port_out_b,
        IO::Int_hub& int_hub_, IO::Int_hub::Src int_src_, const u64& system_cycle)
      :
        port_a(port_out_a), port_b(port_out_b),
        int_ctrl(int_hub_, int_src_),
        timer_b(int_ctrl, sig_null, cnt, inmode_mask_b, Int_ctrl::Int_src::tb, port_b, tb_pb_bit),
        timer_a(int_ctrl, timer_b.tick_ta, cnt, inmode_mask_a, Int_ctrl::Int_src::ta, port_b, ta_pb_bit),
        tod(timer_a.cr, int_ctrl, system_cycle) {}

    void reset_warm() { int_ctrl.reset(); }

    void reset_cold() {
        /*  Commodore spec (http://archive.6502.org/datasheets/mos_6526_cia_recreated.pdf):
            A low on the /RES pin resets all internal registers.
            The port pins are set as inputs and port registers to
            zero (although a read of the ports will return all high
            because of passive pullups). The timer control
            registers are set to zero and the timer latches to all
            ones. All other registers are reset to zero.
        */
        cnt = true;

        port_a.reset();
        port_b.reset();
        timer_a.reset();
        timer_b.reset();
        int_ctrl.reset();
        tod.reset();
    }

    void r(const u8& ri, u8& data) {
        _tick();
        sync.tick();

        switch (ri) {
            case pra:      data = port_a.r_pd();    return;
            case prb:      data = r_prb();          return;
            case ddra:     data = port_a.r_dd();    return;
            case ddrb:     data = port_b.r_dd();    return;
            case ta_lo:    data = timer_a.r_lo();   return;
            case ta_hi:    data = timer_a.r_hi();   return;
            case tb_lo:    data = timer_b.r_lo();   return;
            case tb_hi:    data = timer_b.r_hi();   return;
            case tod_10th: data = tod.r_10th();     return;
            case tod_sec:  data = tod.r_sec();      return;
            case tod_min:  data = tod.r_min();      return;
            case tod_hr:   data = tod.r_hr();       return;
            case sdr:      data = 0x00;             return; // TODO
            case icr:      data = int_ctrl.r_icr(); return;
            case cra:      data = timer_a.cr;       return;
            case crb:      data = timer_b.cr;       return;
        }
    }

    void w(const u8& ri, const u8& data) {
        //if (ri == cra) std::cout << "CIA-" << (int)id << ": w " << (int)ri << " " << (int)data << "\n"; //getchar();
        switch (ri) {
            case pra:      port_a.w_pd(data);    return;
            case prb:      port_b.w_pd(data);    return;
            case ddra:     port_a.w_dd(data);    return;
            case ddrb:     port_b.w_dd(data);    return;
            case ta_lo:    timer_a.w_lo(data);   return;
            case ta_hi:    timer_a.w_hi(data);   return;
            case tb_lo:    timer_b.w_lo(data);   return;
            case tb_hi:    timer_b.w_hi(data);   return;
            case tod_10th: tod.w_10th(data);     return;
            case tod_sec:  tod.w_sec(data);      return;
            case tod_min:  tod.w_min(data);      return;
            case tod_hr:   tod.w_hr(data);       return;
            case sdr:                            return; // TODO
            case icr:      int_ctrl.w_icr(data); return;
            case cra: // NOTE! Timers store the full CR (although they don't use all the bits)
                timer_a.w_cr(data);
                tod.upd_todin();
                return;
            case crb:
                timer_b.w_cr(data);
                tod.set_w_dest(data & CRB::alarm);
                return;
        }
    }

    void tick(const u64& system_cycle) { if (!sync.ticked(system_cycle)) _tick(); }

    void set_cnt(bool high) { // TODO (sdr...)
        if (high && !cnt) {
            timer_a.tick_cnt();
            timer_b.tick_cnt();
        }
        cnt = high;
    }

private:
    void _tick() {
        int_ctrl.tick();

        timer_a.tick_phi2();
        timer_b.tick_phi2();

        tod.tick();
    }

    Cycle_sync sync;

    const Sig sig_null = [](){};

    u8 r_prb() { // includes the (possible) timer pb-bits to read value
        // TODO: This ignores the possible 0 value on an input bit
        //       (it would pull the port value to zero too, right?)
        u8 t_bits = timer_a.pb_bit | timer_b.pb_bit;
        u8 t_vals = timer_a.pb_bit_val | timer_b.pb_bit_val;
        return (port_b.r_pd() & ~t_bits) | (t_vals & t_bits);
    }


    class Int_ctrl {
    public:
        enum Int_src : u8 {
            ta = 0x01, tb = 0x02, alrm = 0x04, sp = 0x08, flg = 0x10,
        };
        enum ICR_R : u8 {
            ir = 0x80,
        };
        enum ICR_W : u8 {
            sc = 0x80,
        };

        Int_ctrl(IO::Int_hub& int_hub_, IO::Int_hub::Src int_id_)
                    : int_hub(int_hub_), int_id(int_id_)
        { reset(); }

        void reset() { new_icr = icr = mask = 0x00; }

        void set(const u8& int_src) { icr |= int_src; }

        u8 r_icr() { // 'acked'
            u8 r = icr;
            icr = 0x00;
            return r;
        }
        void w_icr(const u8& data) { new_icr = data | 0x100; }

        void tick() {
            if (icr & mask) {
                icr |= ICR_R::ir;
                int_hub.set(int_id);
            } else {
                int_hub.clr(int_id);
            }

            if (new_icr) {
                if (new_icr & 0x100) {
                    new_icr &= 0xff;
                } else {
                    mask = (new_icr & ICR_W::sc) // set/clr?
                                ? ((mask | new_icr) & ~ICR_W::sc) // set
                                : (mask & ~new_icr); // clr
                    new_icr = 0x00;
                }
            }
        }

    private:
        u16 new_icr;

        u8 icr;
        u8 mask;

        IO::Int_hub& int_hub;
        const IO::Int_hub::Src int_id;
    };


    // Implements both timer A&B functionality (they differ slightly)
    class Timer {
    public:
        enum CR {
            run = 0x01,   pbon   = 0x02, togglemode = 0x04, oneshot = 0x08,
            load  = 0x10, sp_out = 0x40,
        };
        enum Inmode { im_phi2 = 0, im_cnt, im_ta, im_ta_w_cnt };

        Timer(
            Int_ctrl& int_ctrl_, const Sig& sig_underflow_, bool& cnt_,
            u8 inmode_mask_, u8 int_src_,
            IO::Port& port_b_,
            u8 pb_bit_pos_
        )
          : int_ctrl(int_ctrl_), sig_underflow(sig_underflow_), cnt(cnt_),
            inmode_mask(inmode_mask_), int_src(int_src_),
            port_b(port_b_), pb_bit_pos(pb_bit_pos_)
        {
            reset();
        }

        void reset() {
            timer = latch = 0xffff;
            t_ctrl = t_os = pb_bit = pb_pulse = pb_toggle = 0x00;
        }

        /* NOTE:
             Timer itself outputs just the pb-bit (not all the output bits of port B).
             OTOH, the 'real' port B output will include also the timer pb-bit.
             Might this cause problems on the receiving end?
             (Need to maybe do some bit stitching? We'll see...)
        */

        u8 cr;

        u8 pb_bit;
        u8 pb_bit_val;

        u8 r_lo() const { return timer; }
        u8 r_hi() const { return timer >> 8; }

        void w_lo(const u8& data) {
            latch = (latch & 0xff00) | data;
        }
        void w_hi(const u8& data) {
            latch = (latch & 0x00ff) | (data << 8);
            if (!is_running()) reload();
        }

        void w_cr(const u8& data) {
            inmode = (data & inmode_mask) >> 5;

            if (data & CR::run) {
                if (!is_running()) pb_toggle = pb_bit_pos;
                if (inmode == im_phi2) t_ctrl |= (timer == 0x0000) ? 0x10 : 0x40;
            }

            if (data & CR::load) {
                t_ctrl |= 0x20;  // load @ t+2
                cr = (data & ~CR::load); // bit not stored
            } else {
                cr = data;
            }

            pb_bit = (cr & CR::pbon) ? pb_bit_pos : 0x00;

            update_pb();
        }

        void tick_phi2() {
            if (pb_pulse) {
                pb_pulse = 0x00;
                update_pb();
            }

            tick();

            // queue up a tick @ t+2 (possibly)
            t_ctrl |= (((inmode == im_phi2) & (cr & CR::run)) << 4);
        }

        void tick_cnt()  {
            // TODO: sdr
            // if (inmode == im_cnt) t_ctrl |= ((timer == 0x0000) ? 0x04 : 0x10); ???
        }

        Sig tick_ta  { // on timer A underflow (only ever called for timer B)
            [this]() {
                if (is_running()) {
                    if ((inmode == im_ta) || ((inmode == im_ta_w_cnt) && cnt)) {
                        t_ctrl |= ((timer == 0x0000) ? 0x04 : 0x10); // tick @ t+1|t+2
                    }
                }
            }
        };

    private:
        Int_ctrl& int_ctrl;
        const Sig& sig_underflow;

        bool& cnt;

        u8 inmode_mask;
        u8 int_src;
        IO::Port& port_b;

        const u8 pb_bit_pos;

        u8 t_ctrl;
        u8 t_os;

        u8 inmode;

        u16 timer;
        u16 latch;

        u8 pb_toggle;
        u8 pb_pulse;

        void reload()                  { timer = latch; }
        void record_one_shot()         { t_os >>= 1; t_os |= (cr & CR::oneshot); }
        void check_one_shot() {
            if (t_os & 0x6) { // (was it OS @t+1|t+2)
                if (!(t_ctrl & 0x10)) { // (not (re)started @t+0?)
                    cr &= (~CR::run);
                }
                t_ctrl &= 0xfa; // remove 2 ticks
                t_os = 0x00;
            }
        }

        bool is_running() const        { return cr & CR::run; }
        bool is_pb_on() const          { return pb_bit; }
        bool is_one_shot() const       { return cr & CR::oneshot; }
        bool is_in_toggle_mode() const { return cr & CR::togglemode; }

        //Takes a stream of 2bit actions ('x1' -> tick, '1x' -> force load) from t_ctrl.
        void tick() {
            record_one_shot();

            // pop next
            auto action = t_ctrl;
            t_ctrl >>= 2;

            if (action & 0x1) { // tick?
                if (timer > 0x0001) {
                    --timer;
                    goto _check_reload;
                }

                if (timer == 0x0001) {
                    timer = 0x0000;
                    if (t_ctrl & 0x1) t_ctrl ^= 0x1; // remove next tick
                    else goto _check_reload;
                } else if (!is_pb_on()) { // timer == 0x0000
                    t_ctrl &= ~0x1; // remove next tick
                }

                underflow();
                reload();
                return;
            }

        _check_reload:
            if (action & 0x2) { // force reload?
                if ((t_ctrl & 0x1) && timer == 0x0000 && latch != 0x0000) {
                    underflow();
                }
                reload();
                t_ctrl &= ~0x1; // remove next tick
            }
        }

        void underflow() {
            int_ctrl.set(int_src);
            sig_underflow();
            tick_pb();
            check_one_shot();
        }

        void tick_pb() {
            pb_toggle ^= pb_bit_pos;
            pb_pulse = pb_bit;
            update_pb();
        }

        void update_pb() {
            pb_bit_val = is_in_toggle_mode() ? pb_toggle : pb_pulse;
            if (pb_bit) port_b._set_p_out(pb_bit, pb_bit_val);
        }
    };


    class TOD {
    public:
        enum HR { am = 0x00, pm = 0x80 };

        TOD(const u8& cra_, Int_ctrl& int_ctrl_, const u64& system_cycle_)
            : cra(cra_), int_ctrl(int_ctrl_), system_cycle(system_cycle_)
        { reset(); }

        void reset() {
            time  = Time{0x00, 0x00, 0x00, 0x01};
            alarm = Time{0xff, 0xff, 0xff, 0xff};
            r_src = w_dest = &time;
            stop();
        }

        void upd_todin() { if (running()) start(); }
        void set_w_dest(bool al) { w_dest = al ? &alarm : &time; }

        u8 r_10th() {
            u8 tnth = r_src->tnth;
            r_src = &time;
            return tnth;
        }
        u8 r_sec() const { return to_bcd(r_src->sec); }
        u8 r_min() const { return to_bcd(r_src->min); }
        u8 r_hr() {
            time_latch = time;
            r_src = &time_latch;
            u8 hr = r_src->hr % 12;
            hr = (hr == 0) ? 12 : hr;
            return (r_src->hr > 11 ? HR::pm : HR::am) | to_bcd(hr);
        }

        void w_10th(const u8& data) {
            if (w_dest == &time) start();
            w_dest->tnth = data & 0xf;
        }
        void w_sec(const u8& data) { w_dest->sec = (to_u8(data) & 0x7f); }
        void w_min(const u8& data) { w_dest->min = (to_u8(data) & 0x7f); }
        void w_hr(const u8& data)  {
            if (w_dest == &time) stop();
            u8 hr = to_u8(data & 0x1f);
            if (data & HR::pm) hr = (hr < 12) ? hr + 12 : hr;
            else hr = (hr == 12) ? 0 : hr;
            w_dest->hr = hr;
        }

        void tick() {
            if (running()) {
                if (system_cycle % tick_freq == 0) {
                    time.tick();
                    if (time == alarm) int_ctrl.set(Int_ctrl::Int_src::alrm);
                }
            }
        }

    private:
        static u8 to_bcd(u8 val) { return ((val / 10) << 4) | (val % 10); }
        static u8 to_u8(u8 bcd)  { return ((bcd >> 4) * 10) + (bcd & 0xf); }

        void start() {
            const bool todin_50hz = cra & CRA::todin;
            tick_freq = std::round((CPU_FREQ_PAL / 50) * (todin_50hz ? 5 : 6));
        }
        void stop() { tick_freq = 0; }
        bool running() const { return tick_freq; }

        struct Time {
            u8 tnth;
            u8 sec;
            u8 min;
            u8 hr;

            void tick() {
                if (++tnth > 9) {
                    tnth = 0;
                    if (++sec > 59) {
                        sec = 0;
                        if (++min > 59) {
                            min = 0;
                            if (++hr > 23) hr = 0;
                        }
                    }
                }
            }

            bool operator==(const Time& ct) {
                return (tnth == ct.tnth) && (sec == ct.sec) && (min == ct.min) && (hr == ct.hr);
            }
        };

        Time time;
        Time time_latch;
        Time alarm;

        u32 tick_freq;

        Time* r_src;
        Time* w_dest;

        const u8& cra;

        Int_ctrl& int_ctrl;
        const u64& system_cycle;
    };

    bool cnt;

    Int_ctrl int_ctrl;
    Timer timer_b;
    Timer timer_a;
    TOD tod;
};


} // namespace CIA


#endif // CIA_H_INCLUDED
