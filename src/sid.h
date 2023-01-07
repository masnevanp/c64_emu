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

    void reconfig(double frame_rate, bool pitch_shift) {
        if (pitch_shift) {
            re_sid.set_sampling_parameters(frame_rate * FRAME_CYCLE_COUNT, set.sampling, OUTPUT_FREQ);
            clock_speed_base = 1.0;
        } else {
            re_sid.set_sampling_parameters(CPU_FREQ_PAL, set.sampling, OUTPUT_FREQ);
            clock_speed_base = FRAME_RATE_PAL / frame_rate;
        }
    }

    /*
        Some crude rate control here... But seems to be good enough.
        (Not really tested on low-end machines though.)
        TODO: make buffer sizes & rate control params runtime configurable (if ever needed)
    */
    void output() {
        static constexpr int buffered_lo = AUDIO_BUFFER_SIZE * 2;
        static constexpr int buffered_hi = 3 * buffered_lo;

        tick();

        const int buffered = audio_out.put(buf, buf_ptr - buf);
        if (buffered <= buffered_lo) {
            clock_speed = 1.01 * clock_speed_base;
            //std::cout << "+";
        } else if (buffered >= buffered_hi) {
            const float reduction = 0.01 + (((buffered - buffered_hi) / 128.0) / 100.0);
            clock_speed = (1 - reduction) * clock_speed_base;
            //std::cout << "-";
        } else {
            clock_speed = clock_speed_base;
            //std::cout << ".";
        }

        buf_ptr = buf;
    }

    void r(const u8& ri, u8& data) { data = re_sid.read(ri); }
    void w(const u8& ri, const u8& data) {
        tick(); // tick with old state first
        re_sid.write(ri, data);
    }

    reSID_Wrapper(const u64& system_cycle_) : system_cycle(system_cycle_) {}

private:
    reSID::SID re_sid;

    // max. ever needed + some extra for the crude rate control
    static constexpr u32 BUF_SZ = (1.01 * ((AUDIO_OUTPUT_FREQ / FRAME_RATE_MIN) / FRAME_SYNC_POINTS)) + 8;

    i16 buf[BUF_SZ];
    i16* buf_ptr = buf;

    u64 last_tick_cycle = 0;
    const u64& system_cycle;

    float clock_speed_base = 1.0;
    float clock_speed = 1.0;

    Host::Audio_out audio_out;

    Settings set;

    std::vector<Menu::Knob> menu_items{
        {"RESID / MODEL",    set.model,    [&](){ re_sid.set_chip_model(set.model); }},
        {"RESID / SAMPLING", set.sampling, [&](){ re_sid.set_sampling_parameters(982800, set.sampling, OUTPUT_FREQ); }},
    };

    void tick() {
        int cycles = clock_speed * (system_cycle - last_tick_cycle);
        last_tick_cycle = system_cycle;

        if (cycles > 0) {
            // there is always enough space in the buffer (hence the '0xffff')
            buf_ptr += re_sid.clock(cycles, buf_ptr, 0xffff);
        }
    }
};


#endif // SID_H_INCLUDED
