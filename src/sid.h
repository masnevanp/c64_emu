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
    }

    void reset() { re_sid.reset(); }

    void flush() {
        audio_out.flush();
        //audio_out.put(buf, BUF_SZ / 2); // some breathing room
    }

    void output(bool frame_done) {
        tick();
        audio_out.put(buf, buf_ptr - buf);
        buf_ptr = buf;
        if (frame_done) ticked = 0; // 'frame_cycle' will also get reset (elsewhere)
    }

    void r(const u8& ri, u8& data) { data = re_sid.read(ri); }

    void w(const u8& ri, const u8& data) {
        tick(); // tick with old state first
        re_sid.write(ri, data);
    }

private:
    reSID::SID re_sid;

    // TODO: non-hard coding
    // output 6 times per frame  => ~147.0 samples
    static const u32 BUF_SZ = 152;

    i16 buf[BUF_SZ];
    i16* buf_ptr = buf;

    u16 ticked = 0; // how many times has re-sid been ticked (for upcoming burst)
    const u16& frame_cycle;

    Host::Audio_out audio_out;

    void tick() {
        int n = frame_cycle - ticked;
        if (n) {
            // there is always enough space in the buffer (hence the '0xffff')
            buf_ptr += re_sid.clock(n, buf_ptr, 0xffff);
            ticked = frame_cycle;
        }
    }
};


#endif // SID_H_INCLUDED
