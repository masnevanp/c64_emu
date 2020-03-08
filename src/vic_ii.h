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
static const u16 BORDER_SZ_V       = 32;
static const u16 BORDER_SZ_H       = 20;

static const u16 VIEW_WIDTH        = 320 + 2 * BORDER_SZ_V;
static const u16 VIEW_HEIGHT       = 200 + 2 * BORDER_SZ_H;

static const u16 RASTER_LINE_COUNT = 312;
static const u8  LINE_CYCLES       =  63;


class Color_RAM {
public:
    static const u16 SIZE = 0x0400;

    void r(const u16& addr, u8& data) const { data = ram[addr]; }
    void w(const u16& addr, const u8& data) { ram[addr] = data & 0xf; } // masking the write is enough

private:
    u8 ram[SIZE] = {}; // just the lower nibble is actually used

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

    static const u8 TRANSPARENT_PX = 0xff;


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
          irq_unit(reg, out_),
          ba_unit(ba_low),
          mob_unit(banker, col_ram_, reg, raster_y, ba_unit, irq_unit),
          gfx_unit(banker, col_ram_, reg, line_cycle, raster_y, ba_unit),
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
                mob_unit.mob[ri >> 1].set_x_lo(d);
                break;
            case mnx8:
                for (auto& m : mob_unit.mob) { m.set_x_hi(d & 0x1); d >>= 1; }
                break;
            case cr1:
                if (d & rst8_bit) cmp_raster |= 0x100; else cmp_raster &= 0x0ff;
                gfx_unit.set_ecm(d & ecm_bit);
                gfx_unit.set_bmm(d & bmm_bit);
                den = d & den_bit;
                gfx_unit.set_den(den);
                set_rsel(d & rsel_bit);
                gfx_unit.set_y_scroll(d & y_scroll_mask);
                break;
            case rast:
                cmp_raster = (cmp_raster & 0x100) | d;
                break;
            case cr2:
                gfx_unit.set_mcm(d & mcm_bit);
                set_csel(d & CR2::csel_bit);
                gfx_unit.set_x_scroll(d & x_scroll_mask);
                break;
            case mptr:
                gfx_unit.c_base = d & MPTR::cg_mask;
                gfx_unit.c_base <<= 10;
                gfx_unit.vm_base = d & MPTR::vm_mask;
                gfx_unit.vm_base <<= 6;
                mob_unit.set_vm_base(gfx_unit.vm_base);
                break;
            case ireg: irq_unit.w_ireg(d); break;
            case ien:  irq_unit.ien_upd(); break;
            case mnye: for (auto& m : mob_unit.mob) { m.set_ye(d & 0x1); d >>= 1; } break;
            case mnmc: for (auto& m : mob_unit.mob) { m.set_mc(d & 0x1); d >>= 1; } break;
            case mnxe: for (auto& m : mob_unit.mob) { m.set_xe(d & 0x1); d >>= 1; } break;
            case mmc0: for (auto& m : mob_unit.mob) m.set_mc0(d); break;
            case mmc1: for (auto& m : mob_unit.mob) m.set_mc1(d); break;
            case m0c: case m1c: case m2c: case m3c: case m4c: case m5c: case m6c: case m7c:
                mob_unit.mob[ri - m0c].set_c(d);
                break;
        }
    }

#if defined(__GNUC__) && (__GNUC__ >= 7)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#endif

    /* NOTE:
        mob_unit.pre_dma/do_dma pair is done, so that it takes the specified
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

        line_cycle = frame_cycle % LINE_CYCLES;

        switch (line_cycle) {
            case  0:
                mob_unit.do_dma(2);

                raster_y++;

                if (raster_y == RASTER_LINE_COUNT) {
                    out.sync_line(0, true);
                    raster_y = frame_cycle = 0;
                    return;
                }

                out.sync_line(raster_y, false);

                if (raster_y == cmp_raster) irq_unit.req(IRQ_unit::rst);

                if ((raster_y % 2) == 0) {
                    // TODO: reveal the magic numbers....
                    if (raster_y == (50 - BORDER_SZ_H)) {
                        v_blank = false;
                        frame_lp = 0;
                        set_lp(lp_p1_p6_low || lp_pb_b4_low);
                    } else if (raster_y == 48) {
                        gfx_unit.vma_start(den);
                    } else if (raster_y == 248) {
                        gfx_unit.vma_end();
                    } else if (raster_y == (250 + BORDER_SZ_H)) {
                        v_blank = true;
                    }
                }

                gfx_unit.ba_check();

                return;
            case  1:
                mob_unit.pre_dma(5);
                if (raster_y == 0 && cmp_raster == 0) irq_unit.req(IRQ_unit::rst);
                return;
            case  2: mob_unit.do_dma(3);  return;
            case  3: mob_unit.pre_dma(6); return;
            case  4: mob_unit.do_dma(4);  return;
            case  5: mob_unit.pre_dma(7); return;
            case  6: mob_unit.do_dma(5);  return;
            case  7: // 448..
                if (!v_blank) {
                    raster_x = 448;
                    update_mobs();
                }
                return;
            case  8:
                if (!v_blank) update_mobs();
                mob_unit.do_dma(6);
                return;
            case  9: if (!v_blank) update_mobs(); return;
            case 10:
                if (!v_blank) update_mobs();
                mob_unit.do_dma(7);
                return;
            case 11: gfx_unit.ba_check(); // fall through
            case 12: if (!v_blank) update_mobs(); return;
            case 13: // 496..503
                if (!v_blank) {
                    gfx_unit.gfx_border_read();
                    output_border();
                    raster_x = 0;
                    gfx_unit.row_start();
                }
                return;
            case 14: // 0
                if (!v_blank) {
                    gfx_unit.gfx_border_read();
                    output_border();
                    gfx_unit.vm_read();
                }
                mob_unit.inc_mdc_base_2();
                return;
            case 15: // 8
                if (!v_blank) {
                    gfx_unit.gfx_border_read();
                    output_border();
                    gfx_unit.gfx_pad_left();
                    gfx_unit.gfx_read();
                    gfx_unit.vm_read();
                }
                mob_unit.inc_mdc_base_1();
                return;
            case 16: // 16
                if (!v_blank) {
                    gfx_unit.gfx_border_read();
                    output_border();
                    gfx_unit.gfx_read();
                    gfx_unit.vm_read();
                }
                return;
            case 17: // 24..31
                if (!v_blank) {
                    output_on_left_edge();
                    gfx_unit.gfx_read();
                    gfx_unit.vm_read();
                }
                return;
            case 18: case 19:
            case 20: case 21: case 22: case 23: case 24: case 25: case 26: case 27: case 28: case 29:
            case 30: case 31: case 32: case 33: case 34: case 35: case 36: case 37: case 38: case 39:
            case 40: case 41: case 42: case 43: case 44: case 45: case 46: case 47: case 48: case 49:
            case 50: case 51: case 52: case 53: // 32..319
                if (!v_blank) {
                    output();
                    gfx_unit.gfx_read();
                    gfx_unit.vm_read();
                }
                return;
            case 54: // 320
                if (!v_blank) {
                    output();
                    gfx_unit.gfx_read();
                    gfx_unit.gfx_pad_right();
                    gfx_unit.ba_end();
                }
                mob_unit.check_mdc_base_upd();
                mob_unit.check_dma();
                mob_unit.pre_dma(0);
                return;
            case 55: // 328
                if (!v_blank) output_on_right_edge();
                mob_unit.check_dma();
                mob_unit.pre_dma(0);
                return;
            case 56: // 336
                if (!v_blank) output();
                mob_unit.pre_dma(1);
                return;
            case 57: // 344
                if (!v_blank) {
                    output_on_right_edge();
                    gfx_unit.row_end();
                }
                mob_unit.load_mdc();
                return;
            case 58: // 352
                if (!v_blank) {
                    gfx_unit.gfx_border_read();
                    output_border();
                }
                mob_unit.pre_dma(2);
                return;
            case 59: // 360
                if (!v_blank) {
                    gfx_unit.gfx_border_read();
                    output_border();
                }
                mob_unit.do_dma(0);
                return;
            case 60: // 368
                if (!v_blank) {
                    gfx_unit.gfx_border_read();
                    output_border();
                }
                mob_unit.pre_dma(3);
                return;
            case 61: mob_unit.do_dma(1);  return;
            case 62:
                check_hb();
                mob_unit.pre_dma(4);
                return;
        }
    }

#if defined(__GNUC__) && (__GNUC__ >= 7)
#pragma GCC diagnostic pop
#endif

private:
    class IRQ_unit {
    public:
        enum Ireg : u8 {
            rst = 0x01, mdc = 0x02, mmc = 0x04, lp = 0x08,
            irq = 0x80,
        };

        IRQ_unit(u8* reg_, Out& out_) : reg(reg_), out(out_) { }

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


    class BA_unit {
    public:
        BA_unit(u16& ba_low_) : ba_low(ba_low_) {}

        void mob_start(u8 mn)  { ba_low |=  (0x100 << mn); }
        void mob_done(u8 mn)   { ba_low &= ~(0x100 << mn); }
        void gfx_set(u8 b)     { ba_low = (ba_low & 0xff00) | b; }
        void gfx_rel()         { gfx_set(0); }
    private:
        u16& ba_low; // 8 MSBs for MOBs, 8 LSBs for GFX
    };


    class MOB_unit {
    public:
        static const u8  MOB_COUNT = 8;
        static const u16 MP_BASE   = 0x3f8; // offset from vm_base

        class MOB {
        public:
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
                pixel_mask = mc ? PIXEL_MASK_MC : PIXEL_MASK_SC;
                shift_amount = mc ? 2 : 1;
            }
            void set_xe(bool xe)  { shift_base_delay = xe ? 2 : 1; }
            void set_c(u8 c)      { col[2] = c; }
            void set_mc0(u8 mc0)  { col[1] = mc0; }
            void set_mc1(u8 mc1)  { col[3] = mc1; }

            void load_data(u32 d1, u32 d2, u32 d3) {
                data = (d1 << 24) | (d2 << 16) | (d3 << 8) | PRISTINE_DATA;
                shift_delay = shift_base_delay * shift_amount;
            }

            u8 pixel_out(u16 px) {
                if (data == OUT_OF_DATA) return TRANSPARENT_PX;
                if ((data & PRISTINE_DATA) == PRISTINE_DATA) {
                    if (px != x) return TRANSPARENT_PX;
                    data ^= PRISTINE_FLIP;
                }
                u8 p_col = col[(data & pixel_mask) >> 30];
                if (++shift_timer == shift_delay) {
                    data <<= shift_amount;
                    shift_timer = 0;
                }
                return p_col;
            }

        private:
            const u32 PRISTINE_DATA  = 0x00000003;
            const u32 PRISTINE_FLIP  = 0x00000002;
            const u32 OUT_OF_DATA    = 0x01000000;

            const u32 PIXEL_MASK_SC  = 0x80000000;
            const u32 PIXEL_MASK_MC  = 0xc0000000;

            u16 x;

            u32 data = OUT_OF_DATA;
            u32 pixel_mask;

            u8 shift_base_delay;
            u8 shift_amount;
            u8 shift_delay;
            u8 shift_timer  = 0;

            u8 col[4] = { TRANSPARENT_PX, 0, 0, 0 };
        };


        MOB mob[MOB_COUNT];

        void set_vm_base(u16 vm_base) { mp_base = vm_base | MP_BASE; }

        void check_mdc_base_upd() { for(auto&m : mob) if (m.ye) m.mdc_base_upd = !m.mdc_base_upd; }
        void check_dma() {
            for (u8 mn = 0, mb = 0x01; mn < 8; ++mn, mb <<= 1) {
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
        void pre_dma(u8 mn) { if (mob[mn].dma_on) ba.mob_start(mn); }
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

        u8 pixel_out( // also does collision detection
            u16 x,
            bool gfx_fg, // gfx_fg at this pixel position (for collision detection)
            u8& src_mb) // if 'non-transparent' returned, the source mob can be found here (bit set)
        {
            u8 mdc = 0x00; // mob-gfx colliders
            u8 mmc = 0x00; // mob-mob colliders
            bool mmc_on = false; // mob-mob coll. happened
            u8 col = TRANSPARENT_PX;
            u8 mn = MOB_COUNT;
            u8 mb = 0x80; // mob bit ('id')

            do {
                MOB& m = mob[--mn];
                u8 m_col = m.pixel_out(x);
                if (m_col != TRANSPARENT_PX) {
                    col = m_col;
                    src_mb = mb;

                    if (gfx_fg) mdc |= mb;

                    if (mmc != 0x00) mmc_on = true;
                    mmc |= mb;
                }
                mb >>= 1;
            } while (mn > 0);

            if (mdc) {
                if (reg[mnd] == 0x00) irq_unit.req(IRQ_unit::mdc);
                reg[mnd] |= mdc;
            }

            if (mmc_on) {
                if (reg[mnm] == 0x00) irq_unit.req(IRQ_unit::mmc);
                reg[mnm] |= mmc;
            }

            return col;
        }

        void update(u16 x, u16 x_stop) { // update during h-blank
            u8 mmc_total = 0x00; // mob-mob colliders

            for (; x < x_stop; ++x) {
                bool mmc_on = false; // mob-mob coll. happened
                u8 mmc = 0x00; // mob-mob colliders (for single pixel)
                u8 mn = MOB_COUNT;
                u8 mb = 0x80; // mob bit ('id')

                do {
                    MOB& m = mob[--mn];
                    if (m.pixel_out(x) != TRANSPARENT_PX) {
                        if (mmc != 0x00) mmc_on = true;
                        mmc |= mb;
                    }
                    mb >>= 1;
                } while (mn > 0);

                if (mmc_on) mmc_total |= mmc;
            }

            if (mmc_total) {
                if (reg[mnm] == 0x00) irq_unit.req(IRQ_unit::mmc);
                reg[mnm] |= mmc_total;
            }
        }

        MOB_unit(
              Banker& bank_, const Color_RAM& col_ram_, u8* reg_, const u16& raster_y_,
              BA_unit& ba_, IRQ_unit& irq_unit_)
            : bank(bank_), col_ram(col_ram_), reg(reg_), raster_y(raster_y_),
              ba(ba_), irq_unit(irq_unit_) {}

    private:
        u16 mp_base;

        Banker& bank;
        const Color_RAM& col_ram;
        u8* reg;
        const u16& raster_y;
        BA_unit& ba;
        IRQ_unit& irq_unit;
    };


    class GFX_unit {
    public:
        static const u16 VMA_TOP_RASTER_Y = 48;
        static const u8  FG_GFX_FLAG = 0x80;
        static const u8  BG_COL_FLAG = 0x20; // works out since 'bgc0..bgc3' --> '0x21..0x24'
        static const u8  BG_COL_MASK = 0x27;

        enum Mode_bit : u8 {
            ecm_set = 0x4, bmm_set = 0x2, mcm_set = 0x1, not_set = 0x0
        };
        enum Mode : u8 {
        /*  mode  = ECM-bit | BMM-bit | MCM-bit  */
            scm   = not_set | not_set | not_set, // standard character mode
            mccm  = not_set | not_set | mcm_set, // multi-color character mode
            sbmm  = not_set | bmm_set | not_set, // standard bit map mode
            mcbmm = not_set | bmm_set | mcm_set, // multi-color bit map mode
            ecm   = ecm_set | not_set | not_set, // extended color mode
            icm   = ecm_set | not_set | mcm_set, // invalid character mode
            ibmm1 = ecm_set | bmm_set | not_set, // invalid bit map mode 1
            ibmm2 = ecm_set | bmm_set | mcm_set, // invalid bit map mode 2
        };

        u8 y_scroll;
        u16 vm_base;
        u16 c_base;

        GFX_unit(
            Banker& bank_, const Color_RAM& col_ram_, const u8* reg_,
            const u8& line_cycle_, const u16& raster_y_, BA_unit& ba_)
          : bank(bank_), col_ram(col_ram_), reg(reg_),
            line_cycle(line_cycle_), raster_y(raster_y_), ba(ba_) {}

        void set_ecm(bool e)   { mode = e ? mode | ecm_set : mode & ~ecm_set; }
        void set_bmm(bool b)   { mode = b ? mode | bmm_set : mode & ~bmm_set; }
        void set_mcm(bool m)   { mode = m ? mode | mcm_set : mode & ~mcm_set; }

        // called if cr1.den is modified (interesting only if it happens during VMA_TOP_RASTER_Y)
        void set_den(bool den) {
            ba_area |= ((raster_y == VMA_TOP_RASTER_Y) && den);
            ba_check();
        }

        void set_x_scroll(u8 x_scroll_) {
            if (x_scroll > x_scroll_) g_out_idx = g_out_idx + x_scroll - x_scroll_;
            x_scroll = x_scroll_;
        }

        void set_y_scroll(u8 y_scroll_) {
            y_scroll = y_scroll_;
            ba_check();
            if (!ba_line) ba.gfx_rel();
        }

        void vma_start(bool den) { ba_area = den; vc_base = 0; }
        void vma_end()           { ba_area = ba_line = false;  }

        void ba_check() {
            if (ba_area) {
                ba_line = (raster_y & y_scroll_mask) == y_scroll;
                if (ba_line) {
                    if (line_cycle > 10 && line_cycle < 54) {
                        ba.gfx_set(line_cycle | 0x80); // store cycle for AEC checking later
                        if (line_cycle < 14) activate_gfx();
                        // else activate_gfx() delayed (called from vm_read())
                    } else {
                        activate_gfx();
                    }
                }
            }
        }
        void ba_end() { ba.gfx_rel(); }

        void row_start() {
            vc = vc_base;
            vmri = 0;
            if (ba_line) rc = 0;
        }
        void row_end() {
            if (rc == 7) {
                vc_base = vc;
                if (!ba_line) {
                    deactivate_gfx();
                    return;
                }
			}
            if (gfx_active()) rc = (rc + 1) & 0x7;
        }

        void vm_read() {
            if (ba_line) {
                activate_gfx(); // delayed (see ba_check())
                col_ram.r(vc, vm_row[vmri].cr_data); // TODO: mask upper bits if 'noise' is implemented
                vm_row[vmri].vm_data = bank.r(vm_base | vc);
            }
        }

        void gfx_pad_left() {
            for (int i = 7 + x_scroll; i > 7; --i) g_out[i] = bgc;
            g_out_idx = 0;
        }

        void gfx_pad_right() {
            u8 start = g_out_idx + x_scroll + 16;
            u8 end = g_out_idx + 24;
            for (u8 i = start; i < end; ++i) g_out[i & 0x1f] = bgc;
        }

        void gfx_read() {
            static const u8  MC_flag         = 0x8;
            static const u16 G_ADDR_ECM_MASK = 0x39ff;

            vd = vm_row[vmri].vm_data & vm_mask;
            u8 cd = vm_row[vmri].cr_data & vm_mask;

            u16 addr;
            u8 data;
            u8 load_idx = g_out_idx + x_scroll + 8;

            switch (mode) {
                case scm: case_scm:
                    addr = c_base | (vd << 3) | rc;
                    data = bank.r(addr);
                    bgc = reg[bgc0];
                    cd |= FG_GFX_FLAG;
                    for (u8 p = 0x80; p; p >>= 1) {
                        g_out[load_idx++ & 0x1f] = data & p ? cd : bgc;
                    }
                    break;
                case mccm:
                    if (!(cd & MC_flag)) goto case_scm;
                    addr = c_base | (vd << 3) | rc;
                    data = bank.r(addr);
                    bgc = reg[bgc0];
                    for (int p = 6; p >= 0; p -= 2) {
                        auto d = (data >> p) & 0x3;
                        u8 g = (d == 0x3) ? cd ^ MC_flag : reg[bgc0 + d];
                        g |= (d << 6); // sets fg-gfx flag
                        g_out[load_idx++ & 0x1f] = g;
                        g_out[load_idx++ & 0x1f] = g;
                    }
                    break;
                case sbmm: {
                    addr = (c_base & 0x2000) | (vc << 3) | rc;
                    data = bank.r(addr);
                    u8 fgc = (vd >> 4) | FG_GFX_FLAG;
                    bgc = vd & 0xf;
                    for (u8 p = 0x80; p; p >>= 1) {
                        g_out[load_idx++ & 0x1f] = data & p ? fgc : bgc;
                    }
                    break;
                }
                case mcbmm:
                    addr = (c_base & 0x2000) | (vc << 3) | rc;
                    data = bank.r(addr);
                    bgc = reg[bgc0];
                    for (int p = 6; p >= 0; p -= 2) {
                        auto d = (data >> p) & 0x3;
                        u8 g;
                        switch (d) {
                            case 0x0: g = bgc;      break;
                            case 0x1: g = vd >> 4;  break;
                            case 0x2: g = vd & 0xf; break;
                            case 0x3: g = cd;       break;
                        }
                        g |= (d << 6); // sets fg-gfx flag
                        g_out[load_idx++ & 0x1f] = g;
                        g_out[load_idx++ & 0x1f] = g;
                    }
                    break;
                case ecm: {
                    addr = (c_base | (vd << 3) | rc) & G_ADDR_ECM_MASK;
                    data = bank.r(addr);
                    u8 fgc = cd | FG_GFX_FLAG;
                    bgc = reg[bgc0 + (vd >> 6)];
                    for (u8 p = 0x80; p; p >>= 1) {
                        g_out[load_idx++ & 0x1f] = data & p ? fgc : bgc;
                    }
                    break;
                }
                case icm:
                    addr = (c_base | (vd << 3) | rc) & G_ADDR_ECM_MASK;
                    data = bank.r(addr);
                    bgc = 0;
                    if (cd & MC_flag) {
                        for (int p = 0; p < 4; ++p) {
                            u8 g = data & 0xc0; // sets color to 0 and fg-gfx flag
                            g_out[load_idx++ & 0x1f] = g;
                            g_out[load_idx++ & 0x1f] = g;
                            data <<= 2;
                        }
                    } else {
                        for (u8 p = 0x80; p; p >>= 1) {
                            g_out[load_idx++ & 0x1f] = data & p ? (0 | FG_GFX_FLAG) : 0;
                        }
                    }
                    break;
                case ibmm1:
                    addr = ((c_base & 0x2000) | (vc << 3) | rc) & G_ADDR_ECM_MASK;
                    data = bank.r(addr);
                    bgc = 0;
                    for (u8 p = 0x80; p; p >>= 1) {
                        g_out[load_idx++ & 0x1f] = data & p ? (0 | FG_GFX_FLAG) : 0;
                    }
                    break;
                case ibmm2:
                    addr = ((c_base & 0x2000) | (vc << 3) | rc) & G_ADDR_ECM_MASK;
                    data = bank.r(addr);
                    bgc = 0;
                    for (int p = 0; p < 4; ++p) {
                        u8 g = data & 0xc0; // sets color to 0 and fg-gfx flag
                        g_out[load_idx++ & 0x1f] = g;
                        g_out[load_idx++ & 0x1f] = g;
                        data <<= 2;
                    }
                    break;
            }

            if (gfx_active()) {
                vc = (vc + 1) & 0x3ff;
                ++vmri;
            }
        }

        void gfx_border_read() {
            g_out_idx += 8; // ok to do here (moved from border_pixel_out())
            switch (mode) {
                case scm: case mccm: bgc = reg[bgc0]; break;
                case sbmm: bgc = vd & 0xf; break;
                case mcbmm: bgc = reg[bgc0]; break;
                case ecm: bgc = reg[bgc0 + (vd >> 6)]; break;
                case icm: case ibmm1: case ibmm2: bgc = 0; break;
            }
        }

        u8 pixel_out(bool& fg_gfx) {
            u8 px = g_out[g_out_idx++ & 0x1f];
            fg_gfx = px & FG_GFX_FLAG;
            return px & 0xf;
        }

        u8 border_pixel_out() const { return bgc; }

    private:
        enum VM_Mask : u8 { idle = 0x00, active = 0xff };
        static const u16 RC_IDLE = 0x3fff;

        void activate_gfx()     { vm_mask = active; rc &= 0x7; }
        void deactivate_gfx()   { vm_mask = idle;   rc = RC_IDLE; }
        bool gfx_active() const { return vm_mask; }

        u8 x_scroll;

        u8 ba_area = false; // set for each frame (at the top of disp.win)
        u8 ba_line = false;

        struct VM_data { // video-matrix data
            u8 vm_data;
            u8 cr_data; // color-ram data
        };
        VM_data vm_row[40]; // vm row buffer

        u16 vc_base;
        u16 vc;
        u16 rc = RC_IDLE;
        u8  vmri; // vm_row index
        VM_Mask vm_mask = idle;

        u8 g_out[32];
        u8 g_out_idx;
        u8 bgc;
        u8 vd;

        u8 mode = scm;

        Banker& bank;
        const Color_RAM& col_ram;
        const u8* reg;
        const u8& line_cycle;
        const u16& raster_y;
        BA_unit& ba;
    };

    void set_lp(u8 low) {
        // TODO: need to adjust the x for CIA originated ones?
        if (low && !frame_lp) {
            frame_lp = true;
            reg[lpx] = raster_x >> 1;
            reg[lpy] = raster_y;
            irq_unit.req(IRQ_unit::lp);
        }
    }

    void set_rsel(bool rs) {
        static const u16 CSEL_top[2]    = { 55, 51 };
        static const u16 CSEL_bottom[2] = { 247, 251 };
        cmp_top    = CSEL_top[rs];
        cmp_bottom = CSEL_bottom[rs];
    }
    void set_csel(bool cs) {
        static const u16 CSEL_left[2]  = { 31, 24 };
        static const u16 CSEL_right[2] = { 335, 344 };
        cmp_left  = CSEL_left[cs];
        cmp_right = CSEL_right[cs];
    }

    void check_hb() {
        if (raster_y == cmp_top && den) vert_border_on = false;
        else if (raster_y == cmp_bottom) vert_border_on = true;
    }

    void update_mobs() {
        mob_unit.update(raster_x, raster_x + 8);
        raster_x += 8;
    }

    void output() {
        if (vert_border_on) output_border();
        else for (int p = 0; p < 8; ++p) exude_pixel();
    }
    void output_border() { for (int p = 0; p < 8; ++p) exude_border_pixel(); }
    void output_on_left_edge() {
        if (raster_y == cmp_top && den) {
            for (int p = 0; p < 8; ++p) {
                if (raster_x == cmp_left) main_border_on = vert_border_on = false;
                exude_pixel(); // must 'consume' pixels (even if vert_border_on)
            }
        } else {
            for (int p = 0; p < 8; ++p) {
                if (raster_x == cmp_left) {
                    if (raster_y == cmp_bottom) vert_border_on = true;
                    else if (!vert_border_on) main_border_on = false;
                }
                if (vert_border_on) exude_border_pixel();
                else exude_pixel();
            }
        }
    }
    void output_on_right_edge() {
        for (int p = 0; p < 8; ++p) {
            if (raster_x == cmp_right) main_border_on = true;
            if (vert_border_on) exude_border_pixel();
            else exude_pixel();
        }
    }

    void exude_pixel() {
        bool gfx_fg = false;
        u8 g_col = gfx_unit.pixel_out(gfx_fg); // must call always (to keep in sync)

        u8 src_mob = 0; // if not transparet then src bit will be set here
        u8 m_col = mob_unit.pixel_out(raster_x, gfx_fg, src_mob); // must call always (to keep in sync)

        // priorites: border < mob|gfx_fg (based on reg[mndp]) < gfx_bg
        u8 o_col;
        if (main_border_on) o_col = reg[ecol];
        else if (m_col != TRANSPARENT_PX) {
            if (!gfx_fg) o_col = m_col;
            else o_col = (reg[mndp] & src_mob) ? g_col : m_col;
        }
        else o_col = g_col;

        out.put_pixel(o_col);

        ++raster_x;
    }

    void exude_border_pixel() {
        u8 _ = 0;
        u8 o_col = mob_unit.pixel_out(raster_x, false, _); // must call always (to keep in sync)

        if (main_border_on) o_col = reg[ecol];
        else if (o_col == TRANSPARENT_PX) o_col = gfx_unit.border_pixel_out();

        out.put_pixel(o_col);

        ++raster_x;
    }

    IRQ_unit irq_unit;
    BA_unit ba_unit;
    MOB_unit mob_unit;
    GFX_unit gfx_unit;

    u8 reg[REG_COUNT];

    u8 line_cycle = 0;
    u8 frame_lp;
    u8 lp_p1_p6_low = 0;
    u8 lp_pb_b4_low = 0;
    u16 raster_x;
    u16 raster_y = 0;
    u8 v_blank = true;

    bool den;
    u16 cmp_left;
    u16 cmp_right;
    u16 cmp_top;
    u16 cmp_bottom;
    u16 cmp_raster;

    bool main_border_on;
    bool vert_border_on;

    Out& out;
};


} // namespace VIC_II

#endif // VIC_II_H_INCLUDED
