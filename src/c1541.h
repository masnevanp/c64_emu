#ifndef C1541_H_INCLUDED
#define C1541_H_INCLUDED

#include <vector>
#include "common.h"
#include "state.h"
#include "utils.h"
#include "files.h"
#include "nmos6502/nmos6502_core.h"
#include "menu.h"
#include "dbg.h"


/* TODO:
    - VIA:
      - timer 2: ever needed?
    - howto handle modified disks? (should not be a hassle for the user...)
        - introduce a special disk image format (i.e. a Disk_image) for storing
          the GCR-data as handled by Disk_ctrl...? But why... not go with g64?
*/

namespace C1541 {

using CPU = NMOS6502::Core;

// CPU freq. ratio (C64 vs. C1541): 67/68 (darn close to 985248.45 / 1000000, 66/67 is also close)
static constexpr int extra_cycle_freq = 67;

static constexpr double motor_rpm = 300.0;

// zone == (clock_sel_b << 1) | clock_sel_a == (VIA.pb6 << 1) | VIA.pb5
constexpr double coder_clock_freq(u8 zone) { return 1000000 * (16.0 / (16 - (zone & 0b11))); }

static constexpr double gcr_len(int non_enc_len) { return 5 * (non_enc_len / 4.0); }


struct Disk_format {
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

    class GCR_output {
    public:
        static constexpr u8 gcr_enc[16] = { // nybble GCR encoding
            0b01010, 0b01011, 0b10010, 0b10011, 0b01110, 0b01111, 0b10110, 0b10111,
            0b01001, 0b11001, 0b11010, 0b11011, 0b01101, 0b11101, 0b11110, 0b10101
        };

        GCR_output(u8* out_buf, u8 start_phase = 0)
            : start(out_buf), out(out_buf), phase(start_phase) {}

        void enc(const u8 byte);
        void raw(const u8 byte);

        const u8* pos() const { return out; }

        u32 len() const { return (phase == 0) ? out - start : (out - start) + 1; }

    private:
        const u8* start;
        u8* out;
        u8 phase;
    };

    static constexpr double total_bits_per_track(u8 track) { // an ideal value for a 'standard' disk
        const auto bit_cell_len = 4; // in coder clocks
        const auto rotations_per_sec = motor_rpm / 60;

        const auto coder_freq = coder_clock_freq(zone(track));
        const auto bits_per_sec = coder_freq / bit_cell_len;
        const auto bits_per_rotation = bits_per_sec / rotations_per_sec;

        return bits_per_rotation;
    }

    static constexpr double used_bits_per_track(u8 track) {
        return (sector_count(track) * sector_len) * 8;
    }

    static constexpr double unused_bits_per_track(u8 track) {
        return total_bits_per_track(track) - used_bits_per_track(track);
    }

    static constexpr std::pair<int, int> sector_gap_len(u8 track) {
        const int unused_bits = int(unused_bits_per_track(track));
        const int total_gap_bits = ((unused_bits % 8) == 0)
            ? unused_bits
            : unused_bits + (8 - (unused_bits % 8));
        const int total_gap_bytes = total_gap_bits / 8;
        const int sector_gap_bytes = total_gap_bytes / sector_count(track);
        const int leftover_bytes = total_gap_bytes % sector_count(track);
        return {sector_gap_bytes, leftover_bytes};
    }

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
        output_sync_mark(output);

        output.enc(data_block_id);

        u8 checksum = 0x00;
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


class Disk_image {
public:
    struct Track {
        const std::size_t len = 0;
        const u8* data = nullptr;
    };

    virtual Track track(u8 half_track_num) const = 0;

    static Track null_track() {
        static constexpr u8 data[2] = {DF::gap_byte, DF::gap_byte ^ 0b11111111};
        return Track{2, data};
    };

    virtual ~Disk_image() {}
};


class G64 : public Disk_image {
public:
    G64(std::vector<u8>&& data) : g64{std::move(data)} {}
    virtual ~G64() {}
    /* TODO:
        - validate image (here?), just return null tracks if invalid
    */
    const Files::G64 g64;

    virtual Track track(u8 half_track_num) const {
        auto [len, data] = g64.track(half_track_num);
        return (len && data) ? Track{len, data} : null_track();
    }
};


class D64 : public Disk_image {
public:
    D64(const Files::D64& d64) { generate_disk(d64); }
    virtual ~D64() {}

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
            const auto track_start = gcr_out.pos();

            const auto [sec_gap_len, leftover_gap_bytes] = DF::sector_gap_len(track);
    
            const u8 sector_cnt = sector_count(track);
            for (u8 sector = 0; sector < sector_cnt; ++sector) {
                write_sector(track, sector, src.block({ track, sector }));
                DF::output_gap(gcr_out, sec_gap_len);
            }

            // append the leftovers...
            DF::output_gap(gcr_out, leftover_gap_bytes);

            const std::size_t len = gcr_out.pos() - track_start;
            tracks.emplace_back(Track{len, track_start});
        };

        auto write_disk = [&]() {
            for (u8 track = first_track; track <= last_track; ++track) {
                write_track(track);
            }
        };

        write_disk();

        Log::info("GCR data size: %d", (int)(gcr_out.len()));
    }

    std::vector<Track> tracks;
    std::vector<u8> gcr_data;
};


class Null_disk : public Disk_image {
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
        using State = State::C1541::IRQ;

        enum Src : u8 {
            ca2 = 0b00000001, ca1 = 0b00000010, sr = 0b00000100, cb2 = 0b00001000,
            cb1 = 0b00010000, t2  = 0b00100000, t1 = 0b01000000, any = 0b10000000,
            ca1_ca2 = 0b00000011, cb1_cb2 = 0b00011000, none = 0b00000000,
        };
        enum SC : u8 { // set/clr
            bit = 0b10000000, sc_set = 0b10000000, sc_clr = 0b00000000,
        };

        IRQ(State& s_) : s(s_) {}

        void reset() { s.ifr = s.ier = 0x00; }

        void set(Src src) { s.ifr |= src; check(); }
        void clr(Src src) { s.ifr &= ~src; check(); }

        void w_ifr(u8 data) {
            data &= 0b01111111; // keep ifr7 untouched
            s.ifr &= ~data;
            check();
        }
        u8 r_ifr() const { return s.ifr; }

        void w_ier(u8 data) {
            s.ier = (data & SC::bit) == SC::sc_set
                ? ((s.ier | data) & ~SC::bit) // set
                : (s.ier & ~data); // clr
            check();
        }
        u8 r_ier() const { return s.ier | SC::bit; }

    private:
        void check() {
            s.ifr = (s.ifr & s.ier)
                ? (s.ifr | Src::any)
                : (s.ifr & ~Src::any);
        }

        State& s;
    };

} // namespace VIA


class IEC {
private:
    using State = State::C1541::IEC;

    State& s;

public:

    static constexpr u8 dev_num = 8; // 8..11

    IEC(State& s_, IO::Port::PD_in& cia2_pa_in_) : s(s_), irq(s.irq), cia2_pa_in(cia2_pa_in_) {}

    void reset();

    void via_r(const u8& ri, u8& data) {
        switch (ri) {
            case VIA::R::rb:
                irq.clr(VIA::IRQ::Src::cb1_cb2);
                data = (s.r_orb & s.r_ddrb) | (s.via_pb_in & ~s.r_ddrb);
                return;
            case VIA::R::ra:
                irq.clr(VIA::IRQ::Src::ca1_ca2);
                data = (s.r_ora & s.r_ddra) | ~s.r_ddra; // not connected
                return;
            case VIA::R::ddrb: data = s.r_ddrb; return;
            case VIA::R::ddra: data = s.r_ddra; return;
            case VIA::R::t1c_l:
                irq.clr(VIA::IRQ::Src::t1);
                data = s.r_t1c;
                return;
            case VIA::R::t1c_h: data = s.r_t1c >> 8; return;
            case VIA::R::t1l_l: data = s.r_t1l;      return;
            case VIA::R::t1l_h: data = s.r_t1l >> 8; return;
            case VIA::R::t2c_l:
                irq.clr(VIA::IRQ::Src::t2);
                return;
            case VIA::R::t2c_h: return;
            case VIA::R::sr:
                irq.clr(VIA::IRQ::Src::sr);
                data = s.r_sr;
                return;
            case VIA::R::acr: data = s.r_acr;     return;
            case VIA::R::pcr: data = s.r_pcr;     return;
            case VIA::R::ifr: data = irq.r_ifr(); return;
            case VIA::R::ier: data = irq.r_ier(); return;
            case VIA::R::ra_nh:
                data = (s.r_ora & s.r_ddra) | ~s.r_ddra; // not connected
                return;
        }
    }

    void via_w(const u8& ri, const u8& data) {
        switch (ri) {
            case VIA::R::rb:
                irq.clr(VIA::IRQ::Src::cb1_cb2);
                s.r_orb = data;
                output_pb();
                return;
            case VIA::R::ra:
                irq.clr(VIA::IRQ::Src::ca1_ca2);
                s.r_ora = data;
                return;
            case VIA::R::ddrb:  s.r_ddrb = data; output_pb(); return;
            case VIA::R::ddra:  s.r_ddra = data; return;
            case VIA::R::t1c_l: s.r_t1l = (s.r_t1l & 0xff00) | data; return;
            case VIA::R::t1c_h:
                irq.clr(VIA::IRQ::Src::t1);
                s.r_t1l = (s.r_t1l & 0x00ff) | (data << 8);
                s.r_t1c = s.r_t1l;
                s.t1_irq = VIA::IRQ::Src::t1; // 'starts' t1
                return;
            case VIA::R::t1l_l:
                // irq.clr(IRQ::Src::t1); // NOTE: mixed info on this one
                s.r_t1l = (s.r_t1l & 0xff00) | data;
                return;
            case VIA::R::t1l_h:
                irq.clr(VIA::IRQ::Src::t1); // NOTE: mixed info on this one
                s.r_t1l = (s.r_t1l & 0x00ff) | (data << 8);
                return;
            case VIA::R::t2c_l: return;
            case VIA::R::t2c_h:
                irq.clr(VIA::IRQ::Src::t2);
                return;
            case VIA::R::sr:
                irq.clr(VIA::IRQ::Src::sr);
                s.r_sr = data;
                return;
            case VIA::R::acr:   s.r_acr = data;  return;
            case VIA::R::pcr:   s.r_pcr = data;  return;
            case VIA::R::ifr:   irq.w_ifr(data); return;
            case VIA::R::ier:   irq.w_ier(data); return;
            case VIA::R::ra_nh: s.r_ora = data;  return;
        }
    }

    void cia2_pa_output(u8 state) {
        s.cia2_pa_out = state;
        update_iec_lines();
    }

    void tick() { // TODO: extract T1 (+ T2 if ever needed) functionality
        if (--s.r_t1c == 0xffff) {
            s.r_t1c = s.r_t1l;
            irq.set(VIA::IRQ::Src(s.t1_irq));
            s.t1_irq = VIA::IRQ::Src(s.r_acr & VIA::ACR::t1_cont_int); // no more IRQs if one-shot
        }
    }

    VIA::IRQ irq;

private:
    enum { low = 0b0, high = 0b1 };

    using pin_state = u8;

    void output_pb() {
        s.via_pb_out = (s.r_orb & s.r_ddrb) | ~s.r_ddrb;
        update_iec_lines();
    }

    void update_iec_lines();

    void ca1_edge(u8 edge) {
        if (edge == (s.r_pcr & VIA::PCR::ca1)) irq.set(VIA::IRQ::Src::ca1);
    }

    IO::Port::PD_in& cia2_pa_in;

    pin_state cia2_pa(int pin) const { return read_pin(s.cia2_pa_out, pin); }
    pin_state via_pb(int pin) const  { return read_pin(s.via_pb_out, pin); }

    static constexpr pin_state read_pin(u8 pins, int pin) { return (pins >> pin) & 0b1; }
    static constexpr pin_state invert(pin_state s)        { return s ^ 0b1; }
};


class Disk_ctrl {
private:
    using State = State::C1541::Disk_ctrl;

    State& s;

public:
    static constexpr int first_track   = 0;
    static constexpr int last_track    = State::track_count - 1;

    enum PB : u8 {
        head_step = 0b00000011, motor    = 0b00000100, led  = 0b00001000,
        w_prot    = 0b00010000, bit_rate = 0b01100000, sync = 0b10000000,
    };

    struct Status {
        struct Head {
            State::Head& s;

            u8   active()  const { return s.mode & PB::motor; }
            bool reading() const { return s.mode == State::Head::Mode::mode_r; }
            bool writing() const { return s.mode == State::Head::Mode::mode_w; }
        };

        const Head head;

        const u8& pb_out;
        const u8& pb_in;

        bool led_on() const        { return pb_out & PB::led; }
        bool write_prot_on() const { return !(pb_in & PB::w_prot); }
    };
    const Status status{s.head, s.via_pb_out, s.via_pb_in};

    Disk_ctrl(State& s_, CPU& cpu_) : s(s_), irq(s.irq), cpu(cpu_) { load_disk(&null_disk); }

    void reset();

    void via_r(const u8& ri, u8& data) {
        switch (ri) {
            case VIA::R::rb:
                irq.clr(VIA::IRQ::Src::cb1_cb2);
                data = (s.r_orb & s.r_ddrb) | (s.via_pb_in & ~s.r_ddrb);
                return;
            case VIA::R::ra:
                irq.clr(VIA::IRQ::Src::ca1_ca2);
                data = s.via_pa_in;
                return;
            case VIA::R::ddrb: data = s.r_ddrb; return;
            case VIA::R::ddra: data = s.r_ddra; return;
            case VIA::R::t1c_l:
                irq.clr(VIA::IRQ::Src::t1);
                data = s.r_t1c;
                return;
            case VIA::R::t1c_h: data = s.r_t1c >> 8; return;
            case VIA::R::t1l_l: data = s.r_t1l;      return;
            case VIA::R::t1l_h: data = s.r_t1l >> 8; return;
            case VIA::R::t2c_l:
                irq.clr(VIA::IRQ::Src::t2);
                return;
            case VIA::R::t2c_h: return;
            case VIA::R::sr:
                irq.clr(VIA::IRQ::Src::sr);
                data = s.r_sr;
                return;
            case VIA::R::acr:   data = s.r_acr;     return;
            case VIA::R::pcr:   data = s.r_pcr;     return;
            case VIA::R::ifr:   data = irq.r_ifr(); return;
            case VIA::R::ier:   data = irq.r_ier(); return;
            case VIA::R::ra_nh: data = s.via_pa_in; return;
        }
    }

    void via_w(const u8& ri, const u8& data) {
        switch (ri) {
            case VIA::R::rb:
                irq.clr(VIA::IRQ::Src::cb1_cb2);
                s.r_orb = data;
                output_pb();
                return;
            case VIA::R::ra:
                irq.clr(VIA::IRQ::Src::ca1_ca2);
                s.r_ora = data;
                output_pa();
                return;
            case VIA::R::ddrb:  s.r_ddrb = data; output_pb(); return;
            case VIA::R::ddra:  s.r_ddra = data; output_pa(); return;
            case VIA::R::t1c_l: s.r_t1l = (s.r_t1l & 0xff00) | data; return;
            case VIA::R::t1c_h:
                irq.clr(VIA::IRQ::Src::t1);
                s.r_t1l = (s.r_t1l & 0x00ff) | (data << 8);
                s.r_t1c = s.r_t1l;
                s.t1_irq = VIA::IRQ::Src::t1; // 'starts' t1
                return;
            case VIA::R::t1l_l:
                // irq.clr(VIA::IRQ::Src::t1); // NOTE: mixed info on this one
                s.r_t1l = (s.r_t1l & 0xff00) | data;
                return;
            case VIA::R::t1l_h:
                irq.clr(VIA::IRQ::Src::t1); // NOTE: mixed info on this one
                s.r_t1l = (s.r_t1l & 0x00ff) | (data << 8);
                return;
            case VIA::R::t2c_l: return;
            case VIA::R::t2c_h:
                irq.clr(VIA::IRQ::Src::t2);
                return;
            case VIA::R::sr:
                irq.clr(VIA::IRQ::Src::sr);
                s.r_sr = data;
                return;
            case VIA::R::acr:   s.r_acr = data;    return;
            case VIA::R::pcr:   s.r_pcr = data; pcr_update(); return;
            case VIA::R::ifr:   irq.w_ifr(data); return;
            case VIA::R::ier:   irq.w_ier(data); return;
            case VIA::R::ra_nh: s.r_ora = data; output_pa(); return;
        }
    }

    void load_disk(const Disk_image* disk);

    void set_write_prot(bool wp_on) {
        s.via_pb_in = wp_on ? (s.via_pb_in & ~PB::w_prot) : (s.via_pb_in | PB::w_prot);
    }

    void tick() {
        if (--s.r_t1c == 0xffff) {
            s.r_t1c = s.r_t1l;
            irq.set(VIA::IRQ::Src(s.t1_irq));
            s.t1_irq = VIA::IRQ::Src(s.r_acr & VIA::ACR::t1_cont_int); // no more IRQs if one-shot
        }

        if (status.head.reading()) read();
        else if (status.head.writing()) write();
    }

    VIA::IRQ irq;

private:
    void output_pb();
    void output_pa() { s.via_pa_out = (s.r_ora & s.r_ddra) | ~s.r_ddra; }
    void pcr_update() {
        s.head.mode = State::Head::Mode((s.r_pcr & VIA::PCR::cb2) | (s.head.mode & ~VIA::PCR::cb2));
    }
    void ca1_edge(u8 edge) {
        if (edge == (s.r_pcr & VIA::PCR::ca1)) irq.set(VIA::IRQ::Src::ca1);
    }
    bool write_prot_off()     const { return s.via_pb_in & PB::w_prot; }
    bool byte_ready_enabled() const { return (s.r_pcr & VIA::PCR::ca2) != VIA::PCR::ca2_low_out; }
    u8   cycles_per_byte()    const { return 32 - ((s.via_pb_out & PB::bit_rate) >> 4); }
    void set_sync()                 { s.via_pb_in &= ~PB::sync; }
    void clr_sync()                 { s.via_pb_in |= PB::sync; }
    bool not_sync_set()       const { return s.via_pb_in & PB::sync; }
    void signal_byte_ready() {
        ca1_edge(0b0);
        cpu.s.set(NMOS6502::Flag::V); // TODO: SO-detection delay in the CPU (how many cycles?)
    }

    void step_head(const u8 via_pb_out_now);

    void rotate_disk() { s.head.rotation = (s.head.rotation + 1) % s.track_len[s.head.track_num]; }

    void read() {
        // TODO: work on bit level, e.g handle SYNC that is not byte aligned
        //       (and not a multiple of 8 bits), i.e. 're-align' the actual data (if needed)
        if (--s.head.next_byte_timer == 0) {
            s.head.next_byte_timer = cycles_per_byte();

            const u8 next_byte = s.track_data[s.head.track_num][s.head.rotation];

            rotate_disk();

            if (s.via_pa_in == DF::sync_byte) {
                if (next_byte == DF::sync_byte) set_sync();
                else clr_sync();
            }
            s.via_pa_in = next_byte;

            if (byte_ready_enabled() && not_sync_set()) signal_byte_ready();
        }
    }

    void write() {
        /*
          TODO:
            - handle modified disks: set a 'dirty' flag here & save (auto/ask) when ejected?
                + restore disk (i.e. discard changes)
                + wipe disk (or is 'insert blank' enough?)
        */
        if (--s.head.next_byte_timer == 0) {
            s.head.next_byte_timer = cycles_per_byte();

            if (write_prot_off()) s.track_data[s.head.track_num][s.head.rotation] = s.via_pa_out;

            rotate_disk();

            if (byte_ready_enabled()) signal_byte_ready();
        }
    }

    void change_track(const u8 to_track_n) {
        const auto old_rotation = double(s.head.rotation) / s.track_len[s.head.track_num];
        s.head.track_num = to_track_n;
        s.head.rotation = old_rotation * s.track_len[s.head.track_num];
    }

    CPU& cpu;
};


class Disk_carousel {
/*
    TODO:
    - move to 'System' (and add on-screen info... pop-ups..?)
    - 'toss disk' (removes disk from carousel & deletes it (?))
    - make part of state-snap (make it optional, and use a seperate file)?
*/
public:
    static constexpr int slot_count = 0x100;

    struct Slot {
        const Disk_image* disk = nullptr;
        std::string disk_name;
        bool write_prot;
    };

    Disk_carousel(Disk_ctrl& disk_ctrl_, u8& dos_wp_change_flag_)
      : disk_ctrl(disk_ctrl_), dos_wp_change_flag(dos_wp_change_flag_)
    {
        slots[0] = Slot{&null_disk, "none", false};
    }

    void insert(int in_slot, const Disk_image* disk, const std::string& name);

    void rotate();

    void select(int slot) {
        selected_slot = (slot >= 0 && slot < slot_count && slots[slot].disk) ? slot : 0;
        load();
    }

    Slot& selected() { return slots[selected_slot]; }

    void toggle_wp() {
        const auto new_wp_state = !disk_ctrl.status.write_prot_on();
        disk_ctrl.set_write_prot(new_wp_state);
        selected().write_prot = new_wp_state;
    }

private:
    std::vector<Slot> slots{slot_count};
    int selected_slot = 0;

    Disk_ctrl& disk_ctrl;
    u8& dos_wp_change_flag;

    void load();

    int find_free_slot() const {
        for (int slot = 1; slot < slot_count; ++slot) {
            if (!slots[slot].disk) return slot;
        }
        return 0;
    }
};


class System {
private:
    using State = State::C1541;

    State::System& s;

public:
    static constexpr u16 dos_wp_change_flag_addr = 0x1c;

    System(State& s_, IO::Port::PD_in& cia2_pa_in, const u8* rom_/*, bool& run_cfg_change_*/)
      : s(s_.system),
        cpu(s.cpu, cpu_trap), iec(s_.iec, cia2_pa_in), dc(s_.disk_ctrl, cpu),
        rom(rom_),
        disk_carousel(dc, s.ram[dos_wp_change_flag_addr]) /*, run_cfg_change(run_cfg_change_)*/
    {
        //install_idle_trap();
    }

    Menu::Group menu() { return {"DISK / ", menu_imm_actions, menu_actions}; }

    void reset();
    void tick();

    //bool idle;
    CPU cpu;
    IEC iec;
    Disk_ctrl dc;

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
            case ram_w:  s.ram[addr & 0x07ff] = data; return;
            case ram_r:  data = s.ram[addr & 0x07ff]; return;
            case via1_w: iec.via_w(addr & 0xf, data); return;
            case via1_r: iec.via_r(addr & 0xf, data); return;
            case via2_w: dc.via_w(addr & 0xf, data);  return;
            case via2_r: dc.via_r(addr & 0xf, data);  return;
            case none: return;
        }
    }

    void check_irq() {
        const u8 irq_state_now = (iec.irq.r_ifr() | dc.irq.r_ifr()) & VIA::IRQ::Src::any;
        if (s.irq_state != irq_state_now) {
            s.irq_state = irq_state_now;
            cpu.set_irq(s.irq_state);
        }
    }

    /*NMOS6502::Sig cpu_trap {
        [this]() {
            cpu.pc = 0xebff; // start of idle loop
            cpu.resume();
            if ((!dc.status.head.active()) && (!(s.ram[0x26c] | s.ram[0x7c]))) {
                run_cfg_change = idle = true;
            }
        }
    };*/

    void insert_blank() {
        // TODO: generate a truly blank disk
        const std::vector<u8> d64_data(std::size_t{Files::D64::size});
        Disk_image* blank_disk = new C1541::D64(Files::D64{d64_data}); // not truly blank...
        disk_carousel.insert(0, blank_disk, "blank"); // TODO: handle 0 free slots case
    }

    NMOS6502::Sig cpu_trap {
        [this]() {
            Log::error("****** C1541 CPU halted! ******");
            Dbg::print_status(cpu, s.ram);
        }
    };

    std::vector<Menu::Immediate_action> menu_imm_actions{
        {"TOGGLE WRITE PROTECTION !", [&](){ disk_carousel.toggle_wp(); }},
        {"EJECT !",                   [&](){ disk_carousel.select(0); }},
    };
    std::vector<Menu::Action> menu_actions{
        {"INSERT BLANK ?",            [&](){ insert_blank(); }},
        {"RESET DRIVE ?",             [&](){ reset(); }},
    };

    /*bool& run_cfg_change;
    void install_idle_trap();*/
};


} // namespace C1541

#endif // C1541_H_INCLUDED