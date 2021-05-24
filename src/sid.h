#ifndef SID_H_INCLUDED
#define SID_H_INCLUDED

#include "resid/sid.h"
#include "host.h"
#include "menu.h"



// Using reSID by Dag Lem for core functionality
class reSID_Wrapper {
public:
    static const int OUTPUT_FREQ = 44100;

    struct Settings {
        using Model    = reSID::chip_model;
        using Sampling = reSID::sampling_method;

        Choice<Model> model{
            {Model::MOS6581, Model::MOS8580}, {"MOS6581", "MOS8580"},
        };
        Choice<Sampling> sampling{ // NOTE: 'fast' can get pitchy...
            {Sampling::SAMPLE_INTERPOLATE, Sampling::SAMPLE_RESAMPLE,
                Sampling::SAMPLE_RESAMPLE_FASTMEM, Sampling::SAMPLE_FAST},
            {"INTERPOLATE", "RESAMPLE", "RESAMPLE FASTMEM", "FAST"},
        };
    };
    Menu::Group settings_menu() { return Menu::Group("RESID /", menu_items); }

    void reset() { re_sid.reset(); }
    void flush() { audio_out.flush(); }

    void output() {
        tick();
        audio_out.put(buf, buf_ptr - buf);
        buf_ptr = buf;
    }

    void r(const u8& ri, u8& data) { data = re_sid.read(ri); }
    void w(const u8& ri, const u8& data) {
        tick(); // tick with old state first
        re_sid.write(ri, data);
    }

    reSID_Wrapper(const u64& cycle_) : cycle(cycle_) {}

private:
    reSID::SID re_sid;

    // TODO: non-hard coding
    // output 6 times per frame  => ~147.0 samples
    static const u32 BUF_SZ = 152;

    i16 buf[BUF_SZ];
    i16* buf_ptr = buf;

    u64 last_tick_cycle = 0;
    const u64& cycle;

    Host::Audio_out audio_out;

    Settings set;

    std::vector<Menu::Knob> menu_items{
        {"RESID / MODEL",    set.model,    [&](){ re_sid.set_chip_model(set.model); }},
        {"RESID / SAMPLING", set.sampling, [&](){ re_sid.set_sampling_parameters(CPU_FREQ, set.sampling, OUTPUT_FREQ); }},
    };

    void tick() {
        int n = cycle - last_tick_cycle;
        if (n) {
            last_tick_cycle = cycle;
            // there is always enough space in the buffer (hence the '0xffff')
            buf_ptr += re_sid.clock(n, buf_ptr, 0xffff);
        }
    }
};


#endif // SID_H_INCLUDED
