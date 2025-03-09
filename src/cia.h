#ifndef CIA_H_INCLUDED
#define CIA_H_INCLUDED

#include "common.h"
#include "state.h"


namespace CIA {


using CS = State::CIA;


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

    Core(
        CS& cs_,
        const PD_out& port_out_a, const PD_out& port_out_b,
        IO::Int_sig& int_sig_, IO::Int_sig::Src int_src_, const u64& system_cycle)
      :
        port_a(cs_.port_a, port_out_a), port_b(cs_.port_b, port_out_b),
        s(cs_),
        int_ctrl(cs_.int_ctrl, int_sig_, int_src_),
        timer_b(cs_.timer_b, Int_ctrl::Int_src::tb, tb_pb_bit, cs_.cnt, int_ctrl, sig_null, port_b),
        timer_a(cs_.timer_a, Int_ctrl::Int_src::ta, ta_pb_bit, cs_.cnt, int_ctrl, timer_b.tick_ta, port_b),
        tod(cs_.tod, timer_a.s.cr, int_ctrl, system_cycle) {}

    IO::Port port_a;
    IO::Port port_b;

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
        s.cnt = true;

        port_a.reset();
        port_b.reset();
        timer_a.reset();
        timer_b.reset();
        int_ctrl.reset();
        tod.reset();
    }

    void r(const u8& ri, u8& data) {
        _tick();
        r_ticked = true;

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
            case cra:      data = timer_a.s.cr;     return;
            case crb:      data = timer_b.s.cr;     return;
        }
    }

    void w(const u8& ri, const u8& data) {
        //if (ri == cra) std::cout << "CIA-" << (int)id << ": w " << (int)ri << " " << (int)data << "\n"; //getchar();
        switch (ri) {
            case pra:      port_a.w_pd(data);      return;
            case prb:      port_b.w_pd(data);      return;
            case ddra:     port_a.w_dd(data);      return;
            case ddrb:     port_b.w_dd(data);      return;
            case ta_lo:    timer_a.w_lo(data);     return;
            case ta_hi:    timer_a.w_hi(data);     return;
            case tb_lo:    timer_b.w_lo(data);     return;
            case tb_hi:    timer_b.w_hi(data);     return;
            case tod_10th: tod.w_10th(data & 0xf); return;
            case tod_sec:  tod.w_sec(data & 0x7f); return;
            case tod_min:  tod.w_min(data & 0x7f); return;
            case tod_hr:   tod.w_hr(data & 0x9f);  return;
            case sdr:                              return; // TODO
            case icr:      int_ctrl.w_icr(data);   return;
            case cra: // NOTE! Timers store the full CR (although they don't use all the bits)
                timer_a.w_cr(data, inmode_mask_a);
                return;
            case crb:
                timer_b.w_cr(data, inmode_mask_b);
                tod.set_w_dst(CS::TOD::Time_rep::Kind(data >> 7));
                return;
        }
    }

    void tick() { if (!r_ticked) _tick(); else r_ticked = false; }

    void set_cnt(bool high) { // TODO (sdr...)
        if (high && !s.cnt) {
            timer_a.tick_cnt();
            timer_b.tick_cnt();
        }
        s.cnt = high;
    }

private:
    void _tick() {
        int_ctrl.tick();

        timer_a.tick_phi2();
        timer_b.tick_phi2();

        tod.tick();
    }

    // to keep in sync with the cpu (if cpu does a read, we must tick first)
    bool r_ticked = false;

    const Sig sig_null = [](){};

    u8 r_prb() { // includes the (possible) timer pb-bits to read value
        // TODO: This ignores the possible 0 value on an input bit
        //       (it would pull the port value to zero too, right?)
        u8 t_bits = timer_a.s.pb_bit | timer_b.s.pb_bit;
        u8 t_vals = timer_a.s.pb_bit_val | timer_b.s.pb_bit_val;
        return (port_b.r_pd() & ~t_bits) | (t_vals & t_bits);
    }


    class Int_ctrl {
    public:
        using State = CS::Int_ctrl;

        enum Int_src : u8 {
            ta = 0x01, tb = 0x02, alrm = 0x04, sp = 0x08, flg = 0x10,
        };
        enum ICR_R : u8 {
            ir = 0x80,
        };
        enum ICR_W : u8 {
            sc = 0x80,
        };

        Int_ctrl(State& s_, IO::Int_sig& int_sig_, IO::Int_sig::Src int_id_)
                    : s(s_), int_sig(int_sig_), int_id(int_id_)
        { reset(); }

        void reset() { s.new_icr = s.icr = s.mask = 0x00; }

        void set(const u8& int_src) { s.icr |= int_src; }

        u8 r_icr() { // 'acked'
            u8 r = s.icr;
            s.icr = 0x00;
            return r;
        }
        void w_icr(const u8& data) { s.new_icr = data | 0x100; }

        void tick() {
            if (s.icr & s.mask) {
                s.icr |= ICR_R::ir;
                int_sig.set(int_id);
            } else {
                int_sig.clr(int_id);
            }

            if (s.new_icr) {
                if (s.new_icr & 0x100) {
                    s.new_icr &= 0xff;
                } else {
                    s.mask = (s.new_icr & ICR_W::sc) // set/clr?
                                ? ((s.mask | s.new_icr) & ~ICR_W::sc) // set
                                : (s.mask & ~s.new_icr); // clr
                    s.new_icr = 0x00;
                }
            }
        }

    private:
        State& s;

        IO::Int_sig& int_sig;
        const IO::Int_sig::Src int_id;
    };


    // Implements both timer A&B functionality (they differ slightly)
    class Timer {
    public:
        using State = CS::Timer;

        enum CR {
            run = 0x01,   pbon   = 0x02, togglemode = 0x04, oneshot = 0x08,
            load  = 0x10, sp_out = 0x40,
        };
        enum Inmode { im_phi2 = 0, im_cnt, im_ta, im_ta_w_cnt };

        Timer(
            State& s_,
            u8 int_src_, u8 pb_bit_pos_, bool& cnt_,
            Int_ctrl& int_ctrl_, const Sig& sig_underflow_, IO::Port& port_b_
        )
          : s(s_),
            int_src(int_src_), pb_bit_pos(pb_bit_pos_), cnt(cnt_),
            int_ctrl(int_ctrl_), sig_underflow(sig_underflow_), port_b(port_b_)
        {
            reset();
        }

        State& s;

        void reset() {
            s.timer = s.latch = 0xffff;
            s.t_ctrl = s.t_os = s.pb_bit = s.pb_pulse = s.pb_toggle = 0x00;
        }

        /* NOTE:
             Timer itself outputs just the pb-bit (not all the output bits of port B).
             OTOH, the 'real' port B output will include also the timer pb-bit.
             Might this cause problems on the receiving end?
             (Need to maybe do some bit stitching? We'll see...)
        */

        u8 r_lo() const { return s.timer; }
        u8 r_hi() const { return s.timer >> 8; }

        void w_lo(const u8& data) {
            s.latch = (s.latch & 0xff00) | data;
        }
        void w_hi(const u8& data) {
            s.latch = (s.latch & 0x00ff) | (data << 8);
            if (!is_running()) reload();
        }

        void w_cr(const u8& data, u8 inmode_mask) {
            s.inmode = (data & inmode_mask) >> 5;

            if (data & CR::run) {
                if (!is_running()) s.pb_toggle = pb_bit_pos;
                if (s.inmode == im_phi2) s.t_ctrl |= (s.timer == 0x0000) ? 0x10 : 0x40;
            }

            if (data & CR::load) {
                s.t_ctrl |= 0x20;  // load @ t+2
                s.cr = (data & ~CR::load); // bit not stored
            } else {
                s.cr = data;
            }

            s.pb_bit = (s.cr & CR::pbon) ? pb_bit_pos : 0x00;

            update_pb();
        }

        void tick_phi2() {
            if (s.pb_pulse) {
                s.pb_pulse = 0x00;
                update_pb();
            }

            tick();

            // queue up a tick @ t+2 (possibly)
            s.t_ctrl |= (((s.inmode == im_phi2) & (s.cr & CR::run)) << 4);
        }

        void tick_cnt()  {
            // TODO: sdr
            // if (inmode == im_cnt) t_ctrl |= ((timer == 0x0000) ? 0x04 : 0x10); ???
        }

        Sig tick_ta  { // on timer A underflow (only ever called for timer B)
            [this]() {
                if (is_running()) {
                    if ((s.inmode == im_ta) || ((s.inmode == im_ta_w_cnt) && cnt)) {
                        s.t_ctrl |= ((s.timer == 0x0000) ? 0x04 : 0x10); // tick @ t+1|t+2
                    }
                }
            }
        };

    private:
        // TODO: get rid of these...?
        const u8 int_src;
        const u8 pb_bit_pos;

        bool& cnt;

        Int_ctrl& int_ctrl;
        const Sig& sig_underflow;
        IO::Port& port_b;

        void reload()                  { s.timer = s.latch; }
        void record_one_shot()         { s.t_os >>= 1; s.t_os |= (s.cr & CR::oneshot); }
        void check_one_shot() {
            if (s.t_os & 0x6) { // (was it OS @t+1|t+2)
                if (!(s.t_ctrl & 0x10)) { // (not (re)started @t+0?)
                    s.cr &= (~CR::run);
                }
                s.t_ctrl &= 0xfa; // remove 2 ticks
                s.t_os = 0x00;
            }
        }

        bool is_running() const        { return s.cr & CR::run; }
        bool is_pb_on() const          { return s.pb_bit; }
        bool is_one_shot() const       { return s.cr & CR::oneshot; }
        bool is_in_toggle_mode() const { return s.cr & CR::togglemode; }

        //Takes a stream of 2bit actions ('x1' -> tick, '1x' -> force load) from t_ctrl.
        void tick() {
            record_one_shot();

            // pop next
            auto action = s.t_ctrl;
            s.t_ctrl >>= 2;

            if (action & 0x1) { // tick?
                if (s.timer > 0x0001) {
                    --s.timer;
                    goto _check_reload;
                }

                if (s.timer == 0x0001) {
                    s.timer = 0x0000;
                    if (s.t_ctrl & 0x1) s.t_ctrl ^= 0x1; // remove next tick
                    else goto _check_reload;
                } else if (!is_pb_on()) { // timer == 0x0000
                    s.t_ctrl &= ~0x1; // remove next tick
                }

                underflow();
                reload();
                return;
            }

        _check_reload:
            if (action & 0x2) { // force reload?
                if ((s.t_ctrl & 0x1) && s.timer == 0x0000 && s.latch != 0x0000) {
                    underflow();
                }
                reload();
                s.t_ctrl &= ~0x1; // remove next tick
            }
        }

        void underflow() {
            int_ctrl.set(int_src);
            sig_underflow();
            tick_pb();
            check_one_shot();
        }

        void tick_pb() {
            s.pb_toggle ^= pb_bit_pos;
            s.pb_pulse = s.pb_bit;
            update_pb();
        }

        void update_pb() {
            s.pb_bit_val = is_in_toggle_mode() ? s.pb_toggle : s.pb_pulse;
            if (s.pb_bit) port_b._set_p_out(s.pb_bit, s.pb_bit_val);
        }
    };


    // many thanks to 'VICE/testprogs/CIA/tod'
    class TOD {
    public:
        using State = CS::TOD;
        using TR_kind = State::Time_rep::Kind;

        TOD(State& s_, const u8& cra_, Int_ctrl& int_ctrl_, const u64& system_cycle_)
            : s(s_), cra(cra_), int_ctrl(int_ctrl_), system_cycle(system_cycle_)
        { reset(); }

        void reset() {
            s.time[TR_kind::tod] = State::Time_rep{0x01, 0x00, 0x00, 0x00};
            s.time[TR_kind::alarm] = State::Time_rep{0x00, 0x00, 0x00, 0x00};
            s.r_src = s.w_dst = TR_kind::tod;
            stop();
        }

        void set_w_dst(TR_kind t) { s.w_dst = t; }

        u8 r_10th() {
            const u8 tnth = s.time[s.r_src].tnth;
            s.r_src = TR_kind::tod;
            return tnth;
        }
        u8 r_sec() const { return s.time[s.r_src].sec; }
        u8 r_min() const { return s.time[s.r_src].min; }
        u8 r_hr() {
            if (s.r_src == TR_kind::tod) { // don't re-latch before unlatched
                s.time[TR_kind::latch] = s.time[TR_kind::tod];
                s.r_src = TR_kind::latch;
            }
            return s.time[s.r_src].hr;
        }

        void w_10th(const u8& data) {
            if (s.time[s.w_dst].tnth != data) {
                s.time[s.w_dst].tnth = data;
                check_alarm();
            }
            if (s.w_dst == TR_kind::tod && !running()) start();
        }
        void w_sec(const u8& data) {
            if (s.time[s.w_dst].sec != data) {
                s.time[s.w_dst].sec = data;
                check_alarm();
            }
        }
        void w_min(const u8& data) {
            if (s.time[s.w_dst].min != data) {
                s.time[s.w_dst].min = data;
                check_alarm();
            }
        }
        void w_hr(u8 data)  {
            if (s.w_dst == TR_kind::tod) {
                const u8 hr = data & 0x1f;
                if (hr == 0x12) data ^= HR::pm; // am/pm flip (TOD only)
                stop();
            }

            if (s.time[s.w_dst].hr != data) {
                s.time[s.w_dst].hr = data;
                check_alarm();
            }
        }

        void tick() {
            const bool tod_pin_pulse = (system_cycle % tod_pin_freq == 0);
            if (tod_pin_pulse && running()) {
                const bool do_tod_tick = (s.tod_tick_timer == (6 - (cra >> 7)));
                if (do_tod_tick) {
                    s.tod_tick_timer = 1;
                    tick_tod();
                    check_alarm();
                } else {
                    if (++s.tod_tick_timer == 7) s.tod_tick_timer = 1;
                }
            }
        }

    private:
        static constexpr u32 tod_pin_freq = u32((CPU_FREQ_PAL / 50) + 0.5); // round like a pro...

        enum HR { pm = 0x80 };

        void start()         { s.tod_tick_timer = 1; }
        void stop()          { s.tod_tick_timer = 0; }
        bool running() const { return s.tod_tick_timer > 0; }
        void check_alarm() {
            if (s.time[TR_kind::tod] == s.time[TR_kind::alarm]) int_ctrl.set(Int_ctrl::Int_src::alrm);
        }

        void tick_tod() {
            State::Time_rep& t = s.time[TR_kind::tod];

            auto tick_hr = [&]() {
                u8 hr_pm = t.hr & HR::pm;
                t.hr = (t.hr + 1) & 0x1f;
                if (t.hr == 0x0a) t.hr = 0x10;
                else if (t.hr == 0x12) hr_pm ^= HR::pm;
                else if (t.hr == 0x13) t.hr = 0x01;
                else if (t.hr == 0x10) t.hr = 0x00;
                else if (t.hr == 0x00) t.hr = 0x10;
                t.hr |= hr_pm;
            };

            auto tick_min = [&]() {
                u8 min_h = t.min & 0x70;
                u8 min_l = (t.min + 1) & 0xf;
                if (min_l == 10) {
                    min_l = 0;
                    min_h = (min_h + 0x10) & 0x70;
                    if (min_h == 0x60) {
                        min_h = 0x00;
                        tick_hr();
                    }
                }
                t.min = min_l | min_h;
            };

            auto tick_sec = [&]() {
                u8 sec_h = t.sec & 0x70;
                u8 sec_l = (t.sec + 1) & 0xf;
                if (sec_l == 10) {
                    sec_l = 0;
                    sec_h = (sec_h + 0x10) & 0x70;
                    if (sec_h == 0x60) {
                        sec_h = 0x00;
                        tick_min();
                    }
                }
                t.sec = sec_l | sec_h;
            };

            auto tick_tnth = [&]() {
                t.tnth = (t.tnth + 1) & 0xf;
                if (t.tnth == 10) {
                    t.tnth = 0;
                    tick_sec();
                }
            };

            tick_tnth();
        }

        State& s;

        const u8& cra;

        Int_ctrl& int_ctrl;
        const u64& system_cycle;
    };

    CS& s;

    Int_ctrl int_ctrl;
    Timer timer_b;
    Timer timer_a;
    TOD tod;
};


} // namespace CIA


#endif // CIA_H_INCLUDED
