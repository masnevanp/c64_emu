#ifndef SID_H_INCLUDED
#define SID_H_INCLUDED

#include "resid/sid.h"
#include "host.h"



// Using reSID by Dag Lem for core functionality
class reSID_Wrapper {
public:
    static const int OUTPUT_FREQ = 44100;

    reSID_Wrapper(const u16& frame_cycle_) : frame_cycle(frame_cycle_)
    {
        re_sid.set_sampling_parameters(CPU_FREQ, reSID::SAMPLE_RESAMPLE, OUTPUT_FREQ);
        reset();
    }

    void reset() {
        re_sid.reset();
        flush();
    }

    void flush() {
        audio_out.flush();
        buf_ptr = buf;
        ticked = 0;

        // queue something (whatever happens to be there) for some breathing room
        audio_out.put(buf, BUF_SZ);
        audio_out.put(buf, BUF_SZ / 4);
    }

    void tick() {
        int n = frame_cycle - ticked;
        if (n) {
            // there is always enough space in the buffer (hence the '0xffff')
            buf_ptr += re_sid.clock(n, buf_ptr, 0xffff);
            ticked = frame_cycle;
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
    const u16& frame_cycle;

    Host::Audio_out audio_out;

};


#endif // SID_H_INCLUDED
