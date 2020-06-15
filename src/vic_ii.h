#ifndef VIC_II_H_INCLUDED
#define VIC_II_H_INCLUDED

#include <iostream>
#include "common.h"


namespace VIC_II {


static const u8 REG_COUNT = 64;

/*
    http://www.commodore.ca/manuals/c64_programmers_reference/c64-programmers_reference_guide-08-schematics.pdf

    y1 (color clock) freq                 = 17734472 (pal)
    cpu_freq = y1 / 18                    = 985248.444...
    raster_lines_per_frame                = 312
    cycles_per_raster_line                = 63
    frame_cycle_cnt = 312 * 63            = 19656
    pal_freq = cpu_freq / frame_cycle_cnt = 50.12456474...
    frame_duration_ms = 1000 / pal_freq   = 19.95029793...
*/
static const double FRAME_MS       = 1000.0 / (CPU_FREQ / (312.0 * 63));


// TODO: parameterize view/border size (all fixed for now)
static const u16 BORDER_SZ_V       = 28;
static const u16 BORDER_SZ_H       = 20;

static const u16 VIEW_WIDTH        = 320 + 2 * BORDER_SZ_V;
static const u16 VIEW_HEIGHT       = 200 + 2 * BORDER_SZ_H;

static const u16 RASTER_LINE_COUNT = 312;
static const u8  LINE_CYCLES       =  63;


class Color_RAM {
public:
    static const u16 size = 0x0400;

    void r(const u16& addr, u8& data) const { data = ram[addr]; }
    void w(const u16& addr, const u8& data) { ram[addr] = data & 0xf; } // masking the write is enough

    Color_RAM(u8* ram_) : ram(ram_) {}

private:
    u8* ram; // just the lower nibble is actually used

};


enum R : u8 {
    m0x,  m0y,  m1x,  m1y,  m2x,  m2y,  m3x,  m3y,  m4x,  m4y,  m5x,  m5y,  m6x,  m6y,  m7x,  m7y,
    mnx8, cr1,  rast, lpx,  lpy,  mne,  cr2,  mnye, mptr, ireg, ien,  mndp, mnmc, mnxe, mnm,  mnd,
    ecol, bgc0, bgc1, bgc2, bgc3, mmc0, mmc1, m0c,  m1c,  m2c,  m3c,  m4c,  m5c,  m6c,  m7c,
};

static constexpr u8 reg_unused[REG_COUNT] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x00, 0x70, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00,
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

enum MPTR : u8 { vm = 0xf0, cg = 0x0e, };


template <typename Out>
class Core { // 6569 (PAL-B)
public:
    struct State;

private:

    class Banker {
    public:
        struct State {
            // TODO: exrom/game bits
            u8 va14_va15 = 0b11; // NOTE: bits inverted (0b11 --> bank 0)
        };

        u8 r(const u16& addr) const { // addr is 14bits
            switch (bank[s.va14_va15][addr >> 12]) {
                case ram_0: return ram[0x0000 | addr];
                case ram_1: return ram[0x4000 | addr];
                case ram_2: return ram[0x8000 | addr];
                case ram_3: return ram[0xc000 | addr];
                case chr_r: return chr[0x0fff & addr];
                case romh:  return 0x00; // TODO
                case none:  return 0x00; // TODO: wut..?
            }
            return 0x00; // silence compiler...
        }

        void set_va14_va15(u8 v) { s.va14_va15 = v; }

        Banker(State& s_, const u8* ram_, const u8* charr)
            : s(s_), ram(ram_), chr(charr) {}

    private:
        enum Mapping : u8 {
            ram_0, ram_1, ram_2, ram_3,
            chr_r, romh, none,
        };

        // TODO: full mode handling (ultimax + special modes)
        static constexpr u8 bank[4][4] = { // [mode]
            { ram_3, ram_3, ram_3, ram_3 },
            { ram_2, chr_r, ram_2, ram_2 },
            { ram_1, ram_1, ram_1, ram_1 },
            { ram_0, chr_r, ram_0, ram_0 },
        };

        State& s;

        const u8* ram;
        const u8* chr;
        //const u8* romh;
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

        IRQ(State& s, Out& out_)
            : r_ireg(s.reg[R::ireg]), r_ien(s.reg[R::ien]), out(out_) { }

    private:
        void update() {
            if (r_ireg & r_ien) {
                r_ireg |= Ireg::irq;
                out.set_irq();
            } else {
                r_ireg &= ~Ireg::irq;
                out.clr_irq();
            }
        }

        u8& r_ireg;
        const u8& r_ien;
        Out& out;
    };


    class BA {
    public:
        void mob_start(u8 mn)  { ba_low |=  (0x100 << mn); }
        void mob_done(u8 mn)   { ba_low &= ~(0x100 << mn); }
        void gfx_set(u8 b)     { ba_low = (ba_low & 0xff00) | b; }
        void gfx_rel()         { gfx_set(0); }

        BA(u16& ba_low_) : ba_low(ba_low_) {}
    private:
        u16& ba_low; // 8 MSBs for MOBs, 8 LSBs for GFX
    };


    class Light_pen {
    public:
        struct State { u8 triggered = 0; };

        void reset() {
            if (s.lp.triggered & ~per_frame) trigger();
            else s.lp.triggered = 0;
        }

        // TODO: set lp x/y (mouse event)
        void set(u8 src, u8 low) {
            if (low) {
                if (!s.lp.triggered) trigger();
                s.lp.triggered |= (src | per_frame);
            } else {
                s.lp.triggered &= ~src;
            }
        }

        Light_pen(Core::State& s_, IRQ& irq_) : s(s_), irq(irq_) {}
    private:
        static const u8 per_frame = 0x80;

        Core::State& s;
        IRQ& irq;

        void trigger() {
            // TODO: need to adjust the x for CIA originated ones?
            s.reg[R::lpx] = s.raster_x >> 1;
            s.reg[R::lpy] = s.raster_y;
            irq.req(IRQ::lp);
        }
    };


    class MOBs {
    public:
        static constexpr u8  mob_count = 8;
        static constexpr u16 MP_BASE   = 0x3f8; // offset from vm_base

        struct MOB {
            static constexpr u8 transparent_px = 0xff;

            enum : u32 {
                pristine_data  = 0x00000003,
                pristine_flip  = 0x00000002,
                out_of_data    = 0x01000000,

                pixel_mask_sc  = 0x80000000,
                pixel_mask_mc  = 0xc0000000,
            };

            void set_x_hi(u8 hi)  { x = hi ? x | 0x100 : x & 0x0ff; }
            void set_x_lo(u8 lo)  { x = (x & 0x100) | lo; }
            void set_ye(bool ye_) { ye = ye_; if (!ye) mdc_base_upd = true; }
            void set_mc(bool mc)  {
                pixel_mask = mc ? pixel_mask_mc : pixel_mask_sc;
                shift_amount = mc ? 2 : 1;
            }
            void set_xe(bool xe)  { shift_base_delay = xe ? 2 : 1; }
            void set_c(u8 c)      { col[2] = c; }
            void set_mc0(u8 mc0)  { col[1] = mc0; }
            void set_mc1(u8 mc1)  { col[3] = mc1; }

            void load_data(u32 d1, u32 d2, u32 d3) {
                data = (d1 << 24) | (d2 << 16) | (d3 << 8) | pristine_data;
                shift_delay = shift_base_delay * shift_amount;
            }

            u8 pixel_out(u16 px) {
                if (data == out_of_data) return transparent_px;

                if ((data & pristine_data) == pristine_data) {
                    if (px != x) return transparent_px;
                    data ^= pristine_flip;
                }

                u8 p_col = col[(data & pixel_mask) >> 30];

                if (++shift_timer == shift_delay) {
                    data <<= shift_amount;
                    shift_timer = 0;
                }

                return p_col;
            }

            u8 ye;
            u8 dma_on = false;
            u8 mdc_base_upd;

            u8 mdc_base;
            u8 mdc;

            u16 x;

            u32 data = out_of_data;
            u32 pixel_mask;

            u8 shift_base_delay;
            u8 shift_amount;
            u8 shift_delay;
            u8 shift_timer = 0;

            u8 col[4] = { transparent_px, 0, 0, 0 };
        };


        MOB (&mob)[mob_count]; // array reference (sweet syntax)

        void check_mdc_base_upd() {
            for(auto&m : mob) if (m.ye) m.mdc_base_upd = !m.mdc_base_upd;
        }
        void check_dma() {
            for (u8 mn = 0, mb = 0x01; mn < mob_count; ++mn, mb <<= 1) {
                MOB& m = mob[mn];
                if (!m.dma_on) {
                    if ((reg[R::mne] & mb) && (reg[R::m0y + (mn * 2)]
                                                        == (raster_y & 0xff))) {
                        m.dma_on = true;
                        m.mdc_base = 0;
                        if (m.ye) m.mdc_base_upd = false;
                    }
                }
            }
        }
        void load_mdc()       { for (auto& m : mob) m.mdc = m.mdc_base; }
        void inc_mdc_base_2() { for (auto& m : mob) m.mdc_base += (m.mdc_base_upd * 2); }
        void inc_mdc_base_1() {
            for (auto& m : mob) {
                m.mdc_base += m.mdc_base_upd;
                if (m.mdc_base == 63) m.dma_on = false;
            }
        }
        void prep_dma(u8 mn) { if (mob[mn].dma_on) ba.mob_start(mn); }
        void do_dma(u8 mn)  { // do p&s -accesses
            MOB& m = mob[mn];
            // if dma_on, then cpu has already been stopped --> safe to read all at once (1p + 3s)
            if (m.dma_on) {
                u16 mb = ((reg[R::mptr] & MPTR::vm) << 6) | MP_BASE;
                u16 mp = bank.r(mb | mn);
                mp <<= 6;
                u32 d1 = bank.r(mp | m.mdc++);
                u32 d2 = bank.r(mp | m.mdc++);
                u32 d3 = bank.r(mp | m.mdc++);
                m.load_data(d1, d2, d3);
                ba.mob_done(mn);
            }
        }

        void output(u16 x, u16 x_stop, u8* to) {
            while (x < x_stop) output(x++, to++);
        }

        void update(u16 x, u16 x_stop) { // update during h-blank
            u8 mmc_total = 0x00; // mob-mob colliders

            for (; x < x_stop; ++x) {
                bool mmc_on = false; // mob-mob coll. happened
                u8 mmc = 0x00; // mob-mob colliders (for single pixel)
                u8 mn = mob_count;
                u8 mb = 0x80; // mob bit ('id')

                do {
                    MOB& m = mob[--mn];
                    if (m.pixel_out(x) != MOB::transparent_px) {
                        if (mmc != 0x00) mmc_on = true;
                        mmc |= mb;
                    }
                    mb >>= 1;
                } while (mn > 0);

                if (mmc_on) mmc_total |= mmc;
            }

            if (mmc_total) {
                if (reg[R::mnm] == 0x00) irq.req(IRQ::mmc);
                reg[R::mnm] |= mmc_total;
            }
        }

        MOBs(
              State& s, Banker& bank_,
              BA& ba_, IRQ& irq_)
            : mob(s.mob), bank(bank_), reg(s.reg), raster_y(s.raster_y),
              ba(ba_), irq(irq_) {}

    private:
        void output(u16 x, u8* to) { // also does collision detection
            const u8 gfx_fg = (*to & GFX::foreground) ? 0xff : 0x00;
            u8 mdc = 0x00; // mob-gfx colliders
            u8 mmc = 0x00; // mob-mob colliders
            u8 mmc_on = false; // mob-mob coll. happened
            u8 col;
            u8 src_mb = 0x00; // source mob bit, if 'non-transparent'
            u8 mn = mob_count;
            u8 mb = 0x80; // mob bit ('id')

            do {
                MOB& m = mob[--mn];
                u8 c = m.pixel_out(x);
                if (c != MOB::transparent_px) {
                    col = c;
                    src_mb = mb;

                    mdc |= (mb & gfx_fg);

                    if (mmc != 0x00) mmc_on = true;
                    mmc |= mb;
                }
                mb >>= 1;
            } while (mn > 0);

            if (src_mb) {
                // priority: mob|gfx_fg (based on reg[R::mndp]) > mob > gfx_bg
                if (gfx_fg) {
                    if (reg[R::mnd] == 0x00) irq.req(IRQ::mdc);
                    reg[R::mnd] |= mdc;

                    if (!(reg[R::mndp] & src_mb)) *to = col;
                } else {
                    *to = col;
                }

                if (mmc_on) {
                    if (reg[R::mnm] == 0x00) irq.req(IRQ::mmc);
                    reg[R::mnm] |= mmc;
                }
            }
        }

        const Banker& bank;
        u8* reg;
        const u16& raster_y;
        BA& ba;
        IRQ& irq;
    };


    class GFX {
    public:
        static const u16 vma_top_raster_y    = 48;
        static const u16 vma_bottom_raster_y = 248;
        static const u8  foreground          = 0x80;

        struct State {
            u8 mode = scm_i;

            u8 y_scroll;

            u8 ba_area = false; // set for each frame (at the top of disp.win)
            u8 ba_line = false;

            struct VM_data { // video-matrix data
                u8 data = 0; // 'screen memory' data
                u8 col = 0;  // color-ram data
            };
            VM_data vm[42]; // vm row buffer

            u16 vc_base;
            u16 vc;
            u16 rc;
            u16 vb;   // vmoi bump timer
            u16 gfx_addr;
            u32 gd;
            u8  gdr;
            u8  vmri; // vm read index
            u8  vmoi; // vm output index
            u8  col;
        };

        void cr1_upd(const u8& cr1) {
            u8 ecm_bmm = (cr1 & (CR1::ecm | CR1::bmm)) >> 3;
            u8 mcm_act = gs.mode & (mcm_set | act_set);
            gs.mode = ecm_bmm | mcm_act;

            gs.ba_area |= ((raster_y == vma_top_raster_y) && (cr1 & CR1::den));
            ba_check();
            gs.y_scroll = cr1 & CR1::y_scroll;
            ba_check();
            if (!gs.ba_line) ba.gfx_rel();
        }

        void cr2_upd(const u8& cr2) {
            u8 mcm = (cr2 & CR2::mcm) >> 3;
            u8 ecm_bmm_act = gs.mode & ~mcm_set;
            gs.mode = ecm_bmm_act | mcm;
        }

        void vma_start() { gs.ba_area = cs.cr1(CR1::den); gs.vc_base = 0; }
        void vma_done()  { gs.ba_area = gs.ba_line = false;  }

        void ba_check() {
            if (gs.ba_area) {
                gs.ba_line = (raster_y & CR1::y_scroll) == gs.y_scroll;
                if (gs.ba_line) {
                    if (line_cycle < 14) activate(); // else delayed (see read_vm())
                    if (line_cycle > 10 && line_cycle < 54) ba.gfx_set(line_cycle | 0x80); // store cycle for AEC checking later
                }
            }
        }

        void ba_done() { ba.gfx_rel(); }

        void row_start() {
            gs.vc = gs.vc_base;
            gs.vmri = 1 + active();
            gs.vmoi = gs.vmri - 1; // if idle, we stay at index zero (with zeroed data)
            gs.vm[1] = gs.vm[41]; // wrap around (make last one linger...)
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
            gs.rc = (gs.rc + active()) & 0x7;
        }

        void read_vm() {
            if (gs.ba_line) {
                activate(); // delayed (see ba_check())
                col_ram.r(gs.vc, gs.vm[gs.vmri].col); // TODO: mask upper bits if 'noise' is implemented
                u16 vaddr = ((reg[R::mptr] & MPTR::vm) << 6) | gs.vc;
                gs.vm[gs.vmri].data = bank.r(vaddr);
            }
        }

        void read_gd() {
            gs.gdr = bank.r(gs.gfx_addr);
            gs.vc = (gs.vc + active()) & 0x3ff;
            gs.vmri += active();
        }

        void feed_gd() {
            u8 xs = cs.cr2(CR2::x_scroll);
            gs.gd |= (gs.gdr << (20 - xs));
            gs.vb |= (0x10 << xs);
        }

        void output(u8* to) {
            u8 bumps = 8;
            const auto put = [&to](const u8 px) { *to++ = px; };
            const auto bump = [&bumps, this]() {
                gs.gd <<= 1;
                gs.vb >>= 1;
                gs.vmoi += (gs.vb & active());
                return --bumps;
            };
            const auto is_aligned = [this]() { return !(cs.cr2(CR2::x_scroll) & 0x1); };
            const auto c_base =     [this]() { return (reg[R::mptr] & MPTR::cg) << 10; };

            switch (gs.mode) {
                case scm_i:
                    do put(gs.gd & 0x80000000 ? 0x0 | foreground : reg[R::bgc0]);
                    while (bump());
                    gs.gfx_addr = addr_idle;
                    return;

                case scm:
                    do put(gs.gd & 0x80000000
                            ? gs.vm[gs.vmoi].col | foreground
                            : reg[R::bgc0]);
                    while (bump());
                    gs.gfx_addr = c_base() | (gs.vm[gs.vmri].data << 3) | gs.rc;
                    return;

                case mccm_i:
                    do put(gs.gd & 0x80000000 ? 0x0 | foreground : reg[R::bgc0]);
                    while (bump());
                    gs.gfx_addr = addr_idle;
                    return;

                case mccm: {
                    bool aligned;
                    if (gs.vm[gs.vmoi].col & multicolor) {
                        aligned = is_aligned();
                        do {
                            if (aligned) {
                                u8 gd = (gs.gd >> 24) & 0xc0;
                                u8 c = (gd == 0xc0)
                                            ? gs.vm[gs.vmoi].col & 0x7
                                            : reg[R::bgc0 + (gd >> 6)];
                                gs.col = (c | gd); // sets also fg-gfx flag
                            }
                            put(gs.col);
                            gs.gd <<= 1; gs.vb >>= 1;
                            if (gs.vb & 0x1) {
                                ++gs.vmoi;
                                if (!(gs.vm[gs.vmoi].col & multicolor)) goto _scm;
                            }
                            aligned = !aligned;
                            _mccm:
                            --bumps;
                        } while (bumps);
                    } else {
                        do {
                            put(gs.gd & 0x80000000
                                    ? gs.vm[gs.vmoi].col | foreground
                                    : reg[R::bgc0]);
                            gs.gd <<= 1; gs.vb >>= 1;
                            if (gs.vb & 0x1) {
                                ++gs.vmoi;
                                if (gs.vm[gs.vmoi].col & multicolor) {
                                    aligned = true;
                                    goto _mccm;
                                }
                            }
                            _scm:
                            --bumps;
                        } while (bumps);
                    }
                    gs.gfx_addr = c_base() | (gs.vm[gs.vmri].data << 3) | gs.rc;
                    return;
                }

                case sbmm_i:
                    do put(gs.gd & 0x80000000 ? 0x0 | foreground : 0x0);
                    while (bump());
                    gs.gfx_addr = addr_idle;
                    return;

                case sbmm:
                    do put(gs.gd & 0x80000000
                                ? (gs.vm[gs.vmoi].data >> 4) | foreground
                                : (gs.vm[gs.vmoi].data & 0xf));
                    while (bump());
                    gs.gfx_addr = (c_base() & 0x2000) | (gs.vc << 3) | gs.rc;
                    return;

                case mcbmm_i: {
                    bool aligned = is_aligned();
                    do {
                        if (aligned) {
                            switch (gs.gd >> 30) {
                                case 0x0: gs.col = reg[R::bgc0];     break;
                                case 0x1: gs.col = 0x0;              break;
                                case 0x2: gs.col = 0x0 | foreground; break;
                                case 0x3: gs.col = 0x0 | foreground; break;
                            }
                        }
                        put(gs.col);
                        aligned = !aligned;
                    } while (bump());
                    gs.gfx_addr = addr_idle;
                    return;
                }

                case mcbmm: {
                    bool aligned = is_aligned();
                    do {
                        if (aligned) {
                            switch (gs.gd >> 30) {
                                case 0x0: gs.col = reg[R::bgc0];                             break;
                                case 0x1: gs.col = (gs.vm[gs.vmoi].data >> 4);               break;
                                case 0x2: gs.col = (gs.vm[gs.vmoi].data & 0xf) | foreground; break;
                                case 0x3: gs.col = gs.vm[gs.vmoi].col | foreground;          break;
                            }
                        }
                        put(gs.col);
                        aligned = !aligned;
                    } while (bump());
                    gs.gfx_addr = (c_base() & 0x2000) | (gs.vc << 3) | gs.rc;
                    return;
                }

                case ecm_i:
                    do put(gs.gd & 0x80000000 ? 0x0 | foreground : reg[R::bgc0]);
                    while (bump());
                    gs.gfx_addr = addr_idle & addr_ecm_mask;
                    return;

                case ecm:
                    do put(gs.gd & 0x80000000
                                ? gs.vm[gs.vmoi].col | foreground
                                : reg[R::bgc0 + (gs.vm[gs.vmoi].data >> 6)]);
                    while (bump());
                    gs.gfx_addr = (c_base() | (gs.vm[gs.vmri].data << 3) | gs.rc)
                                        & addr_ecm_mask;
                    return;

                case icm_i: case icm: {
                    bool aligned;
                    if (gs.vm[gs.vmoi].col & multicolor) {
                        aligned = is_aligned();
                        do {
                            // sets col.0 & fg-gfx flag
                            if (aligned) gs.col = (gs.gd >> 24) & 0x80;
                            put(gs.col);
                            gs.gd <<= 1; gs.vb >>= 1;
                            if (gs.vb & 0x1) {
                                ++gs.vmoi;
                                if (!(gs.vm[gs.vmoi].col & multicolor)) goto _icm_scm;
                            }
                            aligned = !aligned;
                            _icm_mccm:
                            --bumps;
                        } while (bumps);
                    } else {
                        do {
                            put((gs.gd >> 24) & 0x80); // sets col.0 & fg-gfx flag
                            gs.gd <<= 1; gs.vb >>= 1;
                            if (gs.vb & 0x1) {
                                ++gs.vmoi;
                                if (gs.vm[gs.vmoi].col & multicolor) {
                                    aligned = true;
                                    goto _icm_mccm;
                                }
                            }
                            _icm_scm:
                            --bumps;
                        } while (bumps);
                    }
                    gs.gfx_addr = (gs.mode == icm_i
                        ? addr_idle
                        : (c_base() | (gs.vm[gs.vmri].data << 3) | gs.rc))
                                & addr_ecm_mask;
                    return;
                }

                case ibmm1_i: case ibmm1:
                    do put((gs.gd >> 24) & 0x80); while (bump()); // sets col.0 & fg-gfx flag
                    gs.gfx_addr = (gs.mode == ibmm2_i
                        ? addr_idle
                        : ((c_base() & 0x2000) | (gs.vc << 3) | gs.rc))
                                & addr_ecm_mask;
                    return;

                case ibmm2_i: case ibmm2: {
                    bool aligned = is_aligned();
                    do {
                        // sets col.0 & fg-gfx flag
                        if (aligned) gs.col = (gs.gd >> 24) & 0x80;
                        put(gs.col);
                        aligned = !aligned;
                    } while (bump());
                    gs.gfx_addr = (gs.mode == ibmm2_i
                        ? addr_idle
                        : ((c_base() & 0x2000) | (gs.vc << 3) | gs.rc))
                                & addr_ecm_mask;
                    return;
                }
            }
        }

        void output_border(u8* to) {
            const auto put = [&to](const u8 px) {
                to[0] = to[1] = to[2] = to[3] = to[4] = to[5] = to[6] = to[7] = px;
            };

            switch (gs.mode) {
                case scm_i: case scm: case mccm_i: case mccm:
                    put(reg[R::bgc0]);
                    return;
                case sbmm_i: case sbmm:
                    put(gs.vm[gs.vmoi].data & 0xf);
                    return;
                case mcbmm_i: case mcbmm:
                    put(reg[R::bgc0]);
                    return;
                case ecm_i: case ecm:
                    put(reg[R::bgc0 + (gs.vm[gs.vmoi].data >> 6)]);
                    return;
                case icm_i: case icm: case ibmm1_i: case ibmm1: case ibmm2_i: case ibmm2:
                    put(0x0);
                    return;
            }
        }

        GFX(
            Core::State& cs_, Banker& bank_,
            const Color_RAM& col_ram_, BA& ba_)
          : cs(cs_), gs(cs_.gfx), bank(bank_), col_ram(col_ram_), reg(cs.reg),
            line_cycle(cs.line_cycle), raster_y(cs.raster_y), ba(ba_) {}

    private:
        enum Mode_bit : u8 {
            ecm_set = 0x8, bmm_set = 0x4, mcm_set = 0x2, act_set = 0x1,
            not_set = 0x0
        };
        enum Mode : u8 {
        /*  mode    = ECM-bit | BMM-bit | MCM-bit | active  */
            scm_i   = not_set | not_set | not_set | not_set, // standard character mode
            scm     = not_set | not_set | not_set | act_set,
            mccm_i  = not_set | not_set | mcm_set | not_set, // multi-color character mode
            mccm    = not_set | not_set | mcm_set | act_set,
            sbmm_i  = not_set | bmm_set | not_set | not_set, // standard bit map mode
            sbmm    = not_set | bmm_set | not_set | act_set,
            mcbmm_i = not_set | bmm_set | mcm_set | not_set, // multi-color bit map mode
            mcbmm   = not_set | bmm_set | mcm_set | act_set,
            ecm_i   = ecm_set | not_set | not_set | not_set, // extended color mode
            ecm     = ecm_set | not_set | not_set | act_set,
            icm_i   = ecm_set | not_set | mcm_set | not_set, // invalid character mode
            icm     = ecm_set | not_set | mcm_set | act_set,
            ibmm1_i = ecm_set | bmm_set | not_set | not_set, // invalid bit map mode 1
            ibmm1   = ecm_set | bmm_set | not_set | act_set,
            ibmm2_i = ecm_set | bmm_set | mcm_set | not_set, // invalid bit map mode 2
            ibmm2   = ecm_set | bmm_set | mcm_set | act_set,
        };

        static const u8  multicolor    = 0x8;
        static const u16 addr_idle     = 0x3fff;
        static const u16 addr_ecm_mask = 0x39ff;

        void activate()     { gs.mode |= Mode_bit::act_set; }
        void deactivate()   { gs.mode &= ~Mode_bit::act_set; }
        bool active() const { return gs.mode & Mode_bit::act_set; }

        const Core::State& cs;
        State& gs;
        const Banker& bank;
        const Color_RAM& col_ram;
        const u8* reg;
        const u8& line_cycle;
        const u16& raster_y;
        BA& ba;
    };


    class Border {
    public:
        static constexpr u32 not_set = 0xffffffff;

        struct State {
            // indicate beam_pos at which border should start/end
            u32 on_at = not_set;
            u32 off_at = not_set;

            u8 locked_on;
        };

        void check_left(u8 cmp_csel) {
            if (cs.cr2(CR2::csel) == cmp_csel) {
                if (bottom()) b.locked_on = true;
                else if (top() && cs.cr1(CR1::den)) b.locked_on = false;
                if (!b.locked_on && is_on()) {
                    b.off_at = cs.beam_pos + (3 + (cmp_csel >> 3));
                }
            }
        }

        void check_right(u8 cmp_csel) {
            if (!is_on() && cs.cr2(CR2::csel) == cmp_csel) {
                b.on_at = cs.beam_pos + (3 + (cmp_csel >> 3));
                b.off_at = not_set;
            }
        }

        void line_done() {
            if (bottom()) b.locked_on = true;
            else if (top() && cs.cr1(CR1::den)) b.locked_on = false;
        }

        void frame_start() { if (is_on()) b.on_at = 0; }

        void output(u32 upto_pos) {
            if (is_on()) {
                if (going_off()) {
                    while (b.on_at < b.off_at) cs.frame[b.on_at++] = cs.reg[R::ecol];
                    b.on_at = not_set;
                } else {
                    while (b.on_at < upto_pos) cs.frame[b.on_at++] = cs.reg[R::ecol];
                }
            }
        }

        Border(Core::State& cs_) : cs(cs_), b(cs_.border) {}

    private:
        bool is_on()      const { return  b.on_at != not_set; }
        bool going_off()  const { return b.off_at != not_set; }

        bool top()    const { return cs.raster_y == ( 55 - (cs.cr1(CR1::rsel) >> 1)); }
        bool bottom() const { return cs.raster_y == (247 + (cs.cr1(CR1::rsel) >> 1)); }

        Core::State& cs;
        State& b;
    };


    u8* beam_ptr(u32 offset = 0) const { return &s.frame[s.beam_pos + offset]; }

    // for updating the partially off-screen MOBs so that they are displayed
    // properly on the left screen edge
    void update_mobs() {
        mobs.update(s.raster_x, s.raster_x + 8);
        s.raster_x += 8;
    }

    void output_start() {
        gfx.output_border(beam_ptr()); // gfx.output(beam_pos);
        mobs.output(500, 504, beam_ptr());
        mobs.output(0, 4, beam_ptr(4));
        border.output(s.beam_pos + 4);
        s.beam_pos += 8;
        s.raster_x = 4;
    }

    void output_border() {
        gfx.output_border(beam_ptr()); // gfx.output(beam_pos);
        mobs.output(s.raster_x, s.raster_x + 8, beam_ptr());
        border.output(s.beam_pos + 4);
        s.beam_pos += 8;
        s.raster_x += 8;
    }

    void output() {
        gfx.output(beam_ptr());
        mobs.output(s.raster_x, s.raster_x + 8, beam_ptr());
        border.output(s.beam_pos + 4);
        s.beam_pos += 8;
        s.raster_x += 8;
    }

    void output_end() { border.output(s.beam_pos); }

    State& s;

    Banker banker;
    IRQ irq;
    BA ba;
    Light_pen lp;
    MOBs mobs;
    GFX gfx;
    Border border;

    Out& out;

public:
    enum LP_src { cia = 0x1, ctrl_port = 0x2, };

    struct State {
        u8 reg[REG_COUNT];

        u8 line_cycle = 0;
        u16 raster_x;
        u16 raster_y = 0;
        u8 v_blank = true;

        typename Banker::State banker;
        typename Light_pen::State lp;
        typename MOBs::MOB mob[MOBs::mob_count];
        typename GFX::State gfx;
        typename Border::State border;

        u32 beam_pos = 0;
        u8 frame[VIEW_WIDTH * VIEW_HEIGHT] = {};

        // TODO: move out..?
        u8 cr1(u8 field) const { return reg[R::cr1] & field; }
        u8 cr2(u8 field) const { return reg[R::cr2] & field; }
    };

    void reset() { for (int r = 0; r < REG_COUNT; ++r) w(r, 0); }

    void set_va14_va15(u8 v)    { banker.set_va14_va15(v); }
    void set_lp(u8 src, u8 low) { lp.set(src, low); }

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
                mobs.mob[ri >> 1].set_x_lo(d);
                break;
            case R::mnx8:
                for (auto& m : mobs.mob) { m.set_x_hi(d & 0x1); d >>= 1; }
                break;
            case R::cr1: gfx.cr1_upd(d); break;
            case R::cr2: gfx.cr2_upd(d); break;
            case R::ireg: irq.w_ireg(d); break;
            case R::ien:  irq.ien_upd(); break;
            case R::mnye: for (auto& m : mobs.mob) { m.set_ye(d & 0x1); d >>= 1; } break;
            case R::mnmc: for (auto& m : mobs.mob) { m.set_mc(d & 0x1); d >>= 1; } break;
            case R::mnxe: for (auto& m : mobs.mob) { m.set_xe(d & 0x1); d >>= 1; } break;
            case R::mmc0: for (auto& m : mobs.mob) m.set_mc0(d); break;
            case R::mmc1: for (auto& m : mobs.mob) m.set_mc1(d); break;
            case R::m0c: case R::m1c: case R::m2c: case R::m3c:
            case R::m4c: case R::m5c: case R::m6c: case R::m7c:
                mobs.mob[ri - R::m0c].set_c(d);
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

    void tick(u16& frame_cycle) {
        auto raster_cmp = [this](u16 r) {
            if (((s.cr1(CR1::rst8) << 1) | s.reg[R::rast]) == r) irq.req(IRQ::rst);
        };

        s.line_cycle = frame_cycle % LINE_CYCLES;

        switch (s.line_cycle) {
            case  0:
                mobs.do_dma(2);

                s.raster_y++;

                if (s.raster_y == RASTER_LINE_COUNT) {
                    out.sync_line(0);
                    s.raster_y = frame_cycle = s.beam_pos = 0;
                    border.frame_start();
                    return;
                }

                out.sync_line(s.raster_y);

                raster_cmp(s.raster_y);

                // TODO: reveal the magic numbers....
                if (!s.v_blank) {
                    if (s.raster_y == GFX::vma_top_raster_y) {
                        gfx.vma_start();
                    } else if (s.raster_y == GFX::vma_bottom_raster_y) {
                        gfx.vma_done();
                    } else if (s.raster_y == (250 + BORDER_SZ_H)) {
                        s.v_blank = true;
                        lp.reset();
                    }
                } else if (s.raster_y == (50 - BORDER_SZ_H)) {
                    s.v_blank = false;
                }

                return;
            case  1:
                mobs.prep_dma(5);
                if (s.raster_y == 0) raster_cmp(0);
                return;
            case  2: mobs.do_dma(3);   return;
            case  3: mobs.prep_dma(6); return;
            case  4: mobs.do_dma(4);   return;
            case  5: mobs.prep_dma(7); return;
            case  6: mobs.do_dma(5);   return;
            case  7: // 452
                if (!s.v_blank) {
                    s.raster_x = 452;
                    update_mobs();
                }
                return;
            case  8: // 460
                if (!s.v_blank) update_mobs();
                mobs.do_dma(6);
                return;
            case  9: // 468
                if (!s.v_blank) update_mobs();
                return;
            case 10: // 476
                if (!s.v_blank) update_mobs();
                mobs.do_dma(7);
                return;
            case 11: // 484
                if (!s.v_blank) {
                    gfx.ba_check();
                    update_mobs();
                }
                return;
            case 12: // 492
                if (!s.v_blank) update_mobs();
                return;
            case 13: // 500
                if (!s.v_blank) {
                    output_start();
                    gfx.row_start();
                }
                return;
            case 14: // 4
                if (!s.v_blank) {
                    output_border();
                    gfx.read_vm();
                }
                mobs.inc_mdc_base_2();
                return;
            case 15: // 12
                if (!s.v_blank) {
                    output();
                    gfx.read_gd();
                    gfx.read_vm();
                }
                mobs.inc_mdc_base_1();
                return;
            case 16: // 20
                if (!s.v_blank) {
                    gfx.feed_gd();
                    border.check_left(CR2::csel);
                    output();
                    gfx.read_gd();
                    gfx.read_vm();
                }
                return;
            case 17: // 28
                if (!s.v_blank) {
                    gfx.feed_gd();
                    border.check_left(!CR2::csel);
                    output();
                    gfx.read_gd();
                    gfx.read_vm();
                }
                return;
            case 18: case 19: // 36..
            case 20: case 21: case 22: case 23: case 24: case 25: case 26: case 27: case 28: case 29:
            case 30: case 31: case 32: case 33: case 34: case 35: case 36: case 37: case 38: case 39:
            case 40: case 41: case 42: case 43: case 44: case 45: case 46: case 47: case 48: case 49:
            case 50: case 51: case 52: case 53: // ..323
                if (!s.v_blank) {
                    gfx.feed_gd();
                    output();
                    gfx.read_gd();
                    gfx.read_vm();
                }
                return;
            case 54: // 324
                if (!s.v_blank) {
                    gfx.feed_gd();
                    output();
                    gfx.read_gd();
                    gfx.ba_done();
                }
                mobs.check_mdc_base_upd();
                mobs.check_dma();
                mobs.prep_dma(0);
                return;
            case 55: // 332
                if (!s.v_blank) {
                    gfx.feed_gd();
                    border.check_right(!CR2::csel);
                    output();
                }
                mobs.check_dma();
                mobs.prep_dma(0);
                return;
            case 56: // 340
                if (!s.v_blank) {
                    border.check_right(CR2::csel);
                    output();
                }
                mobs.prep_dma(1);
                return;
            case 57: // 348
                if (!s.v_blank) {
                    output();
                    gfx.row_done();
                }
                mobs.load_mdc();
                return;
            case 58: // 356
                if (!s.v_blank) output_border();
                mobs.prep_dma(2);
                return;
            case 59: // 364
                if (!s.v_blank) output_border();
                mobs.do_dma(0);
                return;
            case 60: // 372
                if (!s.v_blank) output_end();
                mobs.prep_dma(3);
                return;
            case 61: mobs.do_dma(1); return;
            case 62:
                border.line_done();
                mobs.prep_dma(4);
                return;
        }
    }

    Core(
          State& s_,
          const u8* ram_, const Color_RAM& col_ram_, const u8* charr,
          u16& ba_low, Out& out_)
        : s(s_),
          banker(s_.banker, ram_, charr), irq(s, out_), ba(ba_low), lp(s, irq),
          mobs(s, banker, ba, irq), gfx(s, banker, col_ram_, ba), border(s),
          out(out_) { }
};


} // namespace VIC_II

#endif // VIC_II_H_INCLUDED
