#ifndef SID_H_INCLUDED
#define SID_H_INCLUDED

#include "resid/sid.h"


namespace SID {


// Using reSID by Dag Lem for core functionality
class Wrapper {
public:
    static const int OUTPUT_FREQ = 44100;

    //static const int BUF_TICK_CNT = 100;
    static const int BUF_TICK_CNT = 2000; // 1ms = ~1000 ticks

    Wrapper()
    {
        re_sid.set_sampling_parameters(CPU_FREQ, reSID::SAMPLE_RESAMPLE, OUTPUT_FREQ);
        reset();
    }

    void reset() {
        re_sid.reset();
        flush();
    }

    void flush() {
        //buf_tick_cnt = next_buf_tick_cnt;
        buf_tick_cnt = BUF_TICK_CNT;
        tick_cnt = 0;
        buf_ptr = buf;
        audio_out.flush();
    }

    void tick(bool do_output = true) {
        if (!do_output) {
            re_sid.clock();
            return;
        } else if (++tick_cnt < buf_tick_cnt) {
            return;
        }

        while (tick_cnt) {
            int n = re_sid.clock(tick_cnt, buf_ptr, buf_end - buf_ptr);
            buf_ptr += n;
            if (buf_ptr == buf_end) {
                audio_out.put(buf, BUF_SZ);
                buf_ptr = buf;
            }
        }
        if (buf_ptr != buf) {
            audio_out.put(buf, buf_ptr - buf);
            buf_ptr = buf;
        }

        buf_tick_cnt = 1;
    }

    void r(const u8& ri, u8& data) {
        //std::cout << "SID r: " << (int)ri << "\n";
        data = re_sid.read(ri);
    }
    void w(const u8& ri, const u8& data) {
        //std::cout << "SID w: " << (int)ri << " " << (int)data << "\n";
        re_sid.write(ri, data);
    }

private:
    reSID::SID re_sid;

    int buf_tick_cnt;
    int tick_cnt;

    static const u32 BUF_SZ = 0x200; // TODO: calculate from other params?

    i16 buf[BUF_SZ];
    i16* buf_ptr = buf;
    i16* buf_end = buf + BUF_SZ; // one past last

    Host::Audio_out audio_out;
};


} // namespace SID

#endif // SID_H_INCLUDED
