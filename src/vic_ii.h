#ifndef VIC_II_H_INCLUDED
#define VIC_II_H_INCLUDED

#include <iostream>
#include "common.h"
#include "io.h"
#include "host.h"


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
static const u16 VIEW_WIDTH        = 384; // lef/right borders: 32 px
static const u16 VIEW_HEIGHT       = 280; // top/bottom borders: 40 px

static const u16 RASTER_LINE_COUNT = 312;


enum Color : u32 {
    // Colodore by pepto - http://www.pepto.de/projects/colorvic/
    black       = 0xff000000,
    white       = 0xffffffff,
    red         = 0xff813338,
    cyan        = 0xff75cec8,
    purple      = 0xff8e3c97,
    green       = 0xff56ac4d,
    blue        = 0xff2e2c9b,
    yellow      = 0xffedf171,
    orange      = 0xff8e5029,
    brown       = 0xff553800,
    light_red   = 0xffc46c71,
    dark_grey   = 0xff4a4a4a,
    grey        = 0xff7b7b7b,
    light_green = 0xffa9ff9f,
    light_blue  = 0xff706deb,
    light_gray  = 0xffb2b2b2,
};

static const u32 Palette[16] = {
    black,  white, red,       cyan,      purple, green,       blue,       yellow,
    orange, brown, light_red, dark_grey, grey,   light_green, light_blue, light_gray,
};


class Color_RAM {
public:
    static const u16 SIZE = 0x0400;

    Color_RAM() { reset(); }

    void reset() { for (auto& c : ram) c = 0x0; }

    void r(const u16& addr, u8& data) const { data = ram[addr]; }
    void w(const u16& addr, const u8& data) { ram[addr] = data & 0xf; } // masking the write is enough

private:
    u8 ram[SIZE]; // just the lower nibble is actually used

};


class Banker {
public:
    // TODO: ROMH
    Banker(const u8* ram_, const u8* charr) : ram(ram_), chr(charr) {
        set_bank(0x3, 0x3);
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
    void set_bank(u8 bits, u8 bit_vals) {
        // TODO: a util func (set_bits(old, new_bits, new_bit_vals))
        u8 new_mode = ((bit_vals & bits) | (mode & ~bits)) & 0x3;
        if (new_mode != mode) {
            mode = new_mode;
            pla = (u8*)PLA[mode];
            std::cout << "\n[VIC_II::Banker]: mode " << (int)mode;
        }
    }

private:
    static const u8 PLA[4][4];

    u8 mode = 0xff; // active mode
    const u8* pla;  // active pla line

    const u8* ram;
    const u8* chr;
    //const u8* romh;
};


// TODO: lightpen (3.11)
template <typename VIC_II_out>
class Core { // 6569 (PAL-B)
private:
    IO::Sync::Slave sync;

public:
    static const u8 REG_COUNT = 64;

    static const u8 TRANSPARENT = 0xff;


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
    /* Raster line : beam_area
          000..009 : v_blank_290_009 (top v-blank)
          010..047 : disp_010_047    (top border)
          048..247 : disp_048_247    (video-matrix access area)
          248..254 : disp_248_254    (post vmaa)
          255..289 : disp_255_289    (bottom border)
          290..312 : v_blank_290_009 (bottom v-blank)
    */
    enum Beam_area : u8 {
        v_blank_290_009, disp_010_047, disp_048_247, disp_248_254, disp_255_289,
    };


    Core(
          const IO::Sync::Master& sync_master,
          const u8* ram_, const Color_RAM& col_ram_, const u8* charr,
          Int_sig& irq, bool& ba_low, VIC_II_out& vic_out_)
        : sync(sync_master),
          banker(ram_, charr),
          irq_unit(reg, irq),
          ba_unit(ba_low),
          mob_unit(banker, col_ram_, reg, raster_y, ba_unit, irq_unit),
          gfx_unit(banker, col_ram_, reg, raster_y, vert_border_on, ba_unit),
          vic_out(vic_out_)
    { reset_cold(); }

    Banker banker;

    void reset_warm() { irq_unit.reset(); }

    void reset_cold() {
        irq_unit.reset();
        for (int r = 0; r < REG_COUNT; ++r) w(r, 0);
    }

    void r(const u8& ri, u8& data) {
        tick();
        // std::cout << "VIC r: " << (int)ri << " " << (int)data << "\n";
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
        // std::cout << "VIC w: " << (int)ri << " " << (int)data << "\n";
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
                gfx_unit.y_scroll = d & y_scroll_mask;
                break;
            case rast:
                cmp_raster = (cmp_raster & 0x100) | d;
                //if (cmp_raster == 52) std::cout << " r" << (int)cmp_raster << " " << (int)cycle;
                break;
            case cr2:
                gfx_unit.set_mcm(d & mcm_bit);
                set_csel(d & CR2::csel_bit);
                gfx_unit.x_scroll = d & x_scroll_mask;
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
            case bgc0: gfx_unit.set_bgc0(d); break;
            case bgc1: gfx_unit.set_bgc1(d); break;
            case bgc2: gfx_unit.set_bgc2(d); break;
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

    void tick() {
        static const u16 LAST_RASTER_Y = RASTER_LINE_COUNT; // (312 done for 1 cycle)

        if (sync.tick()) return;

        //if (cycle == 53) set_csel(1); else if (cycle == 54) set_csel(0); else if (cycle == 56) set_csel(1);
        switch (cycle++) {
            case 0: // h-blank (continues)
                //if (raster_y == 249) set_rsel(0); else if (raster_y == 255) set_rsel(1);
                mob_unit.do_dma(2);

                vic_out.line_done(raster_y);

                ++raster_y;

                switch (beam_area) {
                    case v_blank_290_009: if (raster_y == 10) beam_area = disp_010_047; break;
                    case disp_010_047:    if (raster_y == 48) { gfx_unit.init(den); beam_area = disp_048_247; } break;
                    case disp_048_247:    if (raster_y == 248) beam_area = disp_248_254; break;
                    case disp_248_254:    if (raster_y == 255) beam_area = disp_255_289; break;
                    case disp_255_289:    if (raster_y == 290) beam_area = v_blank_290_009; break;
                }

                if (raster_y == cmp_raster && raster_y != LAST_RASTER_Y) irq_unit.req(IRQ_unit::rst);

                /* NOTE: mob_unit.pre_dma/do_dma pair is done, so that it takes the specified
                         5 cycles in total. do_dma is done at the last moment possible, and
                         all the memory access is then done at once (which is not how it is
                         done in reality). Normally this is not an issue since the CPU is asleep
                         all the way through (and hence, cannot modify the memory). But...
                   TODO: Might a cartridge modify the memory during these cycles? Surely not in
                         a 'safe' way since VIC has/uses the bus for both phases, right?
                         Anyway, the current MOB access would not be 100% cycle/memory accurate
                         in those cases.
                */

                return;
            case 1: // h-blank
                mob_unit.pre_dma(5);
                if (raster_y == LAST_RASTER_Y) {
                    raster_y = 0;
                    if (cmp_raster == 0) irq_unit.req(IRQ_unit::rst);
                    vic_out.frame_done();
                }
                return;
            case 2:  mob_unit.do_dma(3);  return;
            case 3:  mob_unit.pre_dma(6); return;
            case 4:  mob_unit.do_dma(4);  return;
            case 5:  mob_unit.pre_dma(7); return;
            case 6:  mob_unit.do_dma(5);  return;
            case 7:                       return;
            case 8:  mob_unit.do_dma(6);  return;
            case 9:                       return;
            case 10: mob_unit.do_dma(7);  return; // h-blank (ends)
            case 11: // x: 496
                switch (beam_area) {
                    case v_blank_290_009: return;
                    case disp_048_247:
                        gfx_unit.gfx_activation();
                    case disp_010_047: case disp_248_254: case disp_255_289:
                        output(496);
                        return;
                }
            case 12: // x: 0
                switch (beam_area) {
                    case v_blank_290_009: return;
                    case disp_048_247:
                        gfx_unit.gfx_activation();
                    case disp_010_047: case disp_248_254: case disp_255_289:
                        output(0);
                        return;
                }
            case 13: // x: 8
                switch (beam_area) {
                    case v_blank_290_009: return;
                    case disp_048_247:
                        gfx_unit.gfx_activation();
                    case disp_010_047: case disp_248_254: case disp_255_289:
                        output(8);
                        return;
                }
            case 14: // x: 16
                mob_unit.inc_mdc_base_2();
                switch (beam_area) {
                    case v_blank_290_009: return;
                    case disp_048_247:
                        gfx_unit.gfx_activation();
                    case disp_010_047: case disp_248_254: case disp_255_289:
                        output(16);
                        return;
                }
            case 15: // x: 24 (display column starts)
                mob_unit.inc_mdc_base_1();
                switch (beam_area) {
                    case v_blank_290_009: check_left_vb(); return;
                    case disp_048_247:
                        gfx_unit.gfx_activation();
                    case disp_010_047: case disp_248_254: case disp_255_289:
                        gfx_unit.row_start();
                        output_on_edge(24);
                        return;
                }
             // x: 32..327
            case 16: case 17: case 18: case 19:
            case 20: case 21: case 22: case 23: case 24: case 25: case 26: case 27: case 28: case 29:
            case 30: case 31: case 32: case 33: case 34: case 35: case 36: case 37: case 38: case 39:
            case 40: case 41: case 42: case 43: case 44: case 45: case 46: case 47: case 48: case 49:
            case 50: case 51: case 52:
                switch (beam_area) {
                    case v_blank_290_009: return;
                    case disp_048_247:
                        gfx_unit.gfx_activation();
                    case disp_010_047: case disp_248_254: case disp_255_289:
                        output(cycle*8 - 104);
                        return;
                }
            case 53: // x: 328
                switch (beam_area) {
                    case v_blank_290_009:
                        check_right_vb(335);
                        return;
                    case disp_048_247:
                        // TODO: 3.14.5. Doubled text lines ==> act also in 53-56
                        gfx_unit.gfx_activation();
                    case disp_010_047: case disp_248_254: case disp_255_289:
                        output_on_edge(328);
                        return;
                }
            case 54: // x: 336 (display column ends)
                mob_unit.check_mdc_base_upd();
                mob_unit.check_dma(0, 3);
                mob_unit.pre_dma(0);
                if (beam_area != v_blank_290_009) output(336);
                return;
            case 55: // x: 344
                mob_unit.check_dma(4, 7);
                switch (beam_area) {
                    case v_blank_290_009:
                        check_right_vb(344);
                        return;
                    case disp_010_047: case disp_048_247: case disp_248_254: case disp_255_289:
                        output_on_edge(344);
                        return;
                }
            case 56: // x: 352
                mob_unit.pre_dma(1);
                if (beam_area != v_blank_290_009) output(352);
                return;
            case 57: // x: 360
                mob_unit.check_disp();
                switch (beam_area) {
                    case v_blank_290_009: return;
                    case disp_048_247: case disp_248_254:
                        gfx_unit.row_end();
                    case disp_010_047: case disp_255_289:
                        output(360);
                        return;
                }
            case 58:  // x: 368
                mob_unit.pre_dma(2);
                if (beam_area != v_blank_290_009) output(368);
                return;
            case 59: mob_unit.do_dma(0);  return; // h-blank (first)
            case 60: mob_unit.pre_dma(3); return;
            case 61: mob_unit.do_dma(1);  return;
            case 62:
                cycle = 0;
                mob_unit.pre_dma(4);
                switch (beam_area) {
                    case v_blank_290_009: case disp_010_047: case disp_255_289:
                        return;
                    case disp_048_247:
                        check_top_hb(raster_y);
                    case disp_248_254:
                        check_bottom_hb(raster_y);
                        return;
                }
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
            irq_id_bits = 0x0f,
        };

        IRQ_unit(u8* reg_, Int_sig& irq_sig_) : reg(reg_), irq_sig(irq_sig_) { reset(); }

        void reset() { irq_set = 0x00; }

        void w_ireg(u8 data) {
            u8 new_int_bits = (reg[ireg] & ~data) & irq_id_bits;
            if (((new_int_bits & reg[ien]) == 0x00) && irq_set) {
                irq_sig.clr();
                irq_set = 0x00;
            }
            reg[ireg] = new_int_bits | irq_set;
        }

        void ien_upd() { check(); }

        void req(u8 kind) {
            reg[ireg] |= kind;
            check();
        }

    private:
        void check() {
            if ((reg[ireg] & reg[ien]) && !irq_set) {
                irq_sig.set();
                irq_set = irq;
                reg[ireg] |= irq;
            }
        }

        u8 irq_set;

        u8* reg;
        Int_sig& irq_sig;
    };


    class BA_unit {
    public:
        void req()  { ++req_cnt; ba_low = true; }
        void done() { if (--req_cnt == 0) ba_low = false; }

        BA_unit(bool& ba_low_) : ba_low(ba_low_) {}
    private:
        u8 req_cnt = 0;
        bool& ba_low;
    };


    class MOB_unit {
    public:
        static const u8  MOB_COUNT = 8;
        static const u16 MP_BASE   = 0x3f8; // offset from vm_base

        class MOB {
        public:
            bool ye;

            bool disp_on = false;
            bool dma_on = false;
            bool mdc_base_upd;

            u8 mdc_base;
            u8 mdc;

            void set_x_hi(u8 hi)  { x = hi ? x | 0x100 : x & 0x0ff; }
            void set_x_lo(u8 lo)  { x = (x & 0x100) | lo; }
            void set_ye(bool ye_) { ye = ye_; mdc_base_upd = !ye; }
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
                if (data == OUT_OF_DATA) return TRANSPARENT;
                if ((data & PRISTINE_DATA) == PRISTINE_DATA) {
                    if (px != x) return TRANSPARENT;
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

            u8 col[4] = { TRANSPARENT, 0, 0, 0 };
        };


        MOB mob[MOB_COUNT];

        void set_vm_base(u16 vm_base) { mp_base = vm_base | MP_BASE; }

        void check_mdc_base_upd() { for(auto&m : mob) if (m.ye) m.mdc_base_upd ^= m.ye; }
        void check_dma(u8 from, u8 to) {
            for (u8 mn = from, mb = (0x01 << from); mn <= to; ++mn, mb <<= 1) {
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
        void check_disp() {
            for (u8 mn = 0; mn < MOB_COUNT; ++mn) {
                MOB& m = mob[mn];
                m.mdc = m.mdc_base;
                if (m.dma_on && (reg[m0y + (mn * 2)] == (raster_y & 0xff))) m.disp_on = true;
            }
        }
        void inc_mdc_base_2() { for (auto& m : mob) m.mdc_base += (m.mdc_base_upd * 2); }
        void inc_mdc_base_1() {
            for (auto& m : mob) {
                m.mdc_base += m.mdc_base_upd;
                if (m.mdc_base == 63) m.dma_on = m.disp_on = false;
            }
        }
        void pre_dma(u8 mn) { if (mob[mn].dma_on) ba.req(); }
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
                ba.done();
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
            u8 col = TRANSPARENT;
            u8 mn = MOB_COUNT;
            u8 mb = 0x80; // mob bit ('id')

            do {
                MOB& m = mob[--mn];
                u8 m_col = m.pixel_out(x);
                if (m_col != TRANSPARENT) {
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

        enum Mode_bit : u8 {
            ecm_set = 0x4, bmm_set = 0x2, mcm_set = 0x1, not_set = 0x0
        };
        enum Mode_id : u8 {
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
        u8 x_scroll;
        u16 vm_base;
        u16 c_base;

        GFX_unit(
            Banker& bank_, const Color_RAM& col_ram_, const u8* reg_, const u16& raster_y_,
            const bool& v_border_on_, BA_unit& ba_)
          : bank(bank_), col_ram(col_ram_), reg(reg_), raster_y(raster_y_),
            v_border_on(v_border_on_), ba(ba_)
        { set_mode(scm); }

        // called at the beginning of VMA_TOP_RASTER_Y
        void init(bool den)    { frame_den = den; vc_base = 0; }
        // called if cr1.den is modified (interesting only if it happens during VMA_TOP_RASTER_Y)
        void set_den(bool den) { frame_den |= (raster_y == VMA_TOP_RASTER_Y && den); }

        void set_ecm(bool e)   { set_mode(e ? mode_id | ecm_set : mode_id & ~ecm_set); }
        void set_bmm(bool b)   { set_mode(b ? mode_id | bmm_set : mode_id & ~bmm_set); }
        void set_mcm(bool m)   { set_mode(m ? mode_id | mcm_set : mode_id & ~mcm_set); }

        void set_bgc0(u8 bgc0) { color_tbl.scm[0] = color_tbl.mccm[0] = color_tbl.mcbmm[0] = bgc0; }
        void set_bgc1(u8 bgc1) { color_tbl.mccm[1] = bgc1; }
        void set_bgc2(u8 bgc2) { color_tbl.mccm[2] = bgc2; }

        void row_start()       { vc = vc_base; vmri = 0; }
        void row_end() {
            if (!gfx_idle()) {
                if (++rc == 8) { gfx_deactivate(); vc_base = vc; }
            }
        }

        void gfx_activation() {
            if (((raster_y & y_scroll_mask) == y_scroll) && frame_den) {
                ba_low_cnt += 1;
                if (ba_low_cnt == 1) ba.req();
                else if (ba_low_cnt == 4) { // 'aec low' ?
                    // TODO: 3.14.6. DMA delay (should not actually wait 3 cycles & stuph...)
                    gfx_activate();
                }
            } else if (ba_low_cnt > 0) {
                ba.done();
                ba_low_cnt = 0;
            }
        }

        u8 pixel_out(u8 pixel_time, bool& fg_gfx) { // 8 pixels/cycle --> pixel_time: 0..7
            if (pixel_time == x_scroll && !row_done()) {
                read_vmd();
                read_gd();
            } else if (((pixel_time - x_scroll) % mode->pixel_width) == 0) { // TODO: nicer?
                pixel_data <<= mode->pixel_width;
            }

            u8 p_col;
            if (!v_border_on) {
                fg_gfx = pixel_data & 0x80;
                p_col = mode->color[pixel_data >> (8 - mode->pixel_width)];
            } else {
                p_col = reg[bgc0];
            }

            return p_col;
        }

    private:
        static const u16 RC_IDLE = 0x3fff;

        bool gfx_idle()   const { return rc == RC_IDLE; }
        bool gfx_active() const { return rc < 8; }
        void gfx_activate()     { rc = 0; };
        void gfx_deactivate()   { rc = RC_IDLE; };
        bool vma_row()    const { return rc == 0; }
        bool row_done()   const { return vmri == 40; }

        void read_vmd() { // video-matrix data
            if (vma_row()) {
                col_ram.r(vc, vm_row[vmri].cr_data);
                vm_row[vmri].vm_data = bank.r(vm_base | vc);
            } else if (gfx_idle()) {
                vm_row[vmri].cr_data = 0;
            }
        }

        void read_gd() { // gfx data
            static const u8 MC_flag          = 0x8;
            static const u16 G_ADDR_ECM_MASK = 0x39ff;
            //mode = &mode_tbl[mode_id]; //if it is necessary to delay the change

            u16 addr;
            const u8& vd = vm_row[vmri].vm_data;
            const u8& cd = vm_row[vmri].cr_data;
            switch (mode_id) {
                case scm:
                    mode->color[1] = cd;
                    addr = c_base | (vd << 3) | rc;
                    break;
                case mccm:
                    if (cd & MC_flag) {
                        mode->pixel_width = 2;
                        mode->color = color_tbl.mccm;
                        mode->color[3] = cd & ~MC_flag;
                    } else {
                        mode->pixel_width = 1;
                        mode->color = color_tbl.scm;
                        mode->color[1] = cd;
                    }
                    addr = c_base | (vd << 3) | rc;
                    break;
                case sbmm:
                    mode->color[0] = vd & 0xf;
                    mode->color[1] = vd >> 4;
                    addr = (c_base & 0x2000) | (vc << 3) | rc;
                    break;
                case mcbmm:
                    mode->color[1] = vd >> 4;
                    mode->color[2] = vd & 0xf;
                    mode->color[3] = cd;
                    addr = (c_base & 0x2000) | (vc << 3) | rc;
                    break;
                case ecm:
                    mode->color[0] = reg[bgc0 + (vd >> 6)];
                    mode->color[1] = cd;
                    addr = (c_base | (vd << 3) | rc) & G_ADDR_ECM_MASK;
                    break;
                case icm:
                    mode->pixel_width = (cd & MC_flag) ? 2 : 1;
                    addr = (c_base | (vd << 3) | rc) & G_ADDR_ECM_MASK;
                    break;
                case ibmm1: case ibmm2: default:
                    addr = ((c_base & 0x2000) | (vc << 3) | rc) & G_ADDR_ECM_MASK;
                    break;
            }

            pixel_data = bank.r(addr);

            ++vc;
            ++vmri;
            if (row_done() && ba_low_cnt > 0) {
                ba.done();
                ba_low_cnt = 0;
            }
        }

        u8 frame_den; // set for each frame (at the top of disp.win)

        u8 ba_low_cnt = 0;

        struct VM_data { // video-matrix data
            u8 vm_data;
            u8 cr_data; // color-ram data
        };
        VM_data vm_row[40]; // vm row buffer

        u16 vc_base; // video counter base
        u16 vc;      // video counter
        u16 rc = 8;  // row counter (0: vm-access on, 8: gfx idle)
        u8  vmri;    // vm_row index

        u8 pixel_data = 0x00;

        struct Color_tbl {
            u8 scm[2];
            u8 mccm[4];
            u8 sbmm[2];
            u8 mcbmm[4];
            u8 ecm[2];
            u8 invalid[4] = { 0, 0, 0, 0 }; // all black
        };
        Color_tbl color_tbl;

        struct Mode {
            u8 pixel_width;
            u8* color;
        };
        Mode mode_tbl[8] = {
            { 1, color_tbl.scm     },
            { 2, color_tbl.mccm    }, // pixel_width set dynamically based on c-data
            { 1, color_tbl.sbmm    },
            { 2, color_tbl.mcbmm   },
            { 1, color_tbl.ecm     },
            { 2, color_tbl.invalid }, // pixel_width set dynamically based on c-data
            { 1, color_tbl.invalid },
            { 2, color_tbl.invalid },
        };
        Mode* mode = &mode_tbl[0]; // active mode

        u8 mode_id;
        /*
           TODO: switch mode immediately, or do it at next g_access()..?

           What does VIC-II do when mode is changed in the middle of a line, and xscroll != 0?
           Spit out the remaining pixels, and then change mode?
           Or change it immediately? (the simpler alternative)
        */
        void set_mode(u8 m) { mode_id = m; mode = &mode_tbl[m]; }

        Banker& bank;
        const Color_RAM& col_ram;
        const u8* reg;
        const u16& raster_y;
        const bool& v_border_on;
        BA_unit& ba;
    };


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


    // checks done during blanking periods (don't necessarely need to be pixel-perfect)
    void check_left_vb()               { if (!vert_border_on) main_border_on = false; }
    void check_right_vb(u16 x)         { if (x == cmp_right) main_border_on = true; }
    void check_top_hb(u16 raster_y)    { if (raster_y == cmp_top && den) vert_border_on = false; }
    void check_bottom_hb(u16 raster_y) { if (raster_y == cmp_bottom) vert_border_on = true; }

    /*
    void output(u16 x_from, u16 x_to) {
        do exude_pixel(x_from, x_from % 8);
        while(++x_from <= x_to);
    }
    void output(u16 x) { output(x, x+7); }
    void output_on_edge(u16 x) {
        for (u16 x_to = x+7; x <= x_to; ++x) {
            if (x == cmp_left) {
                if (raster_y == cmp_top) {
                    if (den) main_border_on = vert_border_on = false;
                }
                else if (raster_y == cmp_bottom) vert_border_on = true;
                else if (!vert_border_on) main_border_on = false;
            } else if (x == cmp_right) main_border_on = true;
            exude_pixel(x, x % 8);
        }
    }
    */
    void output(u16 x) {
        for (int pt = 0; pt < 8; ++pt, ++x)
            exude_pixel(x, pt);
    }
    void output_on_edge(u16 x) {
        for (int pt = 0; pt < 8; ++pt) {
            if (x == cmp_left) {
                if (raster_y == cmp_top) {
                    if (den) main_border_on = vert_border_on = false;
                }
                else if (raster_y == cmp_bottom) vert_border_on = true;
                else if (!vert_border_on) main_border_on = false;
            } else if (x == cmp_right) main_border_on = true;
            exude_pixel(x++, pt);
        }
    }

    void exude_pixel(u16 x, u8 pixel_time) {
        bool gfx_fg = false;
        u8 g_col = gfx_unit.pixel_out(pixel_time, gfx_fg); // must call always (to keep in sync)

        u8 src_mob = 0; // if not transparet then src bit will be set here
        u8 m_col = mob_unit.pixel_out(x, gfx_fg, src_mob); // must call always (to keep in sync)

        // priorites: border < mob|gfx_fg (based on reg[mndp]) < gfx_bg
        u8 o_col;
        if (main_border_on) o_col = reg[ecol];
        else if (m_col != TRANSPARENT) {
            if (!gfx_fg) o_col = m_col;
            else o_col = (reg[mndp] & src_mob) ? g_col : m_col;
        }
        else o_col = g_col;

        vic_out.put_pixel(o_col);
    }


    IRQ_unit irq_unit;
    BA_unit ba_unit;
    MOB_unit mob_unit;
    GFX_unit gfx_unit;

    u8 reg[REG_COUNT];

    u8 cycle = 1;
    u16 raster_y = 0;
    Beam_area beam_area = v_blank_290_009;

    bool den;
    u16 cmp_left;
    u16 cmp_right;
    u16 cmp_top;
    u16 cmp_bottom;
    u16 cmp_raster;

    bool main_border_on;
    bool vert_border_on;

    VIC_II_out& vic_out;
};


} // namespace VIC_II

#endif // VIC_II_H_INCLUDED
