#ifndef C1541_H_INCLUDED
#define C1541_H_INCLUDED

#include <iostream>
#include <vector>
#include "common.h"
#include "utils.h"
#include "files.h"
#include "nmos6502/nmos6502_core.h"
#include "menu.h"
#include "dbg.h"


/* TODO:
    - VIA:
      - timer 2: ever needed?
    - ...
*/

namespace C1541 {

using CPU = NMOS6502::Core;

// CPU freq. ratio (C64 vs. C1541): 67/68 (darn close to 985248.45 / 1000000, 66/67 is also close)
static constexpr int extra_cycle_freq = 67;

static constexpr double motor_rpm = 300.0;

// zone == (clock_sel_b << 1) | clock_sel_a == (VIA.pb6 << 1) | VIA.pb5
constexpr double coder_freq(u8 zone) { return 1000000 * (16.0 / (16 - (zone & 0b11))); }

static constexpr double gcr_len(int non_enc_len) { return 5 * (non_enc_len / 4.0); }


struct Disk_format {
    class GCR_output {
    public:
        static constexpr u8 gcr_enc[16] = { // nybble GCR encoding
            0b01010, 0b01011, 0b10010, 0b10011, 0b01110, 0b01111, 0b10110, 0b10111,
            0b01001, 0b11001, 0b11010, 0b11011, 0b01101, 0b11101, 0b11110, 0b10101
        };

        GCR_output(u8* out_buf, u8 start_phase = 0)
            : start(out_buf), out(out_buf), phase(start_phase) {}

        void enc(const u8 byte) {
            const u8 gcr_nyb_1 = gcr_enc[byte >> 4];
            const u8 gcr_nyb_2 = gcr_enc[byte & 0b1111];

            switch (phase++) {
                case 0:
                    *out = gcr_nyb_1 << 3;
                    *out |= (gcr_nyb_2 >> 2);
                    ++out;
                    *out = gcr_nyb_2 << 6;
                    return;
                case 1:
                    *out |= (gcr_nyb_1 << 1);
                    *out |= (gcr_nyb_2 >> 4);
                    ++out;
                    *out = gcr_nyb_2 << 4;
                    return;
                case 2:
                    *out |= (gcr_nyb_1 >> 1);
                    ++out;
                    *out = (gcr_nyb_1 << 7);
                    *out |= (gcr_nyb_2 << 2);
                    return;
                case 3:
                    phase = 0;
                    *out |= (gcr_nyb_1 >> 3);
                    ++out;
                    *out = (gcr_nyb_1 << 5);
                    *out |= gcr_nyb_2;
                    ++out;
                    return;
            }
        }

        void raw(const u8 byte) {
            if (phase == 0) *out++ = byte;
            else {
                const auto shift = 2 * phase;
                *out |= (byte >> shift);
                ++out;
                *out = (byte << (8 - shift));
            }
        }

        const u8* pos() const { return out; }

        u32 len() const { return (phase == 0) ? out - start : (out - start) + 1; }

    private:
        const u8* start;
        u8* out;
        u8 phase;
    };

    static constexpr u8 header_block_id       = 0x08;
    static constexpr u8 data_block_id         = 0x07;
    static constexpr u8 header_block_off_byte = 0x0f;
    static constexpr u8 data_block_off_byte   = 0x00;
    static constexpr u8 gap_byte              = 0b01010101;
    static constexpr u8 sync_byte             = 0b11111111; // no such thing actually (SYNC is 10..n of '1')

    static constexpr int sync_mark_len        = 5; // no gcr for these
    static constexpr int header_data_len      = 8;
    static constexpr int header_gap_len       = 9; // no gcr for these
    static constexpr int data_block_data_len  = 260;
    static constexpr int header_block_len     = sync_mark_len + gcr_len(header_data_len) + header_gap_len;
    static constexpr int data_block_len       = sync_mark_len + gcr_len(data_block_data_len);
    static constexpr int sector_len           = header_block_len + data_block_len; // NOTE: sector gap excluded

    constexpr double total_bits_per_track(u8 track) { // theoretical max
        const auto zone = speed_zone(track);
        const auto bit_cell_len = 4; // in coder clocks
        const auto rps = motor_rpm / 60;
        return (coder_freq(zone) / bit_cell_len) / rps;
    }

    constexpr double used_bits_per_track(u8 track) {
        return (sector_count(track) * sector_len) * 8;
    }

    constexpr double unused_bits_per_track(u8 track) {
        return total_bits_per_track(track) - used_bits_per_track(track);
    }

    /*static constexpr std::pair<int, int> sector_gap_len(u8 track) {
        const int total_bytes = unused_bits_per_track(track) / 8;
        const int sector_gap = total_bytes / sector_count(track);
        const int left_over = total_bytes % sector_count(track);
        return {sector_gap, left_over};
    }*/

    static void output_sync_mark(GCR_output& output) {
        u8 len = 5;
        while (len--) output.raw(sync_byte);
    }

    static void output_gap(GCR_output& output, u16 len) {
        while (len--) output.raw(gap_byte);
    }

    static void output_header_block(GCR_output& output, u8 sec_n, u8 tr_n, u8 id2, u8 id1) {
        const u8 checksum = sec_n ^ tr_n ^ id2 ^ id1;

        output_sync_mark(output);
        output.enc(header_block_id);
        output.enc(checksum);
        output.enc(sec_n);
        output.enc(tr_n);
        output.enc(id2);
        output.enc(id1);
        output.enc(header_block_off_byte);
        output.enc(header_block_off_byte);
        output_gap(output, header_gap_len);
    }

    static void output_data_block(GCR_output& output, const u8* block) { // sans gap
        u8 checksum = 0x00;

        output_sync_mark(output);
        output.enc(data_block_id);
        for (int b = 0; b < 256; ++b) {
            output.enc(block[b]);
            checksum ^= block[b];
        }
        output.enc(checksum);
        output.enc(data_block_off_byte);
        output.enc(data_block_off_byte);
    }
};

using DF = Disk_format;


class Disk {
public:
    struct Track {
        Track(const std::size_t len = 0, const u8* data_r = nullptr, u8* data_w = nullptr)
          : r{len, data_r}, w{len, data_w} {}

        /*
            TODO: allow different lengths? (tricky to maintain head_pos when switching between r/w...?)
        */
        struct Read {
            std::size_t len = 0;
            const u8* data = nullptr;
        };
        struct Write {
            std::size_t len = 0;
            u8* data = nullptr;
        };

        Read r;
        Write w;
    };

    virtual Track track(u8 half_track_num) const = 0;

    static Track null_track() {
        static constexpr u8 data_r[2] = {DF::gap_byte, DF::gap_byte ^ 0b11111111};
        static u8 data_w[2] = {}; // written data is 'lost' here
        return Track{2, data_r, data_w};
    };

    virtual ~Disk() {}
};


/*
  TODO:
    - handle modified disks
*/
class G64_disk : public Disk {
public:
    G64_disk(std::vector<u8>&& data) : g64{std::move(data)} {}
    virtual ~G64_disk() {}
    /* TODO:
        - validate image (here?), just return null tracks if invalid
    */
    const Files::G64 g64;

    virtual Track track(u8 half_track_num) const {
        auto [len, data] = g64.track(half_track_num);
        return (len && data) // allows also saving (to same array of data).. at least for now...
                    ? Track{len, data, const_cast<u8*>(data)}
                    : null_track();
    }
};


class D64_disk : public Disk {
public:
    D64_disk(const Files::D64& d64) { generate_disk(d64); }
    virtual ~D64_disk() {}

    virtual Track track(u8 half_track_num) const {
        if (half_track_num & 0x1) return null_track(); // or just return the 'main' track?

        const u8 track_num = (half_track_num / 2) + 1;
        if (track_num < first_track || track_num > last_track) return null_track();

        return tracks[track_num - 1];
    }

private:
    void generate_disk(const Files::D64& src) {
        tracks.reserve(Files::D64::track_count);
        gcr_data.resize(300000); // TODO: actual sz

        DF::GCR_output gcr_out(gcr_data.data());

        auto write_sector = [&](const u8 tr_n, const u8 sec_n, const u8* data) {
            const u8 id1 = src.bam().disk_id[0];
            const u8 id2 = src.bam().disk_id[1];
            DF::output_header_block(gcr_out, sec_n, tr_n, id2, id1);
            DF::output_data_block(gcr_out, data);
        };

        auto write_track = [&](const u8 track) {
            auto track_start = gcr_out.pos();

            const u8 sector_cnt = sector_count(track);
            for (u8 sector = 0; sector < sector_cnt; ++sector) {
                write_sector(track, sector, src.block({ track, sector }));
                DF::output_gap(gcr_out, 8); // TODO: proper length (it varies by zone)
            }

            // a longer gap after the last sector (TODO: proper lengths)
            static constexpr int speed_zone_gap_len[4] = { 96, 150, 264, 90 };
            const int gap_len = speed_zone_gap_len[speed_zone(track)];
            DF::output_gap(gcr_out, gap_len);

            const std::size_t len = gcr_out.pos() - track_start;
            tracks.emplace_back(len, track_start, const_cast<u8*>(track_start)); // allows also saving (to same array of data)
        };

        auto write_disk = [&]() {
            for (u8 track = first_track; track <= last_track; ++track) {
                write_track(track);
            }
        };

        write_disk();

        std::cout << "GCR data size: " << (int)(gcr_out.len()) << std::endl;
    }

    std::vector<Track> tracks;
    std::vector<u8> gcr_data;
};


class Null_disk : public Disk {
public:
    virtual Track track(u8 half_track_num) const { UNUSED(half_track_num);
         return null_track();
    }
    virtual ~Null_disk() {}
};

static const Null_disk null_disk;


namespace VIA {

    enum R : u8 {
        rb,    ra,    ddrb,  ddra,  t1c_l, t1c_h, t1l_l, t1l_h,
        t2c_l, t2c_h, sr,    acr,   pcr,   ifr,   ier,   ra_nh,
    };

    enum ACR : u8 {
        pa_latch = 0b00000001, pb_latch = 0b00000010, sr_ctrl = 0b00011100,
        t2_ctrl  = 0b00100000, t1_ctrl  = 0b11000000,

        pa_latch_dis = 0b0 << 0,   pa_latch_en = 0b1 << 0,
        pb_latch_dis = 0b0 << 1,   pb_latch_en = 0b1 << 1,

        src_dis      = 0b000 << 2, src_in_t2   = 0b001 << 2,
        src_in_o2    = 0b010 << 2, src_in_ec   = 0b011 << 2,
        src_out_ft2  = 0b100 << 2, src_out_t2  = 0b101 << 2,
        src_out_o2   = 0b110 << 2, src_out_ec  = 0b111 << 2,

        t2_timed_int = 0b0 << 5,   t2_cnt_pb6  = 0b1 << 5,

        t1_timed_int     = 0b00 << 6, t1_cont_int     = 0b01 << 6,
        t1_timed_int_pb7 = 0b10 << 6, t1_cont_int_pb7 = 0b11 << 6,
    };

    enum PCR : u8 {
        ca1 = 0b00000001, ca2 = 0b00001110, cb1 = 0b00010000, cb2 = 0b11100000,

        ca1_neg     = 0b0 << 0,    ca1_pos            = 0b1 << 0,
        cb1_neg     = 0b0 << 4,    cb1_pos            = 0b1 << 4,

        ca2_in_neg  = 0b000 << 1,  ca2_ind_int_in_neg = 0b001 << 1,
        ca2_in_pos  = 0b010 << 1,  ca2_ind_int_in_pos = 0b011 << 1,
        ca2_hs_out  = 0b100 << 1,  ca2_pulse_out      = 0b101 << 1,
        ca2_low_out = 0b110 << 1,  ca2_high_out       = 0b111 << 1,

        cb2_in_neg  = 0b000 << 5,  cb2_ind_int_in_neg = 0b001 << 5,
        cb2_in_pos  = 0b010 << 5,  cb2_ind_int_in_pos = 0b011 << 5,
        cb2_hs_out  = 0b100 << 5,  cb2_pulse_out      = 0b101 << 5,
        cb2_low_out = 0b110 << 5,  cb2_high_out       = 0b111 << 5,
    };

    class IRQ {
    public:
        enum Src : u8 {
            ca2 = 0b00000001, ca1 = 0b00000010, sr = 0b00000100, cb2 = 0b00001000,
            cb1 = 0b00010000, t2  = 0b00100000, t1 = 0b01000000, any = 0b10000000,
            ca1_ca2 = 0b00000011, cb1_cb2 = 0b00011000, none = 0b00000000,
        };
        enum SC : u8 { // set/clr
            bit = 0b10000000, sc_set = 0b10000000, sc_clr = 0b00000000,
        };

        void reset() { ifr = ier = 0x00; }

        void set(Src s) { ifr |= s; check(); }
        void clr(Src s) { ifr &= ~s; check(); }

        void w_ifr(u8 data) {
            data &= 0b01111111; // keep ifr7 untouched
            ifr &= ~data;
            check();
        }
        u8 r_ifr() const { return ifr; }

        void w_ier(u8 data) {
            ier = (data & SC::bit) == SC::sc_set
                ? ((ier | data) & ~SC::bit) // set
                : (ier & ~data); // clr
            check();
        }
        u8 r_ier() const { return ier | SC::bit; }

    private:
        void check() {
            ifr = (ifr & ier)
                ? (ifr | Src::any)
                : (ifr & ~Src::any);
        }

        u8 ifr;
        u8 ier;
    };

} // namespace VIA


class IEC {
public:
    static constexpr u8 dev_num = 8; // 8..11

    IEC(IO::Port::PD_in& cia2_pa_in_) : cia2_pa_in(cia2_pa_in_) {}

    void reset() {
        irq.reset();

        r_orb = r_ora = r_ddrb = r_ddra = r_acr = r_pcr = 0x00;
        r_t1c = r_t1l = 0xffff;

        t1_irq = VIA::IRQ::Src::none;
        via_pb_in = 0b11111111;
    }

    void via_r(const u8& ri, u8& data) {
        //std::cout << "VIA r: " << (int)ri << std::endl;
        switch (ri) {
            case VIA::R::rb:
                irq.clr(VIA::IRQ::Src::cb1_cb2);
                data = (r_orb & r_ddrb) | (via_pb_in & ~r_ddrb);
                return;
            case VIA::R::ra:
                irq.clr(VIA::IRQ::Src::ca1_ca2);
                data = (r_ora & r_ddra) | ~r_ddra; // not connected
                return;
            case VIA::R::ddrb:  data = r_ddrb; return;
            case VIA::R::ddra:  data = r_ddra; return;
            case VIA::R::t1c_l:
                irq.clr(VIA::IRQ::Src::t1);
                data = r_t1c;
                return;
            case VIA::R::t1c_h: data = r_t1c >> 8; return;
            case VIA::R::t1l_l: data = r_t1l;      return;
            case VIA::R::t1l_h: data = r_t1l >> 8; return;
            case VIA::R::t2c_l:
                irq.clr(VIA::IRQ::Src::t2);
                return;
            case VIA::R::t2c_h: return;
            case VIA::R::sr:
                irq.clr(VIA::IRQ::Src::sr);
                data = r_sr;
                return;
            case VIA::R::acr:   data = r_acr;       return;
            case VIA::R::pcr:   data = r_pcr;       return;
            case VIA::R::ifr:   data = irq.r_ifr(); return;
            case VIA::R::ier:   data = irq.r_ier(); return;
            case VIA::R::ra_nh:
                data = (r_ora & r_ddra) | ~r_ddra; // not connected
                return;
        }
    }

    void via_w(const u8& ri, const u8& data) {
        //std::cout << "VIA w: " << (int)ri << " " << (int)data << std::endl;
        switch (ri) {
            case VIA::R::rb:
                irq.clr(VIA::IRQ::Src::cb1_cb2);
                r_orb = data;
                output_pb();
                return;
            case VIA::R::ra:
                irq.clr(VIA::IRQ::Src::ca1_ca2);
                r_ora = data;
                return;
            case VIA::R::ddrb:  r_ddrb = data; output_pb(); return;
            case VIA::R::ddra:  r_ddra = data; return;
            case VIA::R::t1c_l: r_t1l = (r_t1l & 0xff00) | data; return;
            case VIA::R::t1c_h:
                irq.clr(VIA::IRQ::Src::t1);
                r_t1l = (r_t1l & 0x00ff) | (data << 8);
                r_t1c = r_t1l;
                t1_irq = VIA::IRQ::Src::t1; // 'starts' t1
                return;
            case VIA::R::t1l_l:
                // irq.clr(IRQ::Src::t1); // NOTE: mixed info on this one
                r_t1l = (r_t1l & 0xff00) | data;
                return;
            case VIA::R::t1l_h:
                irq.clr(VIA::IRQ::Src::t1); // NOTE: mixed info on this one
                r_t1l = (r_t1l & 0x00ff) | (data << 8);
                return;
            case VIA::R::t2c_l: return;
            case VIA::R::t2c_h:
                irq.clr(VIA::IRQ::Src::t2);
                return;
            case VIA::R::sr:
                irq.clr(VIA::IRQ::Src::sr);
                r_sr = data;
                return;
            case VIA::R::acr:   r_acr = data;    return;
            case VIA::R::pcr:   r_pcr = data;    return;
            case VIA::R::ifr:   irq.w_ifr(data); return;
            case VIA::R::ier:   irq.w_ier(data); return;
            case VIA::R::ra_nh: r_ora = data;    return;
        }
    }

    void cia2_pa_output(u8 state) {
        cia2_pa_out = state;
        update_iec_lines();
    }

    void tick() { // TODO: extract T1 (+ T2 if ever needed) functionality
        if (--r_t1c == 0xffff) {
            r_t1c = r_t1l;
            irq.set(t1_irq);
            t1_irq = VIA::IRQ::Src(r_acr & VIA::ACR::t1_cont_int); // no more IRQs if one-shot
        }
    }

    VIA::IRQ irq;

private:
    enum { low = 0b0, high = 0b1 };
    using state = u8;

    void output_pb() {
        via_pb_out = (r_orb & r_ddrb) | ~r_ddrb;
        update_iec_lines();
    }

    void update_iec_lines() {
        /*
            - Commodore 64 Programmers Reference Guide, Chapter 8 - Schematics
            - Commodore 1541 Troubleshooting and Repair Guide, Fig. 7-30 (p. 170..171)
        */
        const state clk = invert(cia2_pa(4)) & cia2_pa(6) & invert(via_pb(3));

        const state atn_now = cia2_pa(3) & via_pb(7); // pa3 inverted twice --> taken as such
        if (atn != atn_now) {
            atn = atn_now;
            ca1_edge(atn);
        }

        const state ud3a_out = via_pb(4) ^ atn;
        const state data = invert(cia2_pa(5)) & cia2_pa(7) & invert(via_pb(1)) & invert(ud3a_out);

        const u8 cia2_pa7_pa6 = (data << 7) | (clk << 6);
        cia2_pa_in(0b11000000, cia2_pa7_pa6);

        const u8 via_pb720_in = (atn << 7) | (invert(clk) << 2) | (invert(data) << 0);
        const u8 via_pb65_in = (dev_num - 8) << 5;
        const u8 pb_in = via_pb720_in | via_pb65_in | 0b00011010;
        via_pb_in = via_pb_out & pb_in;
    }

    void ca1_edge(u8 edge) {
        if (edge == (r_pcr & VIA::PCR::ca1)) irq.set(VIA::IRQ::Src::ca1);
    }

    u8 r_orb;
    u8 r_ora;
    u8 r_ddrb;
    u8 r_ddra;
    u16 r_t1c;
    u16 r_t1l;
    u8 r_sr;
    u8 r_acr;
    u8 r_pcr;

    VIA::IRQ::Src t1_irq;

    u8 cia2_pa_out;
    u8 via_pb_out;
    u8 via_pb_in;

    state atn = high; // ok..?

    IO::Port::PD_in& cia2_pa_in;

    state cia2_pa(int pin) const { return pin_state(cia2_pa_out, pin); }
    state via_pb(int pin) const  { return pin_state(via_pb_out, pin); }

    static constexpr state pin_state(u8 pins, int pin) { return (pins >> pin) & 0b1; }
    static constexpr state invert(state s)             { return s ^ 0b1; }
};


class Disk_ctrl {
public:
    static constexpr int first_track =  0;
    static constexpr int last_track  = 83;

    enum PB : u8 {
        head_step = 0b00000011, motor    = 0b00000100, led  = 0b00001000,
        w_prot    = 0b00010000, bit_rate = 0b01100000, sync = 0b10000000,
    };

    struct Head_status {
        enum Mode : u8 { // PB4 (motor) and PCR CB2 (r/w) bits put together
            mode_r = 0b11100100, mode_w = 0b11000100, // anything else means 'off'
            uninit = 0b00000000,
        };

        Mode mode;
        u16 pos = 0; // rotation within current track --> index to data of current track
        u8 track_n = first_track;
        u8 next_byte_timer;

        u8   active()  const { return mode & PB::motor; }
        bool reading() const { return mode == Mode::mode_r; }
        bool writing() const { return mode == Mode::mode_w; }
    };

    struct Status {
        const Head_status& head;
        const u8& pb_out;
        const u8& pb_in;
        u8 led_on() const { return pb_out & PB::led; }
        u8 wp_off() const { return pb_in  & PB::w_prot; }
    };
    const Status status{head, via_pb_out, via_pb_in};

    Disk_ctrl(CPU& cpu_) : cpu(cpu_) { load_disk(&null_disk); }

    void reset() {
        irq.reset();

        r_orb = r_ora = r_ddrb = r_ddra = r_acr = r_pcr = 0x00;
        r_t1c = r_t1l = 0xffff;

        t1_irq = VIA::IRQ::Src::none;

        via_pb_in = via_pa_in = 0b11111111;
        via_pb_out = 0xff;

        head.mode = Head_status::Mode::uninit;
        change_track((dir_track - 1) * 2);
    }

    void via_r(const u8& ri, u8& data) {
        //std::cout << "VIA r: " << (int)ri << std::endl;
        switch (ri) {
            case VIA::R::rb:
                irq.clr(VIA::IRQ::Src::cb1_cb2);
                data = (r_orb & r_ddrb) | (via_pb_in & ~r_ddrb);
                return;
            case VIA::R::ra:
                irq.clr(VIA::IRQ::Src::ca1_ca2);
                data = via_pa_in;
                return;
            case VIA::R::ddrb:  data = r_ddrb; return;
            case VIA::R::ddra:  data = r_ddra; return;
            case VIA::R::t1c_l:
                irq.clr(VIA::IRQ::Src::t1);
                data = r_t1c;
                return;
            case VIA::R::t1c_h: data = r_t1c >> 8; return;
            case VIA::R::t1l_l: data = r_t1l;      return;
            case VIA::R::t1l_h: data = r_t1l >> 8; return;
            case VIA::R::t2c_l:
                irq.clr(VIA::IRQ::Src::t2);
                return;
            case VIA::R::t2c_h: return;
            case VIA::R::sr:
                irq.clr(VIA::IRQ::Src::sr);
                data = r_sr;
                return;
            case VIA::R::acr:   data = r_acr;       return;
            case VIA::R::pcr:   data = r_pcr;       return;
            case VIA::R::ifr:   data = irq.r_ifr(); return;
            case VIA::R::ier:   data = irq.r_ier(); return;
            case VIA::R::ra_nh: data = via_pa_in;  return;
        }
    }

    void via_w(const u8& ri, const u8& data) {
        //std::cout << "VIA w: " << (int)ri << " " << (int)data << std::endl;
        switch (ri) {
            case VIA::R::rb:
                irq.clr(VIA::IRQ::Src::cb1_cb2);
                r_orb = data;
                output_pb();
                return;
            case VIA::R::ra:
                irq.clr(VIA::IRQ::Src::ca1_ca2);
                r_ora = data;
                output_pa();
                return;
            case VIA::R::ddrb:  r_ddrb = data; output_pb(); return;
            case VIA::R::ddra:  r_ddra = data; output_pa(); return;
            case VIA::R::t1c_l: r_t1l = (r_t1l & 0xff00) | data; return;
            case VIA::R::t1c_h:
                irq.clr(VIA::IRQ::Src::t1);
                r_t1l = (r_t1l & 0x00ff) | (data << 8);
                r_t1c = r_t1l;
                t1_irq = VIA::IRQ::Src::t1; // 'starts' t1
                return;
            case VIA::R::t1l_l:
                // irq.clr(VIA::IRQ::Src::t1); // NOTE: mixed info on this one
                r_t1l = (r_t1l & 0xff00) | data;
                return;
            case VIA::R::t1l_h:
                irq.clr(VIA::IRQ::Src::t1); // NOTE: mixed info on this one
                r_t1l = (r_t1l & 0x00ff) | (data << 8);
                return;
            case VIA::R::t2c_l: return;
            case VIA::R::t2c_h:
                irq.clr(VIA::IRQ::Src::t2);
                return;
            case VIA::R::sr:
                irq.clr(VIA::IRQ::Src::sr);
                r_sr = data;
                return;
            case VIA::R::acr:   r_acr = data;    return;
            case VIA::R::pcr:   r_pcr = data; pcr_update(); return;
            case VIA::R::ifr:   irq.w_ifr(data); return;
            case VIA::R::ier:   irq.w_ier(data); return;
            case VIA::R::ra_nh: r_ora = data; output_pa(); return;
        }
    }

    void load_disk(const Disk* disk_) {
        disk = disk_;
        change_track(head.track_n);
    }

    void set_write_prot(bool wp_on) {
        via_pb_in = wp_on ? (via_pb_in & ~PB::w_prot) : (via_pb_in | PB::w_prot);
    }

    void tick() {
        if (--r_t1c == 0xffff) {
            r_t1c = r_t1l;
            irq.set(t1_irq);
            t1_irq = VIA::IRQ::Src(r_acr & VIA::ACR::t1_cont_int); // no more IRQs if one-shot
        }

        if (head.reading()) read();
        else if (head.writing()) write();
    }

    VIA::IRQ irq;

private:
    void output_pb() {
        const u8 via_pb_out_now = (r_orb & r_ddrb) | ~r_ddrb;

        head.mode = Head_status::Mode((via_pb_out_now & PB::motor) | (head.mode & ~PB::motor));

        if (via_pb_out_now & PB::motor) step_head(via_pb_out_now);

        via_pb_out = via_pb_out_now;
    }
    void output_pa() { via_pa_out = (r_ora & r_ddra) | ~r_ddra; }
    void pcr_update() {
        head.mode = Head_status::Mode((r_pcr & VIA::PCR::cb2) | (head.mode & ~VIA::PCR::cb2));
    }
    void ca1_edge(u8 edge) {
        if (edge == (r_pcr & VIA::PCR::ca1)) irq.set(VIA::IRQ::Src::ca1);
    }
    bool write_prot_off()     const { return via_pb_in & PB::w_prot; }
    bool byte_ready_enabled() const { return (r_pcr & VIA::PCR::ca2) != VIA::PCR::ca2_low_out; }
    u8   cycles_per_byte()    const { return 32 - ((via_pb_out & PB::bit_rate) >> 4); }
    void set_sync()                 { via_pb_in &= ~PB::sync; }
    void clr_sync()                 { via_pb_in |= PB::sync; }
    bool not_sync_set()       const { return via_pb_in & PB::sync; }
    void signal_byte_ready() {
        ca1_edge(0b0);
        cpu.set(NMOS6502::Flag::V); // NOTE: SO-detection delay (in the CPU) not happening..
    }

    void step_head(const u8 via_pb_out_now) {
        auto should_step = [&](const int step) -> bool {
            return (via_pb_out_now & PB::head_step) == ((via_pb_out + step) & 0b11);
        };

        if (should_step(+1)) {
            if (head.track_n < last_track) change_track(head.track_n + 1);
        } else if (should_step(-1)) {
            if (head.track_n > first_track) change_track(head.track_n - 1);
        }
    }

    void read() {
        // TODO: work on bit level, e.g handle SYNC that is not byte aligned
        //       (and not a multiple of 8 bits), i.e. 're-align' the actual data (if needed)
        if (--head.next_byte_timer == 0) {
            head.next_byte_timer = cycles_per_byte();

            const u8 next_byte = track.r.data[head.pos];
            head.pos = (head.pos + 1) % track.r.len;

            if (via_pa_in == DF::sync_byte) {
                if (next_byte == DF::sync_byte) set_sync();
                else clr_sync();
            }
            via_pa_in = next_byte;

            if (byte_ready_enabled() && not_sync_set()) signal_byte_ready();
        }
    }

    void write() {
        /*
          TODO:
            - handle modified disks: set a 'dirty' flag here & save (auto/ask) when ejected?
                + restore disk (i.e. discard changes)
                + wipe disk (or is 'insert blank' enough?)
            - changing between read/write:
                - head_pos needs to stay valid (it does now, since r/w data is the same array...)
        */
        if (--head.next_byte_timer == 0) {
            head.next_byte_timer = cycles_per_byte();
            if (write_prot_off()) track.w.data[head.pos] = via_pa_out;
            head.pos = (head.pos + 1) % track.w.len;
            if (byte_ready_enabled()) signal_byte_ready();
        }
    }

    void change_track(const u8 to_half_track_n) {
        head.track_n = to_half_track_n;
        track = disk->track(head.track_n);
        head.pos = 0; // TODO: keep the relative position (does it ever matter?)
    }

    u8 r_orb;
    u8 r_ora;
    u8 r_ddrb;
    u8 r_ddra;
    u16 r_t1c;
    u16 r_t1l;
    u8 r_sr;
    u8 r_acr;
    u8 r_pcr;

    VIA::IRQ::Src t1_irq;

    u8 via_pa_out;
    u8 via_pb_out;
    u8 via_pa_in;
    u8 via_pb_in;

    Head_status head;

    CPU& cpu;

    const Disk* disk;
    Disk::Track track;
};


class Disk_carousel {
/*
    TODO:
    - 'toss disk' (removes disk from carousel & deletes it (?))
*/
public:
    static constexpr int slot_count = 0x100;

    struct Slot {
        const Disk* disk = nullptr;
        std::string disk_name;
        bool write_prot;
    };

    Disk_carousel(Disk_ctrl& disk_ctrl_, u8& dos_wp_change_flag_)
      : disk_ctrl(disk_ctrl_), dos_wp_change_flag(dos_wp_change_flag_)
    {
        slots[0] = Slot{&null_disk, "no disk", false};
    }

    void reset() { load(); }

    void insert(int in_slot, const Disk* disk, std::string name) {
        if (in_slot == 0) {
            in_slot = find_free_slot();
            if (in_slot == 0) {
                std::cout << "Carousel full (TODO)" << std::endl;
                return;
            }
        } else if (slots[in_slot].disk) {
            delete slots[in_slot].disk; // TODO: save changes...
        }
        slots[in_slot] = Slot{disk, name, true};
        select(in_slot);
    }

    void rotate() {
        for (int r = 0; r <= slot_count; ++r) {
            selected_slot = (selected_slot + 1) % slot_count;
            if (selected().disk) break;
        }
        load();
    }

    void select(int slot) {
        selected_slot = (slot >= 0 && slot < slot_count && slots[slot].disk) ? slot : 0;
        load();
    }

    Slot& selected() { return slots[selected_slot]; }

    void toggle_wp() {
        selected().write_prot = !selected().write_prot;
        disk_ctrl.set_write_prot(selected().write_prot);
    }

private:
    std::vector<Slot> slots{slot_count};
    int selected_slot = 0;

    Disk_ctrl& disk_ctrl;
    u8& dos_wp_change_flag;

    void load() {
        disk_ctrl.load_disk(selected().disk);
        disk_ctrl.set_write_prot(selected().write_prot);

        std::cout << "Disk " << selected_slot << ": " << selected().disk_name << std::endl;

        // works...? or breaks some custom loader(s)?
        // (would need to fiddle with the PB WP bit then...)
        dos_wp_change_flag = 0x01;
    }

    int find_free_slot() const {
        for (int slot = 1; slot < slot_count; ++slot) {
            if (!slots[slot].disk) return slot;
        }
        return 0;
    }
};


class System {
public:
    static constexpr u16 dos_wp_change_flag_addr = 0x1c;

    System(IO::Port::PD_in& cia2_pa_in, const u8* rom_/*, bool& run_cfg_change_*/)
      : iec(cia2_pa_in), dc(cpu), rom(rom_), disk_carousel(dc, ram[dos_wp_change_flag_addr]) /*, run_cfg_change(run_cfg_change_)*/
    {
        //install_idle_trap();
    }

    Menu::Group menu() {
        auto group = Menu::Group("DISK /", menu_imm_actions);
        group.add(menu_actions);
        return group;
    }

    void reset() {
        cpu.reset_cold();
        iec.reset();
        dc.reset();
        disk_carousel.reset();
        irq_state = 0x00;
        //idle = false;
    }

    void tick();

    //bool idle;
    CPU cpu{cpu_trap};
    IEC iec;
    Disk_ctrl dc;
    u8 ram[0x0800];
    const u8* rom;

    Disk_carousel disk_carousel;

private:
    /*enum Trap_OPC : u8 { // Halting instuction are used as trap
        idle_trap = 0x52,
    };*/

    enum Mapping : u8 {
        rom_r,
        ram_w, ram_r,
        via1_w, via1_r, via2_w, via2_r,
        none,
    };

    static constexpr Mapping addr_map[64][2] = { // w|r mapping for each 1K block
        // mappings rotated to optimize the 'standard' address space usage
        { rom_r,  rom_r  }, { rom_r,  rom_r  }, { rom_r,  rom_r  }, { rom_r,  rom_r  }, //0xC000..
        { rom_r,  rom_r  }, { rom_r,  rom_r  }, { rom_r,  rom_r  }, { rom_r,  rom_r  }, //0xD000..
        { rom_r,  rom_r  }, { rom_r,  rom_r  }, { rom_r,  rom_r  }, { rom_r,  rom_r  }, //0xE000..
        { rom_r,  rom_r  }, { rom_r,  rom_r  }, { rom_r,  rom_r  }, { rom_r,  rom_r  }, //0xF000..
        { ram_w,  ram_r  }, { ram_w,  ram_r  }, { none,   none   }, { none,   none   }, //0x0000..
        { none,   none   }, { none,   none   }, { via1_w, via1_r }, { via2_w, via2_r }, //0x1000..
        // the rest are mirrors...
        { ram_w,  ram_r  }, { ram_w,  ram_r  }, { none,   none   }, { none,   none   }, //0x2000..
        { none,   none   }, { none,   none   }, { via1_w, via1_r }, { via2_w, via2_r }, //0x3000..
        { ram_w,  ram_r  }, { ram_w,  ram_r  }, { none,   none   }, { none,   none   }, //0x4000..
        { none,   none   }, { none,   none   }, { via1_w, via1_r }, { via2_w, via2_r }, //0x5000..
        { ram_w,  ram_r  }, { ram_w,  ram_r  }, { none,   none   }, { none,   none   }, //0x6000..
        { none,   none   }, { none,   none   }, { via1_w, via1_r }, { via2_w, via2_r }, //0x7000..
        { rom_r,  rom_r  }, { rom_r,  rom_r  }, { rom_r,  rom_r  }, { rom_r,  rom_r  }, //0x8000..
        { rom_r,  rom_r  }, { rom_r,  rom_r  }, { rom_r,  rom_r  }, { rom_r,  rom_r  }, //0x9000..
        { rom_r,  rom_r  }, { rom_r,  rom_r  }, { rom_r,  rom_r  }, { rom_r,  rom_r  }, //0xA000..
        { rom_r,  rom_r  }, { rom_r,  rom_r  }, { rom_r,  rom_r  }, { rom_r,  rom_r  }, //0xB000..
    };

    void address_space_op(const u16& addr, u8& data, const u8& rw) {
        switch (addr_map[u16(addr + 0x4000) >> 10][rw]) {
            case rom_r:  data = rom[addr & 0x3fff];   return;
            case ram_w:  ram[addr & 0x07ff] = data;   return;
            case ram_r:  data = ram[addr & 0x07ff];   return;
            case via1_w: iec.via_w(addr & 0xf, data); return;
            case via1_r: iec.via_r(addr & 0xf, data); return;
            case via2_w: dc.via_w(addr & 0xf, data);  return;
            case via2_r: dc.via_r(addr & 0xf, data);  return;
            case none: return;
        }
    }

    u8 irq_state;
    void check_irq() {
        const u8 irq_state_now = (iec.irq.r_ifr() | dc.irq.r_ifr()) & VIA::IRQ::Src::any;
        if (irq_state != irq_state_now) {
            irq_state = irq_state_now;
            cpu.set_irq(irq_state);
        }
    }

    /*NMOS6502::Sig cpu_trap {
        [this]() {
            cpu.pc = 0xebff; // start of idle loop
            ++cpu.mcp; // bump to recover from halt
            if ((!dc.status.head.active()) && (!(ram[0x26c] | ram[0x7c]))) {
                run_cfg_change = idle = true;
            }
        }
    };*/

    void insert_blank() {
        // TODO: generate a truly blank disk
        const std::vector<u8> d64_data(std::size_t{Files::D64::size});
        Disk* blank_disk = new C1541::D64_disk(Files::D64{d64_data}); // not truly blank...
        disk_carousel.insert(0, blank_disk, "blank"); // TODO: handle 0 free slots case
    }

    NMOS6502::Sig cpu_trap {
        [this]() {
            std::cout << "\n****** C1541 CPU halted! ******" << std::endl;
            Dbg::print_status(cpu, ram);
        }
    };

    std::vector<Menu::Kludge> menu_imm_actions{
        {"DISK / TOGGLE WRITE PROTECTION !", [&](){ disk_carousel.toggle_wp(); return nullptr; }},
        {"DISK / EJECT !",                   [&](){ disk_carousel.select(0); return nullptr; }},
    };
    std::vector<Menu::Action> menu_actions{
        {"DISK / INSERT BLANK ?",            [&](){ insert_blank(); }},
        {"DISK / RESET DRIVE ?",             [&](){ reset(); }},
    };

    /*bool& run_cfg_change;
    void install_idle_trap();*/
};


} // namespace C1541

#endif // C1541_H_INCLUDED