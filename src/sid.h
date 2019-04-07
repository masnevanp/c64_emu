#ifndef SID_H_INCLUDED
#define SID_H_INCLUDED

#include "resid/sid.h"


namespace SID {


// Using reSID by Dag Lem for core functionality
class Wrapper {
public:
    static const int OUTPUT_FREQ = 44100;

    Wrapper(const u16& cycle_) : cycle(cycle_)
    {
        re_sid.set_sampling_parameters(CPU_FREQ, reSID::SAMPLE_RESAMPLE, OUTPUT_FREQ);
        reset();
    }

    void reset() {
        re_sid.reset();
        flush();
        ticked = 0;
    }

    void flush() {
        buf_ptr = buf;
        audio_out.flush();

        // queue something (whatever happens to be there) for some breathing room
        audio_out.put(buf, BUF_SZ);
        audio_out.put(buf, BUF_SZ / 4);
    }

    void tick() {
        int n = cycle - ticked;
        if (n) {
            // there is always enough space in the buffer (hence the '0xffff')
            buf_ptr += re_sid.clock(n, buf_ptr, 0xffff);
            ticked = cycle;
        }
    }

    void output() {
        tick();
        audio_out.put(buf, buf_ptr - buf);
        buf_ptr = buf;
        ticked = 0; // 'cycle' will also get reset (elsewhere)
    }

    void r(const u8& ri, u8& data) { data = re_sid.read(ri); }

    void w(const u8& ri, const u8& data) {
        tick(); // tick with old state first
        re_sid.write(ri, data);
    }

private:
    reSID::SID re_sid;

    // once per frame (312*63 = 19656 cycles) => ~879.8 samples
    static const u32 BUF_SZ = 880;

    i16 buf[BUF_SZ];
    i16* buf_ptr;

    u16 ticked; // how many times has re-sid been ticked (for upcoming burst)
    const u16& cycle; // current cycle number (of the system)

    Host::Audio_out audio_out;

};


} // namespace SID

#endif // SID_H_INCLUDED
