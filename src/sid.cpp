
#include "sid.h"


void reSID_Wrapper::reconfig(double frame_rate, bool pitch_shift) {
    if (pitch_shift) {
        core.set_clock_freq(frame_rate * FRAME_CYCLE_COUNT);
        clock_speed_base = 1.0;
    } else {
        core.set_clock_freq(CPU_FREQ_PAL);
        clock_speed_base = FRAME_RATE_PAL / frame_rate;
    }
}


void reSID_Wrapper::output() {
    tick();

    const int buffered = audio_out.put(buf, buf_ptr - buf);

    const int buffered_lo = audio_out_buf_sz * 2;
    const int buffered_hi = 3 * buffered_lo;

    // Some crude clock speed control here. Good'nuff..? (Not tested on low-end machines...)
    if (buffered <= buffered_lo) {
        clock_speed = 1.01 * clock_speed_base;
    } else if (buffered >= buffered_hi) {
        const float reduction = 0.01 + (((buffered - buffered_hi) / audio_out_buf_sz) / 100.0);
        clock_speed = (1 - reduction) * clock_speed_base;
    } else {
        clock_speed = clock_speed_base;
    }

    buf_ptr = buf;
}


void reSID_Wrapper::tick() {
    int cycles = clock_speed * (system_cycle - last_tick_cycle);
    last_tick_cycle = system_cycle;

    if (cycles > 0) {
        // there is always enough space in the buffer (hence the '0xffff')
        buf_ptr += core.clock(cycles, buf_ptr, 0xffff);
    }
}
