#ifndef SID_H_INCLUDED
#define SID_H_INCLUDED

#include "residfp/SID.h"
#include "host.h"
#include "menu.h"



/*
    Using reSIDfp by Leandro Nini for core functionality:

        "reSIDfp is a fork of Dag Lem's reSID 0.16, a reverse engineered software emulation
        of the MOS6581/8580 SID (Sound Interface Device).

        The project was started by Antti S. Lankila in order to improve SID emulation
        with special focus on the 6581 filter.
        The codebase has been later on ported to java by Ken Händel within the jsidplay2 project
        and has seen further work by Antti Lankila.
        It was then ported back to c++ and integrated with improvements from reSID 1.0 by Leandro Nini."

                                    -- https://github.com/drfiemost/residfp/README
*/

class reSIDfp_Wrapper {
public:
    static const int OUTPUT_FREQ = 44100;

    struct Settings {
        using Model    = reSIDfp::ChipModel;
        using Sampling = reSIDfp::SamplingMethod;

        Choice<Model> model{
            {Model::MOS6581, Model::MOS8580}, {"MOS6581", "MOS8580"},
        };
        Choice<Sampling> sampling{
            {Sampling::DECIMATE, Sampling::RESAMPLE}, {"DECIMATE", "RESAMPLE"},
        };
    };
    Menu::Group settings_menu() { return Menu::Group("RESIDFP /", menu_items); }

    void reset() { core.reset(); }
    void flush() { audio_out.flush(); }

    void output() {
        tick();
        audio_out.put(buf, buf_ptr - buf);
        buf_ptr = buf;
    }

    void r(const u8& ri, u8& data) { data = core.read(ri); }
    void w(const u8& ri, const u8& data) {
        tick(); // tick with old state first
        core.write(ri, data);
    }

    reSIDfp_Wrapper(const u64& system_cycle_) : system_cycle(system_cycle_) {}

private:
    reSIDfp::SID core;

    // TODO: non-hard coding
    // output 6 times per frame  => ~147.0 samples
    static const u32 BUF_SZ = 152;

    i16 buf[BUF_SZ];
    i16* buf_ptr = buf;

    u64 last_tick_cycle = 0;
    const u64& system_cycle;

    Host::Audio_out audio_out;

    Settings set;

    std::vector<Menu::Knob> menu_items{
        {"RESIDFP / MODEL",    set.model,    [&](){ core.setChipModel(set.model); }},
        {"RESIDFP / SAMPLING", set.sampling, [&](){ core.setSamplingParameters(CPU_FREQ, set.sampling, OUTPUT_FREQ, 20000); }},
    };

    void tick() {
        int n = system_cycle - last_tick_cycle;
        if (n) {
            last_tick_cycle = system_cycle;
            // there is always enough space in the buffer (hence the '0xffff')
            buf_ptr += core.clock(n, buf_ptr);
        }
    }
};


#endif // SID_H_INCLUDED
