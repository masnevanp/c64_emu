#ifndef VIC_II_H_INCLUDED
#define VIC_II_H_INCLUDED

#include <iostream>
#include "common.h"
#include "state.h"


namespace VIC_II {


static constexpr u8 CIA1_PB_LP_BIT = 0x10;


using VS = State::VIC_II;
using R = State::VIC_II::R;


static constexpr u8 reg_unused[VS::REG_COUNT] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x01, 0x70, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
};


struct CR1 {
    enum : u8 {
        rst8 = 0x80, ecm = 0x40, bmm = 0x20, den = 0x10,
        rsel = 0x08, y_scroll = 0x07,
    };
};

struct CR2 {
    enum : u8 {
        mcm = 0x10, csel = 0x08, x_scroll = 0x07,
    };
};


class Core { // 6569 (PAL-B)
public:
    enum MPTR : u8 { vm = 0xf0, cg = 0x0e, };

    enum LP_src { cia = 0x1, ctrl_port = 0x2, };

    Core(
          VS& s_,
          const u8* ram_, const Color_RAM& col_ram_, const u8* charr,
          const std::function<void (const u16& addr, u8& data)>& romh_r,
          u16& ba_low, IO::Int_sig& int_sig)
        : s(s_),
          addr_space(s_.addr_space, ram_, charr, romh_r), irq(s, int_sig), ba(ba_low), lp(s, irq),
          mobs(s, addr_space, ba, irq), gfx(s, addr_space, col_ram_, ba), border(s) {}

    void reset() { for (int r = 0; r < VS::REG_COUNT; ++r) w(r, 0); }

    void set_ultimax(bool act)    { addr_space.set_ultimax(act); }
    void set_bank(u8 va14_va15)   { addr_space.set_bank(va14_va15); }
    void set_lp(u8 src, u8 bit)   { lp.set(src, bit ^ CIA1_PB_LP_BIT); }

    void r(const u8& ri, u8& data) {
        switch (ri) {
            case R::cr1:
                data = (s.reg[R::cr1] & ~CR1::rst8) | ((s.raster_y & 0x100) >> 1);
                return;
            case R::rast:
                data = s.raster_y;
                return;
            case R::mnm: case R::mnd:
                data = s.reg[ri];
                s.reg[ri] = 0;
                return;
            default:
                data = s.reg[ri] | reg_unused[ri];
        }
    }

    void w(const u8& ri, const u8& data) {
        u8 d = data & ~reg_unused[ri];
        s.reg[ri] = d;

        switch (ri) {
            case R::m0x: case R::m1x: case R::m2x: case R::m3x:
            case R::m4x: case R::m5x: case R::m6x: case R::m7x:
                MOB{s.mob[ri >> 1]}.set_x_lo(d);
                break;
            case R::mnx8: mobs.set_x_hi(d);   break;
            case R::cr1:  gfx.cr1_upd(d);     // fall through
            case R::rast: upd_raster_y_cmp(); break;
            case R::cr2:  gfx.cr2_upd(d);     break;
            case R::ireg: irq.w_ireg(d);      break;
            case R::ien:  irq.ien_upd();      break;
            case R::mnye: mobs.set_ye(d, s.line_cycle()); break;
            case R::mnmc: for (auto& m : s.mob) { MOB{m}.set_mc(d & 0x1); d >>= 1; } break;
            case R::mnxe: for (auto& m : s.mob) { MOB{m}.set_xe(d & 0x1); d >>= 1; } break;
            case R::mmc0: for (auto& m : s.mob) MOB{m}.set_mc0(d); break;
            case R::mmc1: for (auto& m : s.mob) MOB{m}.set_mc1(d); break;
            case R::m0c: case R::m1c: case R::m2c: case R::m3c:
            case R::m4c: case R::m5c: case R::m6c: case R::m7c:
                MOB{s.mob[ri - R::m0c]}.set_c(d);
                break;
        }
    }

    /* NOTE:
        mobs.prep_dma/do_dma pair is done, so that it takes the specified
        5 cycles in total. do_dma is done at the last moment possible, and
        all the memory access is then done at once (which is not how it is
        done in reality). Normally this is not an issue since the CPU is asleep
        all the way through (and hence, cannot modify the memory). But...
      TODO:
        Might a cartridge modify the memory during these cycles? Surely not in
        a 'safe' way since VIC has/uses the bus for both phases, right?
        Anyway, the current MOB access would not be 100% cycle/memory accurate
        in those cases. */

    void tick();

private:

    class Address_space {
    public:
        using State = VS::Address_space;

        u8 r(const u16& addr) const { // addr is 14bits
            // silence compiler (we do handle all the cases)
            #pragma GCC diagnostic push
            #pragma GCC diagnostic ignored "-Wreturn-type"

            switch (layouts[s.ultimax][s.bank][addr >> 12]) {
                case ram_0: return ram[0x0000 | addr];
                case ram_1: return ram[0x4000 | addr];
                case ram_2: return ram[0x8000 | addr];
                case ram_3: return ram[0xc000 | addr];
                case chr_r: return chr[0x0fff & addr];
                case romh: {
                    u8 data;
                    romh_r(addr & 0x01fff, data);
                    return data;
                }
            }

            #pragma GCC diagnostic push
        }

        void set_ultimax(bool act)  { s.ultimax = act; }
        void set_bank(u8 va14_va15) { s.bank = va14_va15; }

        Address_space(
            State& s_, const u8* ram_, const u8* charr,
            const std::function<void (const u16& addr, u8& data)>& romh_r_)
          : s(s_), ram(ram_), chr(charr), romh_r(romh_r_) {}

    private:
        enum Mapping : u8 {
            ram_0, ram_1, ram_2, ram_3,
            chr_r, romh,
        };

        static constexpr u8 layouts[2][4][4] = {
            {   // non-ultimax
                { ram_3, ram_3, ram_3, ram_3 },
                { ram_2, chr_r, ram_2, ram_2 },
                { ram_1, ram_1, ram_1, ram_1 },
                { ram_0, chr_r, ram_0, ram_0 },
            },
            {   // ultimax
                { ram_3, ram_3, ram_3, romh },
                { ram_2, ram_2, ram_2, romh },
                { ram_1, ram_1, ram_1, romh },
                { ram_0, ram_1, ram_0, romh },
            }
        };

        State& s;

        const u8* ram;
        const u8* chr;

        const std::function<void (const u16& addr, u8& data)>& romh_r;
    };


    class IRQ {
    public:
        enum Ireg : u8 {
            rst = 0x01, mdc = 0x02, mmc = 0x04, lp = 0x08,
            irq = 0x80,
        };

        void w_ireg(u8 data) { r_ireg &= ~data; update(); }
        void ien_upd()       { update(); }
        void req(u8 kind)    { r_ireg |= kind;  update(); }

        IRQ(VS& s, IO::Int_sig& int_sig_)
            : r_ireg(s.reg[R::ireg]), r_ien(s.reg[R::ien]), int_sig(int_sig_) { }

    private:
        void update() {
            if (r_ireg & r_ien) {
                r_ireg |= Ireg::irq;
                int_sig.set(IO::Int_sig::Src::vic);
            } else {
                r_ireg &= ~Ireg::irq;
                int_sig.clr(IO::Int_sig::Src::vic);
            }
        }

        u8& r_ireg;
        const u8& r_ien;
        IO::Int_sig& int_sig;
    };


    class BA_phi2 {
    public:
        void mob_start(u8 mn) { ba_low |=  (0x100 << mn); }
        void mob_done(u8 mn)  { ba_low &= ~(0x100 << mn); }

        void gfx_start(u8 aec_low_cycle) {
            if ((ba_low & 0x00ff) == 0) ba_low = (ba_low & 0xff00) | aec_low_cycle;
        }
        void gfx_done() { ba_low &= 0xff00; }

        bool phi2_aec_high(u8 current_cycle) const {
            return current_cycle < (ba_low & 0x00ff); // called only on ba lines (so it is correct in those cases)
        }

        BA_phi2(u16& ba_low_) : ba_low(ba_low_) {}
    private:
        // 8 MSBs for MOBs, 8 LSBs for GFX (stores the 'PHI2 AEC stays low' cycle),
        u16& ba_low; // any bit set ==> ba low
    };


    class Light_pen {
    public:
        void reset() {
            if (s.lp.triggered & ~per_frame) trigger();
            else s.lp.triggered = 0;
        }

        // TODO: set lp x/y (mouse event)
        void tick() {
            if (s.lp.trigger_at_phi1) {
                s.lp.trigger_at_phi1 = false;
                trigger();
            }
        }
        void set(u8 src, bool low) { // @phi2
            if (low) {
                if (!s.lp.triggered) s.lp.trigger_at_phi1 = true;
                s.lp.triggered |= (src | per_frame);
            } else {
                s.lp.triggered &= ~src;
            }
        }

        Light_pen(VS& s_, IRQ& irq_) : s(s_), irq(irq_) {}
    private:
        static const u8 per_frame = 0x80;

        VS& s;
        IRQ& irq;

        void trigger() {
            s.reg[R::lpx] = (s.raster_x() + 4) >> 1;
            s.reg[R::lpy] = s.raster_y;
            irq.req(IRQ::lp);
        }
    };


    struct MOB { // a (zero state) helper for individual mobs
        static constexpr u8 transparent = VS::MOB::transparent;

        VS::MOB& s;

        void set_x_lo(u8 lo)  { s.x = (s.x & 0x100) | lo; }
        void set_ye(bool ye_) {
            s.ye = ye_;
            if (!s.ye) s.mdc_base_upd = true;
        }
        void set_mc(bool mc)  { s.pixel_mask = 0b10 | mc; s.shift_amount = 1 + mc; }
        void set_xe(bool xe)  { s.shift_base_delay = 1 + xe; }
        void set_c(u8 c)      { s.col[2] = c; }
        void set_mc0(u8 mc0)  { s.col[1] = mc0; }
        void set_mc1(u8 mc1)  { s.col[3] = mc1; }

        bool dma_on() const   { return s.mdc_base < 63; }
        bool dma_done() const { return s.mdc_base == 63; }
        bool dma_off() const  { return s.mdc_base == 0xff; }
        void set_dma_off() {
            s.mdc_base = 0xff;
            s.data = 0;
        }

        void load_data(u32 d1, u32 d2, u32 d3) {
            s.data = (d1 << 24) | (d2 << 16) | (d3 << 8) | Data_status::waiting;
        }

        // during blanking
        void update(u16 px, const u8 mob_bit, u8* mob_presence, u8& mmc) {
            if (is_waiting()) {
                if (!is_scheduled()) {
                    if (s.x < px || (s.x >= (px + 8))) return;
                    schedule(s.x - px);
                    return;
                } else {
                    px = s.shift_timer;
                    release();
                }
            } else {
                px = 0;
            }

            for (; px < 8; ++px) {
                const u8 p_col = s.col[(s.data >> 30) & s.pixel_mask];

                if (p_col != transparent) {
                    if (mob_presence[px]) mmc |= (mob_presence[px] | mob_bit);
                    mob_presence[px] |= mob_bit;
                }

                if ((++s.shift_timer % s.shift_delay) == 0) {
                    s.data <<= s.shift_amount;
                    if (!s.data) return;
                }
            }

            s.shift_delay = s.shift_base_delay * s.shift_amount;
        }

        void output(u16 px, const u8* gfx_out, const u8 mdp,
                        const u8 mob_bit, u8* mob_output, u8* mob_presence,
                        u8& mdc, u8& mmc)
        {
            if (is_waiting()) {
                if (!is_scheduled()) {
                    if (s.x < px || (s.x >= (px + 8))) return;
                    schedule(s.x - px);
                    return;
                } else {
                    px = s.shift_timer;
                    release();
                }
            } else {
                px = 0;
            }

            for (; px < 8; ++px) {
                const u8 p_col = s.col[(s.data >> 30) & s.pixel_mask];

                if (p_col != transparent) {
                    if (mob_presence[px]) mmc |= (mob_presence[px] | mob_bit);
                    mob_presence[px] |= mob_bit;

                    if (gfx_out[px] & GFX::foreground) mdc |= mob_bit;

                    mob_output[px] = p_col | mdp;
                }

                if ((++s.shift_timer % s.shift_delay) == 0) {
                    s.data <<= s.shift_amount;
                    if (!s.data) return;
                }
            }

            // NOTE: this comes a few pixels late (should be done
            //       inside the loop above, but meh..)
            s.shift_delay = s.shift_base_delay * s.shift_amount;
        }

    private:
        enum Data_status : u8 { waiting = 0b01, scheduled = 0b10 };

        bool is_waiting() const   { return s.data & Data_status::waiting; }
        bool is_scheduled() const { return s.data & Data_status::scheduled; }
        void schedule(u8 x_offset) {
            s.data |= Data_status::scheduled;
            s.shift_timer = x_offset; // use as temp storage
            s.shift_delay = s.shift_base_delay * s.shift_amount;
        }
        void release() {
            s.data &= ~(Data_status::waiting | Data_status::scheduled);
            s.shift_timer = 0;
        }
    };


    class MOBs {
    public:
        static constexpr int mob_count = VS::MOB_COUNT;
        static constexpr u16 MP_BASE   = 0x3f8; // offset from vm_base

        VS::MOB (&mob)[mob_count]; // array reference

        void set_x_hi(u8 mnx8) {
            auto hi_bits = mnx8 << 8;
            for (auto& m : mob) {
                m.x = (hi_bits & 0x100) | (m.x & 0x0ff);
                hi_bits >>= 1;
            }
        }
        void set_ye(u8 mnye, const int line_cycle) {
            const bool no_crunch = line_cycle != 14;
            if (no_crunch) {
                for (auto& m : mob) { MOB{m}.set_ye(mnye & 0b1); mnye >>= 1; }
            } else {
                for (auto& m : mob) {
                    const bool ye_new = mnye & 0b1;
                    if (MOB{m}.dma_on()) {
                        const bool crunch_mob = (m.ye & (m.ye ^ ye_new));
                        if (crunch_mob) {
                            m.mdc = ((m.mdc_base & m.mdc) & 0b101010)
                                        | ((m.mdc_base | m.mdc) & 0b010101);
                        }
                    }
                    MOB{m}.set_ye(ye_new);
                    mnye >>= 1;
                }
            }
        }

        void check_dma_ye() {
            for (auto&m : mob) if (m.ye) m.mdc_base_upd = !m.mdc_base_upd;
        }
        void check_dma_start() {
            for (u8 mn = 0, mb = 0x01; mn < mob_count; ++mn, mb <<= 1) {
                VS::MOB& m = mob[mn];
                if ((reg[R::mne] & mb) && !MOB{m}.dma_on()) {
                    if (reg[R::m0y + (mn * 2)] == (raster_y & 0xff)) {
                        m.mdc_base = 0; // initiates dma
                        m.mdc_base_upd = !m.ye;
                    }
                }
            }
        }
        void check_disp() {
            for (u8 mn = 0; mn < mob_count; ++mn) {
                VS::MOB& m = mob[mn];
                if (MOB{m}.dma_on()) {
                    m.mdc = m.mdc_base;
                    const bool ry_match = (reg[R::m0y + (mn * 2)] == (raster_y & 0xff));
                    if (ry_match && (reg[R::mne] & (0b1 << mn))) {
                        m.disp_on = true;
                    }
                }
            }
        }

        void upd_dma() {
            for(auto&m : mob) {
                if (MOB{m}.dma_on() && m.mdc_base_upd) m.mdc_base = m.mdc;
            }
        }
        void prep_dma(u8 mn) { if (MOB{mob[mn]}.dma_on()) ba.mob_start(mn); }
        void do_dma(u8 mn); // do p&s -accesses

        void update(u16 x); // update during blanks

        void output(u16 x, u8* to);

        MOBs(
              VS& s, Address_space& addr_space_,
              BA_phi2& ba_, IRQ& irq_)
            : mob(s.mob), addr_space(addr_space_), reg(s.reg), raster_y(s.raster_y),
              ba(ba_), irq(irq_) {}

    private:
        void _update(int start_mn, u16 x) {
            for (int mn = start_mn; mn < mob_count; ++mn) {
                if (mob[mn].data) {
                    const u8 mob_bit = 0b1 << mn;
                    MOB{mob[mn]}.update(x, mob_bit, mob_presence, mmc);
                }
            }

            if (mmc) {
                if (reg[R::mnm] == 0b00000000) irq.req(IRQ::mmc);
                reg[R::mnm] |= mmc;
                mmc = 0b00000000;
            }

            for (int p = 0; p < 8; ++p) mob_presence[p] = 0b00000000;
        }

        void _output(int start_mn, u16 x, u8* to) { // also does collision detection
            for (int mn = start_mn; mn >= 0; --mn) {
                if (mob[mn].data) {
                    const u8 mob_bit = 0b1 << mn;
                    const u8 mdp = (reg[R::mndp] << (7 - mn)) & 0b10000000;
                    MOB{mob[mn]}.output(x, to, mdp, mob_bit, mob_output, mob_presence, mdc, mmc);
                }
            }

            if (mdc) {
                if (reg[R::mnd] == 0b00000000) irq.req(IRQ::mdc);
                reg[R::mnd] |= mdc;
                mdc = 0b00000000;
            }

            if (mmc) {
                if (reg[R::mnm] == 0b00000000) irq.req(IRQ::mmc);
                reg[R::mnm] |= mmc;
                mmc = 0b00000000;
            }

            for (int p = 0; p < 8; ++p) {
                if (mob_output[p] != MOB::transparent) {
                    if ((to[p] & mob_output[p] & GFX::foreground) == 0b00000000) {
                        to[p] = mob_output[p];
                    }
                    mob_output[p] = MOB::transparent;
                    mob_presence[p] = 0b00000000;
                }
            }
        }

        u8 mob_output[8] = {
            MOB::transparent, MOB::transparent, MOB::transparent, MOB::transparent,
            MOB::transparent, MOB::transparent, MOB::transparent, MOB::transparent
        };
        u8 mob_presence[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
        u8 mdc = 0b00000000; // mob-gfx colliders
        u8 mmc = 0b00000000; // mob-mob colliders

        const Address_space& addr_space;
        u8* reg;
        const u16& raster_y;
        BA_phi2& ba;
        IRQ& irq;
    };


    class GFX {
    public:
        using State = VS::GFX;

        static const u16 vma_start_ry = 48;
        static const u16 vma_done_ry  = 248;
        static const u8  foreground   = 0b10000000;

        void cr1_upd(const u8& cr1) { // @phi2
            const u8 ecm_bmm = (cr1 & (CR1::ecm | CR1::bmm)) >> 4;
            const u8 mcm = gs.mode & mcm_set;
            gs.mode = ecm_bmm | mcm;

            gs.y_scroll = cr1 & CR1::y_scroll;

            const auto cycle = vs.line_cycle();
            if (cycle < 62) {
                gs.ba_area |= ((raster_y == vma_start_ry) && (cr1 & CR1::den));

                gs.ba_line = gs.ba_area && ((raster_y & CR1::y_scroll) == gs.y_scroll);
                if (gs.ba_line) {
                    if (cycle < 14 || cycle > 53) activate(); // else 'DMA delay' (see read_vm())
                    if (cycle >= 11 && cycle <= 52) ba.gfx_start(cycle + 4); // it would be '+3' at the next phi1
                } else {
                    ba.gfx_done();
                }
            }
        }
        void cr2_upd(const u8& cr2) { // @phi2
            const u8 mcm = (cr2 & CR2::mcm) >> 4;
            const u8 ecm_bmm = gs.mode & ~mcm_set;
            gs.mode = ecm_bmm | mcm;
        }

        void vma_start() { gs.ba_area = vs.cr1(CR1::den); gs.vc_base = 0; }
        void vma_done()  { gs.ba_area = false;  }

        void ba_init() {
            gs.ba_line = gs.ba_area && ((raster_y & CR1::y_scroll) == gs.y_scroll);
            if (gs.ba_line) activate();
        }
        void ba_start() { if (gs.ba_line) ba.gfx_start(14); }
        void ba_done() {
            if (gs.ba_line) {
                activate();
                ba.gfx_done();
            }
        }

        void row_start() {
            gs.vc = gs.vc_base;
            gs.vmri = 0;
            gs.vmoi = 0;
            if (gs.ba_line) gs.rc = 0;
        }
        void row_done() {
            if (gs.rc == 7) {
                gs.vc_base = gs.vc;
                if (!gs.ba_line) {
                    deactivate();
                    return;
                }
			}

            if (active()) gs.rc = (gs.rc + 1) & 0x7;
        }

        void init_vmd() { if (!active()) gs.vmd = {0, 0}; }

        void read_vm();
        void read_gd();

        void feed_pipeline();

        void output(u8* to);

        void output_border(u8* to);

        GFX(
            VS& vs_, Address_space& addr_space_,
            const Color_RAM& col_ram_, BA_phi2& ba_)
          : vs(vs_), gs(vs_.gfx), addr_space(addr_space_), col_ram(col_ram_), reg(vs.reg),
            raster_y(vs.raster_y), ba(ba_) {}

    private:
        enum Mode_bit : u8 {
            ecm_set = 0b100, bmm_set = 0b010, mcm_set = 0b001,
            not_set = 0b000,
        };

        enum Mode : u8 {
        /*  mode    = ECM-bit | BMM-bit | MCM-bit */
            scm     = not_set | not_set | not_set, // standard character mode
            mccm    = not_set | not_set | mcm_set, // multi-color character mode
            sbmm    = not_set | bmm_set | not_set, // standard bit map mode
            mcbmm   = not_set | bmm_set | mcm_set, // multi-color bit map mode
            ecm     = ecm_set | not_set | not_set, // extended color mode
            icm     = ecm_set | not_set | mcm_set, // invalid character mode
            ibmm1   = ecm_set | bmm_set | not_set, // invalid bit map mode 1
            ibmm2   = ecm_set | bmm_set | mcm_set, // invalid bit map mode 2
        };

        static const u8  multicolor    = 0x8;
        static const u16 addr_idle     = 0x3fff;
        static const u16 addr_ecm_mask = 0x39ff;

        void activate()     { gs.act = true; }
        void deactivate()   { gs.act = false; }
        bool active() const { return gs.act; }

        const VS& vs;
        State& gs;
        const Address_space& addr_space;
        const Color_RAM& col_ram;
        const u8* reg;
        const u16& raster_y;
        BA_phi2& ba;
    };


    class Border {
    public:
        using State = VS::Border;

        static constexpr u32 not_set = VS::Border::not_set;

        void check_left(u8 cmp_csel) {
            if (vs.cr2(CR2::csel) == cmp_csel) {
                check_lock();
                if (!is_locked() && is_on()) {
                    s.off_at = vs.beam_pos + (7 + (cmp_csel >> 3));
                }
            }
        }

        void check_right(u8 cmp_csel) {
            if (!is_on() && vs.cr2(CR2::csel) == cmp_csel) {
                s.on_at = vs.beam_pos + (7 + (cmp_csel >> 3));
                s.off_at = not_set;
            }
        }

        void check_right_vb(u8 cmp_csel) { // v-blank
            if (!is_on() && vs.cr2(CR2::csel) == cmp_csel) {
                s.on_at = 0;
                s.off_at = not_set;
            }
        }

        void line_done() { check_lock(); }

        void vb_end() { if (is_on()) s.on_at = 0; }

        void output(u32 upto_pos) {
            if (is_on()) {
                if (going_off()) {
                    while (s.on_at < s.off_at) vs.frame[s.on_at++] = vs.reg[R::ecol];
                    s.on_at = not_set;
                } else {
                    while (s.on_at < upto_pos) vs.frame[s.on_at++] = vs.reg[R::ecol];
                }
            }
        }

        Border(VS& vs_) : vs(vs_), s(vs_.border) {}

    private:
        bool is_on()     const { return s.on_at != not_set; }
        bool going_off() const { return s.off_at != not_set; }
        bool is_locked() const { return vs.gfx.blocked; }

        // (if border 'locked', gfx unit shall output only bg color)
        void lock()   { vs.gfx.blocked = true; }
        void unlock() { vs.gfx.blocked = false; }

        bool top()    const { return vs.raster_y == ( 55 - (vs.cr1(CR1::rsel) >> 1)); }
        bool bottom() const { return vs.raster_y == (247 + (vs.cr1(CR1::rsel) >> 1)); }

        void check_lock() {
            if (bottom()) lock();
            else if (top() && vs.cr1(CR1::den)) unlock();
        }

        VS& vs;
        State& s;
    };

    void check_raster_irq() {
        if (s.raster_y != s.raster_y_cmp) {
            s.raster_y_cmp_edge = true;
        } else {
            if (s.raster_y_cmp_edge) {
                s.raster_y_cmp_edge = false;
                irq.req(IRQ::rst);
            }
        }
    }

    void upd_raster_y_cmp() {
        const u16 upd = (s.cr1(CR1::rst8) << 1) | s.reg[R::rast];
        if (s.raster_y_cmp != upd) {
            s.raster_y_cmp = upd;
            if (s.line_cycle() < 62) check_raster_irq();
        }
    }

    u8* beam_ptr() const { return &s.frame[s.beam_pos]; }

    // during blanking periods
    void update_mobs() { mobs.update(s.raster_x()); }

    void output_border(); // in left/right border area

    void output();

    VS& s;

    Address_space addr_space;
    IRQ irq;
    BA_phi2 ba;
    Light_pen lp;
    MOBs mobs;
    GFX gfx;
    Border border;

};


} // namespace VIC_II

#endif // VIC_II_H_INCLUDED
