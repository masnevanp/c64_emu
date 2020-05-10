#ifndef VIC_II_H_INCLUDED
#define VIC_II_H_INCLUDED

#include <iostream>
#include "common.h"


namespace VIC_II {


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

private:
    u8 ram[size] = {}; // just the lower nibble is actually used

};


class Banker {
public:
    // TODO: ROMH
    Banker(const u8* ram_, const u8* charr) : ram(ram_), chr(charr)
    {
        set_bank(0x3);
    }

    enum Mapping : u8 {
        ram_0, ram_1, ram_2, ram_3,
        chr_r, romh, none,
    };

    u8 r(const u16& addr) const { // addr is 14bits
        switch (pla[addr >> 12]) {
            case ram_0: return ram[addr];
            case ram_1: return ram[0x4000 | addr];
            case ram_2: return ram[0x8000 | addr];
            case ram_3: return ram[0xc000 | addr];
            case chr_r: return chr[0x0fff & addr];
            case romh:  return 0x00; // TODO
            case none:  return 0x00; // TODO: wut..?
        }
        return 0x00; // silence compiler...
    }

    // TODO: full mode handling (ultimax + special modes)
    void set_bank(u8 va14_va15) { pla = (u8*)PLA[va14_va15]; }

private:
    static const u8 PLA[4][4];

    const u8* pla;  // active pla line

    const u8* ram;
    const u8* chr;
    //const u8* romh;
};


template <typename Out>
class Core { // 6569 (PAL-B)
public:
    static const u8 REG_COUNT = 64;

    enum Reg : u8 {
        m0x,  m0y,  m1x,  m1y,  m2x,  m2y,  m3x,  m3y,  m4x,  m4y,  m5x,  m5y,  m6x,  m6y,  m7x,  m7y,
        mnx8, cr1,  rast, lpx,  lpy,  mne,  cr2,  mnye, mptr, ireg, ien,  mndp, mnmc, mnxe, mnm,  mnd,
        ecol, bgc0, bgc1, bgc2, bgc3, mmc0, mmc1, m0c,  m1c,  m2c,  m3c,  m4c,  m5c,  m6c,  m7c,
    };
    const u8 Unused_bits[REG_COUNT] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x00, 0x70, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    };

    enum CR1  : u8 {
        rst8_bit = 0x80, ecm_bit = 0x40, bmm_bit = 0x20, den_bit = 0x10,
        rsel_bit = 0x08, y_scroll_mask = 0x07,
    };
    enum CR2  : u8 { mcm_bit = 0x10, csel_bit = 0x08, x_scroll_mask = 0x07, };
    enum MPTR : u8 { vm_mask = 0xf0, cg_mask = 0x0e, };

    Core(
          const u8* ram_, const Color_RAM& col_ram_, const u8* charr,
          u16& ba_low, Out& out_)
        : banker(ram_, charr),
          irq(reg, out_),
          ba(ba_low),
          mobs(banker, reg, raster_y, ba, irq),
          gfx(banker, col_ram_, reg, line_cycle, raster_y, ba),
          border(beam_pos, raster_y, den, reg[ecol]),
          out(out_) { }

    Banker banker;

    void reset() { for (int r = 0; r < REG_COUNT; ++r) w(r, 0); }

    // TODO: set lp x/y (mouse event)
    void set_lp_ctrl_p1_p6(u8 low) { set_lp(lp_p1_p6_low = low); }
    void set_lp_cia1_pb_b4(u8 low) { set_lp(lp_pb_b4_low = low); }

    void r(const u8& ri, u8& data) {
        switch (ri) {
            case cr1:
                data = (reg[cr1] & ~rst8_bit) | ((raster_y & 0x100) >> 1);
                return;
            case rast:
                data = raster_y;
                return;
            case mnm: case mnd:
                data = reg[ri];
                reg[ri] = 0;
                return;
            default:
                data = reg[ri] | Unused_bits[ri];
        }
    }

    void w(const u8& ri, const u8& data) {
        u8 d = data & ~Unused_bits[ri];
        reg[ri] = d;

        switch (ri) {
            case m0x: case m1x: case m2x: case m3x: case m4x: case m5x: case m6x: case m7x:
                mobs.mob[ri >> 1].set_x_lo(d);
                break;
            case mnx8:
                for (auto& m : mobs.mob) { m.set_x_hi(d & 0x1); d >>= 1; }
                break;
            case cr1:
                if (d & rst8_bit) cmp_raster |= 0x100; else cmp_raster &= 0x0ff;
                gfx.set_ecm(d & ecm_bit);
                gfx.set_bmm(d & bmm_bit);
                den = d & den_bit;
                gfx.set_den(den);
                border.set_rsel(d & rsel_bit);
                gfx.set_y_scroll(d & y_scroll_mask);
                break;
            case rast:
                cmp_raster = (cmp_raster & 0x100) | d;
                break;
            case cr2:
                gfx.set_mcm(d & mcm_bit);
                border.set_csel(d & CR2::csel_bit);
                gfx.x_scroll = d & x_scroll_mask;
                break;
            case mptr:
                gfx.c_base = d & MPTR::cg_mask;
                gfx.c_base <<= 10;
                gfx.vm_base = d & MPTR::vm_mask;
                gfx.vm_base <<= 6;
                mobs.set_vm_base(gfx.vm_base);
                break;
            case ireg: irq.w_ireg(d); break;
            case ien:  irq.ien_upd(); break;
            case mnye: for (auto& m : mobs.mob) { m.set_ye(d & 0x1); d >>= 1; } break;
            case mnmc: for (auto& m : mobs.mob) { m.set_mc(d & 0x1); d >>= 1; } break;
            case mnxe: for (auto& m : mobs.mob) { m.set_xe(d & 0x1); d >>= 1; } break;
            case mmc0: for (auto& m : mobs.mob) m.set_mc0(d); break;
            case mmc1: for (auto& m : mobs.mob) m.set_mc1(d); break;
            case m0c: case m1c: case m2c: case m3c: case m4c: case m5c: case m6c: case m7c:
                mobs.mob[ri - m0c].set_c(d);
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

    #if defined(__GNUC__) && (__GNUC__ >= 7)
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
    #endif

    void tick(u16& frame_cycle) {

        line_cycle = frame_cycle % LINE_CYCLES;

        switch (line_cycle) {
            case  0:
                mobs.do_dma(2);

                raster_y++;

                if (raster_y == RASTER_LINE_COUNT) {
                    out.sync_line(0);
                    raster_y = frame_cycle = 0;
                    beam_pos = frame;
                    border.frame_start();
                    return;
                }

                out.sync_line(raster_y);

                if (raster_y == cmp_raster) irq.req(IRQ::rst);

                // TODO: reveal the magic numbers....
                if (!v_blank) {
                    if (raster_y == 48) {
                        gfx.vma_start(den);
                    } else if (raster_y == 248) {
                        gfx.vma_done();
                    } else if (raster_y == (250 + BORDER_SZ_H)) {
                        v_blank = true;
                    }
                } else if (raster_y == (50 - BORDER_SZ_H)) {
                    v_blank = false;
                    frame_lp = false;
                    set_lp(lp_p1_p6_low || lp_pb_b4_low);
                }

                return;
            case  1:
                mobs.prep_dma(5);
                if (raster_y == 0 && cmp_raster == 0) irq.req(IRQ::rst);
                return;
            case  2: mobs.do_dma(3);  return;
            case  3: mobs.prep_dma(6); return;
            case  4: mobs.do_dma(4);  return;
            case  5: mobs.prep_dma(7); return;
            case  6: mobs.do_dma(5);  return;
            case  7: // 448..
                if (!v_blank) {
                    //update_mobs(); // needed??
                }
                return;
            case  8: // 452
                if (!v_blank) {
                    raster_x = 452;
                    update_mobs();
                }
                mobs.do_dma(6);
                return;
            case  9: // 460
                if (!v_blank) update_mobs();
                return;
            case 10: // 468
                if (!v_blank) update_mobs();
                mobs.do_dma(7);
                return;
            case 11: // 476
                if (!v_blank) {
                    gfx.ba_check();
                    update_mobs();
                }
                return;
            case 12: // 484
                if (!v_blank) update_mobs();
                return;
            case 13: // 492
                if (!v_blank) {
                    update_mobs();
                    output_start_1();
                    gfx.row_start();
                }
                return;
            case 14: // 500
                if (!v_blank) {
                    output_start_2();
                    gfx.read_vm();
                }
                mobs.inc_mdc_base_2();
                return;
            case 15: // 4
                if (!v_blank) {
                    output();
                    gfx.read_gd();
                    gfx.read_vm();
                }
                mobs.inc_mdc_base_1();
                return;
            case 16: // 12
                if (!v_blank) {
                    gfx.feed_gd();
                    output();
                    border.check_left(1);
                    gfx.read_gd();
                    gfx.read_vm();
                }
                return;
            case 17: // 20
                if (!v_blank) {
                    gfx.feed_gd();
                    output();
                    border.check_left(0);
                    gfx.read_gd();
                    gfx.read_vm();
                }
                return;
            case 18: case 19: // 28..
            case 20: case 21: case 22: case 23: case 24: case 25: case 26: case 27: case 28: case 29:
            case 30: case 31: case 32: case 33: case 34: case 35: case 36: case 37: case 38: case 39:
            case 40: case 41: case 42: case 43: case 44: case 45: case 46: case 47: case 48: case 49:
            case 50: case 51: case 52: case 53: // ..315
                if (!v_blank) {
                    gfx.feed_gd();
                    output();
                    gfx.read_gd();
                    gfx.read_vm();
                }
                return;
            case 54: // 316
                if (!v_blank) {
                    gfx.feed_gd();
                    output();
                    gfx.read_gd();
                    gfx.ba_done();
                }
                mobs.check_mdc_base_upd();
                mobs.check_dma();
                mobs.prep_dma(0);
                return;
            case 55: // 324
                if (!v_blank) {
                    gfx.feed_gd();
                    output();
                    border.check_right(0);
                }
                mobs.check_dma();
                mobs.prep_dma(0);
                return;
            case 56: // 332
                if (!v_blank) {
                    output();
                    border.check_right(1);
                }
                mobs.prep_dma(1);
                return;
            case 57: // 340
                if (!v_blank) {
                    output();
                    gfx.row_done();
                }
                mobs.load_mdc();
                return;
            case 58: // 348
                if (!v_blank) output_end_1();
                mobs.prep_dma(2);
                return;
            case 59: // 356
                if (!v_blank) output_end_1();
                mobs.do_dma(0);
                return;
            case 60: // 364
                if (!v_blank) output_end_2();
                mobs.prep_dma(3);
                return;
            case 61: mobs.do_dma(1); return;
            case 62:
                border.line_done();
                mobs.prep_dma(4);
                return;
        }
    }

    #if defined(__GNUC__) && (__GNUC__ >= 7)
        #pragma GCC diagnostic pop
    #endif

private:
    class IRQ {
    public:
        enum Ireg : u8 {
            rst = 0x01, mdc = 0x02, mmc = 0x04, lp = 0x08,
            irq = 0x80,
        };

        IRQ(u8* reg_, Out& out_) : reg(reg_), out(out_) { }

        void w_ireg(u8 data) { reg[ireg] &= ~data; update(); }
        void ien_upd()       { update(); }
        void req(u8 kind)    { reg[ireg] |= kind;  update(); }

    private:
        void update() {
            if (reg[ireg] & reg[ien]) {
                reg[ireg] |= Ireg::irq;
                out.set_irq();
            } else {
                reg[ireg] &= ~Ireg::irq;
                out.clr_irq();
            }
        }

        u8* reg;
        Out& out;
    };


    class BA {
    public:
        BA(u16& ba_low_) : ba_low(ba_low_) {}

        void mob_start(u8 mn)  { ba_low |=  (0x100 << mn); }
        void mob_done(u8 mn)   { ba_low &= ~(0x100 << mn); }
        void gfx_set(u8 b)     { ba_low = (ba_low & 0xff00) | b; }
        void gfx_rel()         { gfx_set(0); }
    private:
        u16& ba_low; // 8 MSBs for MOBs, 8 LSBs for GFX
    };


    class MOBs {
    public:
        static const u8  mob_count = 8;
        static const u16 MP_BASE   = 0x3f8; // offset from vm_base

        class MOB {
        public:
            static const u8 transparent_px = 0xff;

            // flags (TODO: these 4 into one u8...?)
            u8 ye;
            u8 dma_on = false;
            u8 mdc_base_upd;

            u8 mdc_base;
            u8 mdc;

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

        private:
            const u32 pristine_data  = 0x00000003;
            const u32 pristine_flip  = 0x00000002;
            const u32 out_of_data    = 0x01000000;

            const u32 pixel_mask_sc  = 0x80000000;
            const u32 pixel_mask_mc  = 0xc0000000;

            u16 x;

            u32 data = out_of_data;
            u32 pixel_mask;

            u8 shift_base_delay;
            u8 shift_amount;
            u8 shift_delay;
            u8 shift_timer  = 0;

            u8 col[4] = { transparent_px, 0, 0, 0 };
        };


        MOB mob[mob_count];

        void set_vm_base(u16 vm_base) { mp_base = vm_base | MP_BASE; }

        void check_mdc_base_upd() { for(auto&m : mob) if (m.ye) m.mdc_base_upd = !m.mdc_base_upd; }
        void check_dma() {
            for (u8 mn = 0, mb = 0x01; mn < mob_count; ++mn, mb <<= 1) {
                MOB& m = mob[mn];
                if (!m.dma_on) {
                    if ((reg[mne] & mb) && (reg[m0y + (mn * 2)] == (raster_y & 0xff))) {
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
                u16 mp = bank.r(mp_base | mn);
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
                if (reg[mnm] == 0x00) irq.req(IRQ::mmc);
                reg[mnm] |= mmc_total;
            }
        }

        MOBs(
              Banker& bank_, u8* reg_, const u16& raster_y_,
              BA& ba_, IRQ& irq_)
            : bank(bank_), reg(reg_), raster_y(raster_y_),
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
                // priority: mob|gfx_fg (based on reg[mndp]) > mob > gfx_bg
                if (gfx_fg) {
                    if (reg[mnd] == 0x00) irq.req(IRQ::mdc);
                    reg[mnd] |= mdc;

                    if (!(reg[mndp] & src_mb)) *to = col;
                } else {
                    *to = col;
                }

                if (mmc_on) {
                    if (reg[mnm] == 0x00) irq.req(IRQ::mmc);
                    reg[mnm] |= mmc;
                }
            }
        }

        u16 mp_base;

        Banker& bank;
        u8* reg;
        const u16& raster_y;
        BA& ba;
        IRQ& irq;
    };


    class GFX {
    public:
        static const u8 foreground = 0x80;

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

        u8 x_scroll;
        u8 y_scroll;
        u16 vm_base;
        u16 c_base;

        GFX(
            Banker& bank_, const Color_RAM& col_ram_, const u8* reg_,
            const u8& line_cycle_, const u16& raster_y_, BA& ba_)
          : bank(bank_), col_ram(col_ram_), reg(reg_),
            line_cycle(line_cycle_), raster_y(raster_y_), ba(ba_) {}

        void set_ecm(bool e)   { mode = e ? mode | ecm_set : mode & ~ecm_set; }
        void set_bmm(bool b)   { mode = b ? mode | bmm_set : mode & ~bmm_set; }
        void set_mcm(bool m)   { mode = m ? mode | mcm_set : mode & ~mcm_set; }
        void set_act(bool m)   { mode = m ? mode | act_set : mode & ~act_set; }

        // called if cr1.den is modified (interesting only if it happens during VMA_TOP_RASTER_Y)
        void set_den(u8 den) {
            ba_area |= ((raster_y == vma_top_raster_y) && den);
            ba_check();
        }

        void set_y_scroll(u8 y_scroll_) {
            y_scroll = y_scroll_;
            ba_check();
            if (!ba_line) ba.gfx_rel();
        }

        void vma_start(u8 den) { ba_area = den; vc_base = 0; }
        void vma_done()        { ba_area = ba_line = false;  }

        void ba_check() {
            if (ba_area) {
                ba_line = (raster_y & y_scroll_mask) == y_scroll;
                if (ba_line) {
                    if (line_cycle > 10 && line_cycle < 54) {
                        ba.gfx_set(line_cycle | 0x80); // store cycle for AEC checking later
                        if (line_cycle < 14) activate();
                        // else activate() delayed (called from read_vm())
                    } else {
                        activate();
                    }
                }
            }
        }

        void ba_done() { ba.gfx_rel(); }

        void row_start() {
            vc = vc_base;
            vmri = 1 + active();
            vmoi = vmri - 1; // if idle, we stay at index zero (with zeroed data)
            vm_row[1] = vm_row[41]; // wrap around (make last one linger...)
            if (ba_line) rc = 0;
        }

        void row_done() {
            if (rc == 7) {
                vc_base = vc;
                if (!ba_line) {
                    deactivate();
                    return;
                }
			}
            rc = (rc + active()) & 0x7;
        }

        void read_vm() {
            if (ba_line) {
                activate(); // delayed (see ba_check())
                col_ram.r(vc, vm_row[vmri].cr_data); // TODO: mask upper bits if 'noise' is implemented
                vm_row[vmri].vm_data = bank.r(vm_base | vc);
            }
        }

        void read_gd() {
            gdr = bank.r(gfx_addr);
            vc = (vc + active()) & 0x3ff;
            vmri += active();
        }

        void feed_gd() {
            gd |= (gdr << (20 - x_scroll));
            vb |= (0x10 << x_scroll);
        }

        void output(u8* to) {
            u8 bumps = 8;
            const auto put = [&to](const u8 px) { *to++ = px; };
            const auto bump = [&bumps, this]() {
                gd <<= 1;
                vb >>= 1;
                vmoi += (vb & active());
                return --bumps;
            };

            switch (mode) {
                case scm_i:
                    do put(gd & 0x80000000 ? 0 | foreground : reg[bgc0]); while (bump());
                    gfx_addr = addr_idle;
                    return;

                case scm:
                    do put(gd & 0x80000000
                            ? vm_row[vmoi].cr_data | foreground
                            : reg[bgc0]);
                    while (bump());
                    gfx_addr = c_base | (vm_row[vmri].vm_data << 3) | rc;
                    return;

                case mccm_i:
                    do put(gd & 0x80000000 ? 0 | foreground : reg[bgc0]); while (bump());
                    gfx_addr = addr_idle;
                    return;

                case mccm: {
                    u8 aligned;
                    if (vm_row[vmoi].cr_data & multicolor) {
                        aligned = (x_scroll ^ 0x1) & 0x1;
                        do {
                            if (aligned) {
                                u8 g = (gd >> 24) & 0xc0;
                                u8 c = (g == 0xc0)
                                    ? vm_row[vmoi].cr_data & 0x7
                                    : reg[bgc0 + (g >> 6)];
                                col = (c | g); // sets also fg-gfx flag
                            }
                            put(col);
                            gd <<= 1; vb >>= 1;
                            if (vb & 0x1) {
                                ++vmoi;
                                if (!(vm_row[vmoi].cr_data & multicolor)) goto _scm;
                            }
                            aligned ^= 0x1;
                            _mccm:
                            --bumps;
                        } while (bumps);
                    } else {
                        do {
                            put(gd & 0x80000000
                                ? vm_row[vmoi].cr_data | foreground
                                : reg[bgc0]);
                            gd <<= 1; vb >>= 1;
                            if (vb & 0x1) {
                                ++vmoi;
                                if (vm_row[vmoi].cr_data & multicolor) {
                                    aligned = 0x1;
                                    goto _mccm;
                                }
                            }
                            _scm:
                            --bumps;
                        } while (bumps);
                    }
                    gfx_addr = c_base | (vm_row[vmri].vm_data << 3) | rc;
                    return;
                }

                case sbmm_i:
                    do put(gd & 0x80000000 ? 0 | foreground : 0); while (bump());
                    gfx_addr = addr_idle;
                    return;

                case sbmm:
                    do put(gd & 0x80000000
                            ? (vm_row[vmoi].vm_data >> 4) | foreground
                            : (vm_row[vmoi].vm_data & 0xf));
                    while (bump());
                    gfx_addr = (c_base & 0x2000) | (vc << 3) | rc;
                    return;

                case mcbmm_i: {
                    u8 aligned = (x_scroll ^ 0x1) & 0x1;
                    do {
                        if (aligned) {
                            switch (gd >> 30) {
                                case 0x0: col = reg[bgc0];      break;
                                case 0x1: col = 0;              break;
                                case 0x2: col = 0 | foreground; break;
                                case 0x3: col = 0 | foreground; break;
                            }
                        }
                        put(col);
                        aligned ^= 0x1;
                    } while (bump());
                    gfx_addr = addr_idle;
                    return;
                }

                case mcbmm: {
                    u8 aligned = (x_scroll ^ 0x1) & 0x1;
                    do {
                        if (aligned) {
                            const auto& vr = vm_row[vmoi];
                            switch (gd >> 30) {
                                case 0x0: col = reg[bgc0];                       break;
                                case 0x1: col = (vr.vm_data >> 4);               break;
                                case 0x2: col = (vr.vm_data & 0xf) | foreground; break;
                                case 0x3: col = (vr.cr_data) | foreground;       break;
                            }
                        }
                        put(col);
                        aligned ^= 0x1;
                    } while (bump());
                    gfx_addr = (c_base & 0x2000) | (vc << 3) | rc;
                    return;
                }

                case ecm_i:
                    do put(gd & 0x80000000 ? 0 | foreground : reg[bgc0]); while (bump());
                    gfx_addr = addr_idle & addr_ecm_mask;
                    return;

                case ecm:
                    do put(gd & 0x80000000
                            ? vm_row[vmoi].cr_data | foreground
                            : reg[bgc0 + (vm_row[vmoi].vm_data >> 6)]);
                    while (bump());
                    gfx_addr = (c_base | (vm_row[vmri].vm_data << 3) | rc) & addr_ecm_mask;
                    return;

                case icm_i: case icm: {
                    u8 aligned;
                    if (vm_row[vmoi].cr_data & multicolor) {
                        aligned = (x_scroll ^ 0x1) & 0x1;
                        do {
                            // sets col.0 & fg-gfx flag
                            if (aligned) col = (gd >> 24) & 0x80;
                            put(col);
                            gd <<= 1; vb >>= 1;
                            if (vb & 0x1) {
                                ++vmoi;
                                if (!(vm_row[vmoi].cr_data & multicolor)) goto _icm_scm;
                            }
                            aligned ^= 0x1;
                            _icm_mccm:
                            --bumps;
                        } while (bumps);
                    } else {
                        do {
                            put((gd >> 24) & 0x80); // sets col.0 & fg-gfx flag
                            gd <<= 1; vb >>= 1;
                            if (vb & 0x1) {
                                ++vmoi;
                                if (vm_row[vmoi].cr_data & multicolor) {
                                    aligned = 0x1;
                                    goto _icm_mccm;
                                }
                            }
                            _icm_scm:
                            --bumps;
                        } while (bumps);
                    }
                    gfx_addr = (mode == icm_i
                                    ? addr_idle
                                    : (c_base | (vm_row[vmri].vm_data << 3) | rc))
                                            & addr_ecm_mask;
                    return;
                }

                case ibmm1_i: case ibmm1:
                    do put((gd >> 24) & 0x80); while (bump()); // sets col.0 & fg-gfx flag
                    gfx_addr = (mode == ibmm2_i
                                    ? addr_idle
                                    : ((c_base & 0x2000) | (vc << 3) | rc)) & addr_ecm_mask;
                    return;

                case ibmm2_i: case ibmm2: {
                    u8 aligned = (x_scroll ^ 0x1) & 0x1;
                    do {
                        // sets col.0 & fg-gfx flag
                        if (aligned) col = (gd >> 24) & 0x80;
                        put(col);
                        aligned ^= 0x1;
                    } while (bump());
                    gfx_addr = (mode == ibmm2_i
                                    ? addr_idle
                                    : ((c_base & 0x2000) | (vc << 3) | rc)) & addr_ecm_mask;
                    return;
                }
            }
        }

        void output_border(u8* to) {
            const auto put = [&to](const u8 px) {
                to[0] = to[1] = to[2] = to[3] = to[4] = to[5] = to[6] = to[7] = px;
            };

            switch (mode) {
                case scm_i: case scm: case mccm_i: case mccm:
                    put(reg[bgc0]);
                    return;
                case sbmm_i: case sbmm:
                    put(vm_row[vmoi].vm_data & 0xf);
                    return;
                case mcbmm_i: case mcbmm:
                    put(reg[bgc0]);
                    return;
                case ecm_i: case ecm:
                    put(reg[bgc0 + (vm_row[vmoi].vm_data >> 6)]);
                    return;
                case icm_i: case icm: case ibmm1_i: case ibmm1: case ibmm2_i: case ibmm2:
                    put(0x0);
                    return;
            }
        }

    private:
        static const u16 vma_top_raster_y = 48;
        static const u8  multicolor       = 0x8;
        static const u16 addr_idle        = 0x3fff;
        static const u16 addr_ecm_mask    = 0x39ff;

        void activate()     { set_act(true);  }
        void deactivate()   { set_act(false); }
        bool active() const { return mode & Mode_bit::act_set; }

        u8 ba_area = false; // set for each frame (at the top of disp.win)
        u8 ba_line = false;

        struct VM_data { // video-matrix data
            u8 vm_data = 0;
            u8 cr_data = 0; // color-ram data
        };
        VM_data vm_row[42]; // vm row buffer

        u16 vc_base;
        u16 vc;
        u16 rc;
        u16 gfx_addr;
        u32 gd;
        u8  gdr;

        u16 vb;   // vmoi bump timer
        u8  vmri; // vm_row read index
        u8  vmoi; // vm_row output index
        u8  col;

        u8 mode = scm_i;

        Banker& bank;
        const Color_RAM& col_ram;
        const u8* reg;
        const u8& line_cycle;
        const u16& raster_y;
        BA& ba;
    };


    // TODO: timing might be off by ~4px (see sprsync.prg)
    class Border {
    public:
        Border(
            u8*& beam_pos_, const u16& raster_y_, const u8& den_, const u8& ecol_)
         :  beam_pos(beam_pos_), raster_y(raster_y_), den(den_), ecol(ecol_) {}

        void set_rsel(u8 rs) {
            cmp_top = 55 - (rs >> 1);
            cmp_bottom = 247 + (rs >> 1);
        }

        void set_csel(bool cs) { csel = cs; }

        void output(u8* to) {
            if (on_at) {
                if (off_at) {
                    while (on_at < off_at) *on_at++ = ecol;
                    on_at = nullptr;
                } else {
                    while (on_at < to) *on_at++ = ecol;
                }
            }
        }

        void check_left(u8 cmp_csel) {
            if (csel == cmp_csel) {
                if (raster_y == cmp_bottom) locked = true;
                else if (raster_y == cmp_top && den) locked = false;
                if (!locked && on_at) off_at = beam_pos + (3 + csel);
            }
        }

        void check_right(u8 cmp_csel) {
            if (on_at) return;
            if (csel == cmp_csel) {
                on_at = beam_pos + (3 + csel);
                off_at = nullptr;
            }
        }

        void line_done() {
            if (raster_y == cmp_bottom) locked = true;
            else if (raster_y == cmp_top && den) locked = false;
        }

        void frame_start() { if (on_at) on_at = beam_pos; }

    private:
        u8*& beam_pos;
        const u16& raster_y;
        const u8& den;  // reg.cr1.den
        const u8& ecol; // reg.ec

        // indicate beam_pos at which border should start/end
        u8* on_at = nullptr;
        u8* off_at = nullptr;

        u8 locked; // can be turned off if true

        u8 csel; // cr2.csel (0|1 ==> left: 31|24 - right: 335|344)

        // cr1.rsel
        u16 cmp_top;
        u16 cmp_bottom;
    };

    void set_lp(u8 low) {
        // TODO: need to adjust the x for CIA originated ones?
        if (low && !frame_lp) {
            frame_lp = true;
            reg[lpx] = raster_x >> 1;
            reg[lpy] = raster_y;
            irq.req(IRQ::lp);
        }
    }

    void update_mobs() {
        mobs.update(raster_x, raster_x + 8);
        raster_x += 8;
    }

    void output_start_1() {
        gfx.output_border(beam_pos);
    }

    void output_start_2() {
        mobs.output(500, 504, beam_pos);
        mobs.output(0, 4, beam_pos + 4);
        beam_pos += 8;
        raster_x = 4;
        border.output(beam_pos);
        gfx.output_border(beam_pos); // gfx.output(beam_pos);
    }

    void output() {
        mobs.output(raster_x, raster_x + 8, beam_pos);
        beam_pos += 8;
        raster_x += 8;
        border.output(beam_pos);
        gfx.output(beam_pos);
    }

    void output_end_1() {
        mobs.output(raster_x, raster_x + 8, beam_pos);
        beam_pos += 8;
        raster_x += 8;
        border.output(beam_pos);
        gfx.output_border(beam_pos);
    }

    void output_end_2() {
        mobs.output(raster_x, raster_x + 8, beam_pos);
        beam_pos += 8;
        raster_x += 8;
        border.output(beam_pos);
    }

    IRQ irq;
    BA ba;
    MOBs mobs;
    GFX gfx;
    Border border;

    u8 reg[REG_COUNT];

    u8 line_cycle = 0;
    u8 frame_lp;
    u8 lp_p1_p6_low = 0;
    u8 lp_pb_b4_low = 0;
    u16 raster_x;
    u16 raster_y = 0;
    u8 v_blank = true;

    u8 den;

    u16 cmp_raster;

public:
    u8 frame[VIEW_WIDTH * VIEW_HEIGHT] = {};
private:
    u8* beam_pos = frame;

    Out& out;
};


} // namespace VIC_II

#endif // VIC_II_H_INCLUDED
