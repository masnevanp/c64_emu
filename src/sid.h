#ifndef SID_H_INCLUDED
#define SID_H_INCLUDED

#include "resid/sid.h"
#include "host.h"
#include "menu.h"



// Using reSID by Dag Lem for core functionality
class reSID_Wrapper {
public:
    reSID_Wrapper(int min_frame_rate, int min_sync_points, const u64& system_cycle_) : system_cycle(system_cycle_) {
        // max. ever needed (with some extra for the crude clock speed control)
        const int max_buf_sz = (1.01 * ((AUDIO_OUTPUT_FREQ / min_frame_rate) / min_sync_points)) + 8;
        buf = buf_ptr = new i16[max_buf_sz];
    }

    class Core : public reSID::SID {
    public:
        bool set_clock_freq(double clock_freq) {
            return set_sampling_parameters(clock_freq, sampling, AUDIO_OUTPUT_FREQ);
        }
        bool set_sampling_method(reSID::sampling_method method) {
            return set_sampling_parameters(clock_frequency, method, AUDIO_OUTPUT_FREQ);
        }
    };

    Core core;

    void reset() { core.reset(); }
    void flush() {
        audio_out.flush();
        buf_ptr = buf;
        last_tick_cycle = system_cycle;
    }

    void reconfig(double frame_rate, bool pitch_shift);

    void reconfig(u16 audio_out_buf_sz_) { audio_out_buf_sz = audio_out.config(audio_out_buf_sz_); }

    void output();

    // TODO: tick also on read of one/some of the regs (rnd generator(s)...)
    void r(const u8& ri, u8& data) { data = core.read(ri); }
    void w(const u8& ri, const u8& data) {
        tick(); // tick with old state first
        core.write(ri, data);
    }

    Menu::Group settings_menu() { return Menu::Group("RESID / ", menu_items); }

private:
    i16* buf;
    i16* buf_ptr;

    u64 last_tick_cycle = 0;
    const u64& system_cycle;

    float clock_speed_base = 1.0;
    float clock_speed = 1.0;

    u16 audio_out_buf_sz;

    Host::Audio_out audio_out;

    struct Settings {
        using Model    = reSID::chip_model;
        using Sampling = reSID::sampling_method;

        Choice<Model> model{
            {Model::MOS6581, Model::MOS8580}, {"6581", "8580"},
        };
        Choice<Sampling> sampling{ // NOTE: 'fast' can get pitchy...
            {Sampling::SAMPLE_INTERPOLATE, Sampling::SAMPLE_RESAMPLE, Sampling::SAMPLE_FAST/*, Sampling::SAMPLE_RESAMPLE_FASTMEM*/},
            {"INTERPOLATE", "RESAMPLE", "FAST"/*, "RESAMPLE FASTMEM"*/},
        };
    };
    Settings set;

    std::vector<Menu::Knob> menu_items{
        {"MODEL",    set.model,    [&](){ core.set_chip_model(set.model); }},
        {"SAMPLING", set.sampling, [&](){ core.set_sampling_method(set.sampling); }},
    };

    void tick();
};


#endif // SID_H_INCLUDED
