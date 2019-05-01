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
static const u16 VIEW_WIDTH        = 384; // lef/right borders: 32 px
static const u16 VIEW_HEIGHT       = 264; // top/bottom borders: 32 px

static const u16 RASTER_LINE_COUNT = 312;
static const u8  LINE_CYCLES       =  63;


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
            // std::cout << "\n[VIC_II::Banker]: mode " << (int)mode;
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


template <typename Out>
class Core { // 6569 (PAL-B)
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
          000..017 : v_blank
          018..047 : disp_018_047    (top border)
          048..247 : disp_048_247    (video-matrix access area)
          248..254 : disp_248_254    (post vmaa)
          255..281 : disp_255_281    (bottom border)
          282..312 : v_blank
    */
    enum Beam_area : u8 {
        v_blank, disp_018_047, disp_048_247, disp_248_254, disp_255_281,
    };


    Core(
          /*const IO::Sync::Master& sync_master,*/
          const u8* ram_, const Color_RAM& col_ram_, const u8* charr,
          Int_sig& irq, u16& ba_low, Out& out_)
        : banker(ram_, charr),
          irq_unit(reg, irq),
          ba_unit(ba_low),
          mob_unit(banker, col_ram_, reg, raster_y, ba_unit, irq_unit),
          gfx_unit(banker, col_ram_, reg, line_cycle, raster_y, vert_border_on, ba_unit),
          out(out_)
    { reset_cold(); }

    Banker banker;

    void reset_warm() { irq_unit.reset(); }

    void reset_cold() {
        reset_warm();
        for (int r = 0; r < REG_COUNT; ++r) w(r, 0);
        raster_y = RASTER_LINE_COUNT - 1;
        line_cycle = lp_p1_p6_low = lp_pb_b4_low = 0;
    }

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
                gfx_unit.set_y_scroll(d & y_scroll_mask, beam_area == disp_048_247);
                break;
            case rast:
                cmp_raster = (cmp_raster & 0x100) | d;
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

    void tick(const u16& frame_cycle) {
        static const u16 LAST_RASTER_Y = RASTER_LINE_COUNT; // (312 done for 1 cycle)

        line_cycle = frame_cycle % LINE_CYCLES;

        switch (line_cycle) {
            case  0:
                mob_unit.do_dma(2);

                out.line_done(raster_y);

                ++raster_y;

                switch (beam_area) {
                    case v_blank:
                        if (raster_y == LAST_RASTER_Y) {
                            out.frame_done();
                            return;
                        }
                        if (raster_y == 18) {
                            beam_area = disp_018_047;
                            frame_lp = 0;
                            set_lp(lp_p1_p6_low || lp_pb_b4_low);
                        }
                        break;
                    case disp_018_047:
                        if (raster_y == 48) {
                            gfx_unit.init(den);
                            beam_area = disp_048_247;
                        }
                        break;
                    case disp_048_247:
                        if (raster_y == 248) {
                            gfx_unit.deinit();
                            beam_area = disp_248_254;
                        }
                        else gfx_unit.ba_eval();
                        break;
                    case disp_248_254:
                        if (raster_y == 255) beam_area = disp_255_281;
                        break;
                    case disp_255_281:
                        if (raster_y == 282) beam_area = v_blank;
                        break;
                }

                if (raster_y == cmp_raster) irq_unit.req(IRQ_unit::rst);

                return;
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
            case  1:
                mob_unit.pre_dma(5);
                if (raster_y == LAST_RASTER_Y) {
                    raster_y = 0;
                    if (cmp_raster == 0) irq_unit.req(IRQ_unit::rst);
                }
                return;
            case  2: mob_unit.do_dma(3);  return;
            case  3: mob_unit.pre_dma(6); return;
            case  4: mob_unit.do_dma(4);  return;
            case  5: mob_unit.pre_dma(7); return;
            case  6: mob_unit.do_dma(5);
            case  7:                      return;
            case  8: mob_unit.do_dma(6);
            case  9:                      return;
            case 10: mob_unit.do_dma(7);  return;
            case 11:
                if (beam_area == disp_048_247) gfx_unit.ba_toggle();
                return;
            case 12:
                if (beam_area != v_blank) output_at(496);
                return;
            case 13:
                if (beam_area != v_blank) {
                    gfx_unit.row_start();
                    output_at(0);
                }
                return;
            case 14:
                mob_unit.inc_mdc_base_2();
                if (beam_area != v_blank) output();
                return;
            case 15:
                mob_unit.inc_mdc_base_1();
                switch (beam_area) {
                    case v_blank: return;
                    case disp_048_247:
                        gfx_unit.read_vm();
                    case disp_018_047: case disp_248_254: case disp_255_281:
                        output();
                        gfx_unit.read_gfx();
                        return;
                }
            case 16:
                switch (beam_area) {
                    case v_blank: check_left_vb(); return;
                    case disp_048_247:
                        gfx_unit.read_vm();
                    case disp_018_047: case disp_248_254: case disp_255_281:
                        output_on_edge();
                        gfx_unit.read_gfx();
                        return;
                }
            case 17: case 18: case 19:
            case 20: case 21: case 22: case 23: case 24: case 25: case 26: case 27: case 28: case 29:
            case 30: case 31: case 32: case 33: case 34: case 35: case 36: case 37: case 38: case 39:
            case 40: case 41: case 42: case 43: case 44: case 45: case 46: case 47: case 48: case 49:
            case 50: case 51: case 52: case 53:
                switch (beam_area) {
                    case v_blank: return;
                    case disp_048_247:
                        gfx_unit.read_vm();
                    case disp_018_047: case disp_248_254: case disp_255_281:
                        output();
                        gfx_unit.read_gfx();
                        return;
                }
            case 54:
                mob_unit.check_mdc_base_upd();
                mob_unit.check_dma();
                mob_unit.pre_dma(0);
                switch (beam_area) {
                    case v_blank:
                        check_right_vb(335);
                        return;
                    case disp_048_247:
                        gfx_unit.read_vm();
                        gfx_unit.ba_end();
                    case disp_018_047: case disp_248_254: case disp_255_281:
                        output_on_edge();
                        gfx_unit.read_gfx();
                        return;
                }
            case 55:
                mob_unit.check_dma();
                if (beam_area != v_blank) output();
                return;
            case 56:
                mob_unit.pre_dma(1);
                if (beam_area == v_blank) check_right_vb(344);
                else output_on_edge();
                return;
            case 57:
                mob_unit.check_disp();
                switch (beam_area) {
                    case v_blank: return;
                    case disp_048_247: case disp_248_254:
                        gfx_unit.row_end();
                    case disp_018_047: case disp_255_281:
                        output();
                        return;
                }
            case 58:
                mob_unit.pre_dma(2);
                if (beam_area != v_blank) output();
                return;
            case 59:
                mob_unit.do_dma(0);
                if (beam_area != v_blank) output();
                return;
            case 60: mob_unit.pre_dma(3); return;
            case 61: mob_unit.do_dma(1);  return;
            case 62:
                mob_unit.pre_dma(4);
                switch (beam_area) {
                    case v_blank: case disp_018_047: case disp_255_281:
                        return;
                    case disp_048_247: case disp_248_254:
                        check_hb(raster_y);
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
        BA_unit(u16& ba_low_) : ba_low(ba_low_) {}

        void mob_start()       { ba_low += 0x100; }
        void mob_done()        { ba_low -= 0x100; }
        void gfx_set(u8 b)     { ba_low = (ba_low & 0x700) | b; }
        void gfx_reset()       { gfx_set(0); }
        u8   gfx_state() const { return ba_low & 0xff; }
    private:
        u16& ba_low;
    };


    class MOB_unit {
    public:
        static const u8  MOB_COUNT = 8;
        static const u16 MP_BASE   = 0x3f8; // offset from vm_base

        class MOB {
        public:
            // TODO: these 4 flags into one u8...?
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
        void pre_dma(u8 mn) { if (mob[mn].dma_on) ba.mob_start(); }
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
                ba.mob_done();
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

        u8 y_scroll; // TODO: private?
        u8 x_scroll;
        u16 vm_base;
        u16 c_base;

        GFX_unit(
            Banker& bank_, const Color_RAM& col_ram_, const u8* reg_,
            const u8& line_cycle_, const u16& raster_y_,
            const bool& v_border_on_, BA_unit& ba_)
          : bank(bank_), col_ram(col_ram_), reg(reg_),
            line_cycle(line_cycle_), raster_y(raster_y_),
            v_border_on(v_border_on_), ba(ba_)
        { set_mode(scm); }

        // called at the beginning of VMA_TOP_RASTER_Y
        void init(bool den)    { frame_den = den; vc_base = 0; }
        // called if cr1.den is modified (interesting only if it happens during VMA_TOP_RASTER_Y)
        void set_den(bool den) { frame_den |= (raster_y == VMA_TOP_RASTER_Y && den); }

        void set_y_scroll(u8 y_scroll_, bool vma_area) {
            y_scroll = y_scroll_;
            if (vma_area) {
                ba_eval();
                if (line_cycle > 10 && line_cycle < 54) ba_toggle();
            }
        }

        void set_ecm(bool e)   { set_mode(e ? mode_id | ecm_set : mode_id & ~ecm_set); }
        void set_bmm(bool b)   { set_mode(b ? mode_id | bmm_set : mode_id & ~bmm_set); }
        void set_mcm(bool m)   { set_mode(m ? mode_id | mcm_set : mode_id & ~mcm_set); }

        void set_bgc0(u8 bgc0) { color_tbl.scm[0] = color_tbl.mccm[0] = color_tbl.mcbmm[0] = bgc0; }
        void set_bgc1(u8 bgc1) { color_tbl.mccm[1] = bgc1; }
        void set_bgc2(u8 bgc2) { color_tbl.mccm[2] = bgc2; }

        void ba_eval() { ba_line = ((raster_y & y_scroll_mask) == y_scroll) && frame_den; }

        void ba_toggle() {
            if (ba_line) {
                ba.gfx_set(line_cycle | 0x80); // store cycle for AEC checking later
                activate_gfx();
            }
            else ba.gfx_reset();
        }

        void row_start() {
            vc = vc_base;
            vmri = 0;
            if (ba_line) rc = 0;
        }

        void row_end() {
            if (rc == 7 && !ba_line) {
                deactivate_gfx();
                vc_base = vc;
            }
            else if (gfx_active()) rc = (rc + 1) & 0x7;
        }

        void ba_end() { ba.gfx_reset(); }

        void deinit() { ba_line = false; }

        void read_vm() {
            if (ba_line) { // (ba.gfx_state())
                col_ram.r(vc, vm_row[vmri].cr_data);
                vm_row[vmri].vm_data = bank.r(vm_base | vc);
            }
        }

        void read_gfx() {
            static const u8 MC_flag          = 0x8;
            static const u16 G_ADDR_ECM_MASK = 0x39ff;
            //mode = &mode_tbl[mode_id]; //if it is necessary to delay the change

            u8 vd = vm_row[vmri].vm_data & vm_mask;
            u8 cd = vm_row[vmri].cr_data & vm_mask;
            u16 addr;

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

            next_pixel_data = bank.r(addr);

            if (gfx_active()) {
                vc = (vc + 1) & 0x3ff;
                ++vmri;
            }
        }

        u8 pixel_out(u8 pixel_time, bool& fg_gfx) { // 8 pixels/cycle --> pixel_time: 0..7
            if (pixel_time == x_scroll) {
                pixel_data = next_pixel_data;
                next_pixel_data = 0; // TODO: best way?
            } else if (((pixel_time - x_scroll) % mode->pixel_width) == 0) { // TODO: nicer?
                pixel_data <<= mode->pixel_width;
            }

            if (v_border_on) {
                return reg[bgc0];
            } else {
                fg_gfx = pixel_data & 0x80;
                return mode->color[pixel_data >> (8 - mode->pixel_width)];
            }
        }

    private:
        static const u16 RC_IDLE   = 0x3fff;
        static const u8  VM_ACTIVE = 0xff;
        static const u8  VM_IDLE   = 0x00;

        void activate_gfx()   { rc = rc & 0x7; vm_mask = VM_ACTIVE; }
        void deactivate_gfx() { rc = RC_IDLE;  vm_mask = VM_IDLE; }

        bool gfx_active() const { return vm_mask != VM_IDLE; }

        u8 ba_line = false;
        u8 frame_den;   // set for each frame (at the top of disp.win)

        struct VM_data { // video-matrix data
            u8 vm_data;
            u8 cr_data; // color-ram data
        };
        VM_data vm_row[40]; // vm row buffer

        u16 vc_base; // video counter base
        u16 vc;      // video counter
        u16 rc = 7;  // row counter
        u8  vmri;    // vm_row index
        u8  vm_mask = 0x00; // 0x00 = gfx_idle, 0xff = gfx_active

        u8 pixel_data = 0x00;
        u8 next_pixel_data = 0x00;

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
        const u8& line_cycle;
        const u16& raster_y;
        const bool& v_border_on;
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


    // checks done during blanking periods (don't necessarely need to be pixel-perfect)
    void check_left_vb()               { if (!vert_border_on) main_border_on = false; }
    void check_right_vb(u16 x)         { if (x == cmp_right) main_border_on = true; }
    void check_hb(u16 raster_y) {
        if (raster_y == cmp_top && den) vert_border_on = false;
        else if (raster_y == cmp_bottom) vert_border_on = true;
    }

    void output_at(u16 x) {
        raster_x = x;
        output();
    }

    void output() {
        for (int pt = 0; pt < 8; ++pt)
            exude_pixel(pt);
    }

    void output_on_edge() {
        for (int pt = 0; pt < 8; ++pt) {
            if (raster_x == cmp_left) {
                if (raster_y == cmp_top && den) vert_border_on = false;
                else if (raster_y == cmp_bottom) vert_border_on = true;
                if (!vert_border_on) main_border_on = false;
            } else if (raster_x == cmp_right) main_border_on = true;
            exude_pixel(pt);
        }
    }

    void exude_pixel(u8 pixel_time) {
        bool gfx_fg = false;
        u8 g_col = gfx_unit.pixel_out(pixel_time, gfx_fg); // must call always (to keep in sync)

        u8 src_mob = 0; // if not transparet then src bit will be set here
        u8 m_col = mob_unit.pixel_out(raster_x, gfx_fg, src_mob); // must call always (to keep in sync)

        // priorites: border < mob|gfx_fg (based on reg[mndp]) < gfx_bg
        u8 o_col;
        if (main_border_on) o_col = reg[ecol];
        else if (m_col != TRANSPARENT) {
            if (!gfx_fg) o_col = m_col;
            else o_col = (reg[mndp] & src_mob) ? g_col : m_col;
        }
        else o_col = g_col;

        out.put_pixel(o_col);

        ++raster_x;
    }


    IRQ_unit irq_unit;
    BA_unit ba_unit;
    MOB_unit mob_unit;
    GFX_unit gfx_unit;

    u8 reg[REG_COUNT];

    u8 line_cycle;
    u8 frame_lp;
    u8 lp_p1_p6_low;
    u8 lp_pb_b4_low;
    u16 raster_x;
    u16 raster_y;
    Beam_area beam_area = v_blank;

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
