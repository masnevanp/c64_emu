
#include "cia.h"


void CIA::Core::reset_cold() {
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


void CIA::Core::TOD::tick_tod() {
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