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
          irq(reg, out_),
          ba(ba_low),
          mobs(banker, reg, raster_y, ba, irq),
          gfx(banker, col_ram_, reg, line_cycle, raster_y, ba),
          border(raster_y, den, reg[ecol]),
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
                border.csel = d & CR2::csel_bit;
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

#if defined(__GNUC__) && (__GNUC__ >= 7)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#endif

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

        line_cycle = frame_cycle % LINE_CYCLES;

        switch (line_cycle) {
            case  0:
                mobs.do_dma(2);

                raster_y++;

                if (raster_y == RASTER_LINE_COUNT) {
                    out.sync_line(0);
                    raster_y = frame_cycle = 0;
                    beam_pos = frame;
                    return;
                }

                out.sync_line(raster_y);

                if (raster_y == cmp_raster) irq.req(IRQ::rst);

                // TODO: reveal the magic numbers....
                if (!v_blank) {
                    if (raster_y == 48) {
                        gfx.vma_start(den);
                    } else if (raster_y == 248) {
                        gfx.vma_end();
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
                    raster_x = 448;
                    update_mobs();
                }
                return;
            case  8:
                if (!v_blank) update_mobs();
                mobs.do_dma(6);
                return;
            case  9: if (!v_blank) update_mobs(); return;
            case 10:
                if (!v_blank) update_mobs();
                mobs.do_dma(7);
                return;
            case 11:
                if (!v_blank) {
                    gfx.ba_check();
                    update_mobs();
                }
                return;
            case 12:
                if (!v_blank) {
                    output_start();
                    update_mobs();
                }
                return;
            case 13: // 496..503
                if (!v_blank) {
                    output_border();
                    raster_x = 0;
                    gfx.row_start();
                }
                return;
            case 14: // 0
                if (!v_blank) {
                    output_border();
                    gfx.read_vm();
                }
                mobs.inc_mdc_base_2();
                return;
            case 15: // 8
                if (!v_blank) {
                    output();
                    gfx.read_gd();
                    gfx.read_vm();
                }
                mobs.inc_mdc_base_1();
                return;
            case 16: // 16
                if (!v_blank) {
                    output();
                    gfx.read_gd();
                    gfx.read_vm();
                }
                return;
            case 17: // 24..31
                if (!v_blank) {
                    output_left_edge();
                    gfx.read_gd();
                    gfx.read_vm();
                }
                return;
            case 18: case 19:
            case 20: case 21: case 22: case 23: case 24: case 25: case 26: case 27: case 28: case 29:
            case 30: case 31: case 32: case 33: case 34: case 35: case 36: case 37: case 38: case 39:
            case 40: case 41: case 42: case 43: case 44: case 45: case 46: case 47: case 48: case 49:
            case 50: case 51: case 52: case 53: // 32..319
                if (!v_blank) {
                    output();
                    gfx.read_gd();
                    gfx.read_vm();
                }
                return;
            case 54: // 320
                if (!v_blank) {
                    output();
                    gfx.read_gd();
                    gfx.ba_end();
                }
                mobs.check_mdc_base_upd();
                mobs.check_dma();
                mobs.prep_dma(0);
                return;
            case 55: // 328
                if (!v_blank) {
                    output_right_edge_1();
                    gfx.flush_gd();
                }
                mobs.check_dma();
                mobs.prep_dma(0);
                return;
            case 56: // 336
                if (!v_blank) output();
                mobs.prep_dma(1);
                return;
            case 57: // 344
                if (!v_blank) {
                    output_right_edge_2();
                    gfx.row_end();
                }
                mobs.load_mdc();
                return;
            case 58: // 352
                if (!v_blank) output_border();
                mobs.prep_dma(2);
                return;
            case 59: // 360
                if (!v_blank) output_border();
                mobs.do_dma(0);
                return;
            case 60: // 368
                if (!v_blank) output_end();
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
            const u8 gfx_fg = (*to & GFX::FG_GFX_FLAG) ? 0xff : 0x00;
            u8 mdc = 0x00; // mob-gfx colliders
            u8 mmc = 0x00; // mob-mob colliders
            u8 mmc_on = false; // mob-mob coll. happened
            u8 col;
            u8 src_mb = 0x00; // source mob bit, if 'non-transparent'
            u8 mn = MOB_COUNT;
            u8 mb = 0x80; // mob bit ('id')

            do {
                MOB& m = mob[--mn];
                u8 c = m.pixel_out(x);
                if (c != TRANSPARENT_PX) {
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
        static const u16 VMA_TOP_RASTER_Y = 48;
        static const u8  FG_GFX_FLAG      = 0x80;
        static const u8  MC_flag          = 0x8;
        static const u16 G_ADDR_ECM_MASK  = 0x39ff;

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

        // called if cr1.den is modified (interesting only if it happens during VMA_TOP_RASTER_Y)
        void set_den(u8 den) {
            ba_area |= ((raster_y == VMA_TOP_RASTER_Y) && den);
            ba_check();
        }

        void set_y_scroll(u8 y_scroll_) {
            y_scroll = y_scroll_;
            ba_check();
            if (!ba_line) ba.gfx_rel();
        }

        void vma_start(u8 den) { ba_area = den; vc_base = 0; }
        void vma_end()         { ba_area = ba_line = false;  }

        void ba_check() {
            if (ba_area) {
                ba_line = (raster_y & y_scroll_mask) == y_scroll;
                if (ba_line) {
                    if (line_cycle > 10 && line_cycle < 54) {
                        ba.gfx_set(line_cycle | 0x80); // store cycle for AEC checking later
                        if (line_cycle < 14) activate_gfx();
                        // else activate_gfx() delayed (called from read_vm())
                    } else {
                        activate_gfx();
                    }
                }
            }
        }
        void ba_end() { ba.gfx_rel(); }

        void row_start() {
            vc = vc_base;
            vmri = 1;
            // (always all zeroes for 'idle' state anyway)
            vm_row[State::active][0] = vm_row[State::active][40];
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
            if (state == State::active) rc = (rc + 1) & 0x7;
        }

        void read_vm() {
            if (ba_line) {
                activate_gfx(); // delayed (see ba_check())
                col_ram.r(vc, vm_row[State::active][vmri].cr_data); // TODO: mask upper bits if 'noise' is implemented
                vm_row[State::active][vmri].vm_data = bank.r(vm_base | vc);
            }
        }

        void read_gd() {
            gdi = bank.r(gfx_addr);
            if (state == State::active) {
                vc = (vc + 1) & 0x3ff;
                ++vmri;
            }
        }

        void flush_gd() { gdi = 0x00; }

        void output(u8* to) {
            const auto put_px = [&to](const u8 px) { *to++ = px; };
            const auto bump = [this]() { gdo = gdi; vmd = vm_row[state][vmri-1]; };

            switch (mode) {
                case scm: {
                    for (u8 px = x_scroll; px > 0; --px) {
                        put_px(gdo & 0x80 ? vmd.cr_data | FG_GFX_FLAG : reg[bgc0]);
                        gdo <<= 1;
                    }
                    bump();
                    for (u8 px = x_scroll; px < 8; ++px) {
                        put_px(gdo & 0x80 ? vmd.cr_data | FG_GFX_FLAG : reg[bgc0]);
                        gdo <<= 1;
                    }

                    const auto& vd = vm_row[state][vmri].vm_data;
                    gfx_addr = c_base | (vd << 3) | rc;

                    return;
                }

                case mccm: {
                    if (vmd.cr_data & MC_flag) {
                        u8 shift = (x_scroll & 0x1) << 1;
                        for (u8 px = x_scroll; px > 0; --px) {
                            u8 gd = gdo & 0xc0;
                            u8 c = (gd == 0xc0)
                                ? vmd.cr_data & 0x7
                                : reg[bgc0 + (gd >> 6)];
                            put_px(c | gd); // sets fg-gfx flag
                            gdo <<= shift;
                            shift ^= 0x2;
                        }
                    } else {
                        for (u8 px = x_scroll; px > 0; --px) {
                            put_px(gdo & 0x80 ? vmd.cr_data | FG_GFX_FLAG : reg[bgc0]);
                            gdo <<= 1;
                        }
                    }
                    bump();
                    if (vmd.cr_data & MC_flag) {
                        u8 shift = 0;
                        for (u8 px = x_scroll; px < 8; ++px) {
                            u8 gd = gdo & 0xc0;
                            u8 c = (gd == 0xc0)
                                ? vmd.cr_data & 0x7
                                : reg[bgc0 + (gd >> 6)];
                            put_px(c | gd); // sets fg-gfx flag
                            gdo <<= shift;
                            shift ^= 0x2;
                        }
                    } else {
                        for (u8 px = x_scroll; px < 8; ++px) {
                            put_px(gdo & 0x80 ? vmd.cr_data | FG_GFX_FLAG : reg[bgc0]);
                            gdo <<= 1;
                        }
                    }

                    const auto& vd = vm_row[state][vmri].vm_data;
                    gfx_addr = c_base | (vd << 3) | rc;

                    return;
                }

                case sbmm: {
                    const auto& vd1 = vmd.vm_data;
                    for (u8 px = x_scroll; px > 0; --px) {
                        put_px(gdo & 0x80 ? (vd1 >> 4) | FG_GFX_FLAG : vd1 & 0xf);
                        gdo <<= 1;
                    }
                    bump();
                    const auto& vd2 = vmd.vm_data;
                    for (u8 px = x_scroll; px < 8; ++px) {
                        put_px(gdo & 0x80 ? (vd2 >> 4) | FG_GFX_FLAG : vd2 & 0xf);
                        gdo <<= 1;
                    }

                    gfx_addr = (c_base & 0x2000) | (vc << 3) | rc;

                    return;
                }

                case mcbmm: {
                    u8 shift = (x_scroll & 0x1) << 1;
                    for (u8 px = x_scroll; px > 0; --px) {
                        u8 gd = gdo >> 6;
                        u8 c;
                        switch (gd) {
                            case 0x0: c = reg[bgc0];         break;
                            case 0x1: c = vmd.vm_data >> 4;  break;
                            case 0x2: c = vmd.vm_data & 0xf; break;
                            case 0x3: c = vmd.cr_data;       break;
                        }
                        put_px(c | (gd << 6)); // sets fg-gfx flag
                        gdo <<= shift;
                        shift ^= 0x2;
                    }
                    bump();
                    shift = 0;
                    for (u8 px = x_scroll; px < 8; ++px) {
                        u8 gd = gdo >> 6;
                        u8 c;
                        switch (gd) {
                            case 0x0: c = reg[bgc0];         break;
                            case 0x1: c = vmd.vm_data >> 4;  break;
                            case 0x2: c = vmd.vm_data & 0xf; break;
                            case 0x3: c = vmd.cr_data;       break;
                        }
                        put_px(c | (gd << 6)); // sets fg-gfx flag
                        gdo <<= shift;
                        shift ^= 0x2;
                    }

                    gfx_addr = (c_base & 0x2000) | (vc << 3) | rc;

                    return;
                }

                case ecm: {
                    for (u8 px = x_scroll; px > 0; --px) {
                        put_px(gdo & 0x80
                            ? vmd.cr_data | FG_GFX_FLAG
                            : reg[bgc0 + (vmd.vm_data >> 6)]);
                        gdo <<= 1;
                    }
                    bump();
                    for (u8 px = x_scroll; px < 8; ++px) {
                        put_px(gdo & 0x80
                            ? vmd.cr_data | FG_GFX_FLAG
                            : reg[bgc0 + (vmd.vm_data >> 6)]);
                        gdo <<= 1;
                    }

                    const auto& vd = vm_row[state][vmri].vm_data;
                    gfx_addr = (c_base | (vd << 3) | rc) & G_ADDR_ECM_MASK;

                    return;
                }

                case icm: {
                    if (vmd.cr_data & MC_flag) {
                        u8 shift = (x_scroll & 0x1) << 1;
                        for (u8 px = x_scroll; px > 0; --px) {
                            put_px(gdo & 0xc0); // sets col.0 & fg-gfx flag
                            gdo <<= shift;
                            shift ^= 0x2;
                        }
                    } else {
                        for (u8 px = x_scroll; px > 0; --px) {
                            put_px(gdo & 0x80); // sets col.0 & fg-gfx flag
                            gdo <<= 1;
                        }
                    }
                    bump();
                    if (vmd.cr_data & MC_flag) {
                        u8 shift = 0;
                        for (u8 px = x_scroll; px < 8; ++px) {
                            put_px(gdo & 0xc0); // sets col.0 & fg-gfx flag
                            gdo <<= shift;
                            shift ^= 0x2;
                        }
                    } else {
                        for (u8 px = x_scroll; px < 8; ++px) {
                            put_px(gdo & 0x80); // sets col.0 & fg-gfx flag
                            gdo <<= 1;
                        }
                    }

                    const auto& vd = vm_row[state][vmri].vm_data;
                    gfx_addr = (c_base | (vd << 3) | rc) & G_ADDR_ECM_MASK;

                    return;
                }

                case ibmm1: {
                    for (u8 px = x_scroll; px > 0; --px) {
                        put_px(gdo & 0x80); // sets col.0 & fg-gfx flag
                        gdo <<= 1;
                    }
                    bump();
                    for (u8 px = x_scroll; px < 8; ++px) {
                        put_px(gdo & 0x80); // sets col.0 & fg-gfx flag
                        gdo <<= 1;
                    }

                    gfx_addr = ((c_base & 0x2000) | (vc << 3) | rc) & G_ADDR_ECM_MASK;

                    return;
                }

                case ibmm2: {
                    u8 shift = (x_scroll & 0x1) << 1;
                    for (u8 px = x_scroll; px > 0; --px) {
                        put_px(gdo & 0xc0); // sets col.0 & fg-gfx flag
                        gdo <<= shift;
                        shift ^= 0x2;
                    }
                    bump();
                    shift = 0;
                    for (u8 px = x_scroll; px < 8; ++px) {
                        put_px(gdo & 0xc0); // sets col.0 & fg-gfx flag
                        gdo <<= shift;
                        shift ^= 0x2;
                    }

                    gfx_addr = ((c_base & 0x2000) | (vc << 3) | rc) & G_ADDR_ECM_MASK;

                    return;
                }
            }
        }

        void output_border(u8* to) {
            const auto put = [&to](const u8 px) {
                *to++ = px; *to++ = px; *to++ = px; *to++ = px;
                *to++ = px; *to++ = px; *to++ = px; *to = px;
            };

            switch (mode) {
                case scm:
                case mccm:  put(reg[bgc0]);         return;
                case sbmm:  put(vmd.vm_data & 0xf); return;
                case mcbmm: put(reg[bgc0]);         return;
                case ecm:   put(reg[bgc0 + (vmd.vm_data >> 6)]); return;
                case icm:
                case ibmm1:
                case ibmm2: put(0x0);               return;
            }
        }

    private:
        enum State : u8 { idle = 0x0, active = 0x1 };
        static const u16 RC_IDLE = 0x3fff;

        void activate_gfx()   { state = active; rc &= 0x7; }
        void deactivate_gfx() { state = idle;   rc = RC_IDLE; }

        u8 ba_area = false; // set for each frame (at the top of disp.win)
        u8 ba_line = false;

        u8 state = State::idle;

        struct VM_data { // video-matrix data
            u8 vm_data = 0;
            u8 cr_data = 0; // color-ram data
        };
        VM_data vm_row[2][41]; // vm row buffer (idle/active states)

        u16 vc_base;
        u16 vc;
        u16 rc = RC_IDLE;
        u16 gfx_addr;
        VM_data vmd;
        u8  vmri; // vm_row index

        u8 gdi;
        u8 gdo;

        u8 mode = scm;

        Banker& bank;
        const Color_RAM& col_ram;
        const u8* reg;
        const u8& line_cycle;
        const u16& raster_y;
        BA& ba;
    };

    class Border {
    public:
        Border(const u16& raster_y_, const u8& den_, const u8& ecol_)
            : raster_y(raster_y_), den(den_), ecol(ecol_) {}

        u8 csel; // leff: 31, 24 - right: 335, 344

        void set_rsel(u8 rs) {
            cmp_top = 55 - (rs >> 1);
            cmp_bottom = 247 + (rs >> 1);
        }

        void output(u8* to) { if (main) do_output(to); }

        void output_left_edge(u8* to) {
            if (raster_y == cmp_top && den) aux = false;
            else if (raster_y == cmp_bottom) aux = true;
            if (!aux && main) {
                main = false;
                if (!csel) do_output(to - 1);
            } else output(to);
        }

        void output_right_edge_1(u8* to) {
            if (main) do_output(to);
            else if (!csel) {
                main = true;
                to[7] = ecol;
            }
        }

        void output_right_edge_2(u8* to) {
            if (csel) main = true;
            output(to);
        }

        void line_done() {
            if (raster_y == cmp_top && den) aux = false;
            else if (raster_y == cmp_bottom) main = true;
        }

    private:
        void do_output(u8* to) {
            auto e = to + 8;
            while (to < e) *to++ = ecol;
        }

        const u16& raster_y;
        const u8& den;
        const u8& ecol;

        u8 main;
        u8 aux;
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

    void output_start() {
        gfx.output_border(beam_pos);
    }

    void output_border() {
        mobs.output(raster_x, raster_x + 8, beam_pos);
        border.output(beam_pos);
        beam_pos += 8;
        raster_x += 8;
        gfx.output_border(beam_pos);
    }

    void output() {
        mobs.output(raster_x, raster_x + 8, beam_pos);
        border.output(beam_pos);
        beam_pos += 8;
        raster_x += 8;
        gfx.output(beam_pos);
    }

    void output_left_edge() {
        mobs.output(raster_x, raster_x + 8, beam_pos);
        border.output_left_edge(beam_pos);
        beam_pos += 8;
        raster_x += 8;
        gfx.output(beam_pos);
    }

    void output_right_edge_1() {
        mobs.output(raster_x, raster_x + 8, beam_pos);
        border.output_right_edge_1(beam_pos);
        beam_pos += 8;
        raster_x += 8;
        gfx.output(beam_pos);
    }

    void output_right_edge_2() {
        mobs.output(raster_x, raster_x + 8, beam_pos);
        border.output_right_edge_2(beam_pos);
        beam_pos += 8;
        raster_x += 8;
        gfx.output_border(beam_pos);
    }

    void output_end() {
        mobs.output(raster_x, raster_x + 8, beam_pos);
        border.output(beam_pos);
        beam_pos += 8;
        raster_x += 8;
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
