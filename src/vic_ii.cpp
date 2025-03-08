#include "vic_ii.h"


void VIC_II::Core::MOBs::do_dma(u8 mn)  { // do p&s -accesses
    VS::MOB& m = mob[mn];
    // if dma_on, then cpu has already been stopped --> safe to read all at once (1p + 3s)
    if (!MOB{m}.dma_off()) {
        if (MOB{m}.dma_on()) {
            if (m.disp_on) {
                const u16 mb = ((reg[R::mptr] & MPTR::vm) << 6) | MP_BASE;
                const u16 mp = addr_space.r(mb | mn) << 6;
                MOB{m}.load_data(
                    addr_space.r(mp | ((m.mdc + 0) & 63)),
                    addr_space.r(mp | ((m.mdc + 1) & 63)),
                    addr_space.r(mp | ((m.mdc + 2) & 63))
                );
            }
            m.mdc = (m.mdc + 3) & 63;
            ba.mob_done(mn);
        } else { // in 'dma done' mode
            MOB{m}.set_dma_off();
            m.disp_on = false;
        }
    }
}


void VIC_II::Core::MOBs::update(u16 x) { // update during blanks
    for (int mn = 0; mn < mob_count; ++mn) {
        if (mob[mn].data) {
            _update(mn, x);
            return;
        }
    }
}


void VIC_II::Core::MOBs::output(u16 x, u8* to) {
    for (int mn = mob_count - 1; mn >= 0; --mn) {
        if (mob[mn].data) {
            _output(mn, x, to);
            return;
        }
    }
}


void VIC_II::Core::GFX::read_vm() {
    if (gs.ba_line) {
        activate(); // delayed - possibly (in case 'DMA delay' was triggered)
        if (ba.phi2_aec_high(vs.line_cycle())) {
            gs.vm[gs.vmri].data = 0xff;
            /*
            'The MOS 6567/6569 video controller (VIC-II) and its application in the Commodore 64 (by Christian Bauer)' states:
                In the first three cycles after BA went low, the VIC reads $ff as character pointers
                and as color information the lower 4 bits of the opcode after the access to $d011.
                Not until then, regular video matrix data is read.

            'The C64 PLA Dissected (by Thomas ’skoe’ Giese)' elaborates:
                If write accesses by the CPU address the I/O area during this phase, the PLA selects
                the I/O chips accordingly. But to make sure that the (dummy) read cycles in this
                phase will never accidentally acknowledge an interrupt, the PLA redirects them to RAM.
            */
            gs.vm[gs.vmri].col = 0; // TODO: 'col = ram[cpu.mar()] & 0b1111'
        } else {
            col_ram.r(gs.vc, gs.vm[gs.vmri].col); // TODO: mask upper bits if 'noise' is implemented
            const u16 vaddr = ((reg[R::mptr] & MPTR::vm) << 6) | gs.vc;
            gs.vm[gs.vmri].data = addr_space.r(vaddr);
        }
    }
}


void VIC_II::Core::GFX::read_gd() {
    if (active()) {
        gs.gd = addr_space.r(gs.gfx_addr);
        gs.vc = (gs.vc + 1) & 0x3ff;
        gs.vmri += 1;
    } else {
        const u16 addr = vs.cr1(CR1::ecm) ? (addr_idle & addr_ecm_mask) : addr_idle;
        gs.gd = addr_space.r(addr);
    }
}


void VIC_II::Core::GFX::feed_pipeline() {
    const auto xs = vs.cr2(CR2::x_scroll);
    const u64 gd_slot = u64(0x000000000000ffff) << (40 - xs);
    gs.pipeline &= ~gd_slot; // clear leftovers (in case xs was decremented)
    gs.pipeline |= (u64(gs.gd) << (48 - xs)); // feed gd
    gs.pipeline |= (0b1 << (8 - xs)); // feed timer
}


void VIC_II::Core::GFX::output(u8* to) {
    static constexpr u64 vmd_timer = u64(0b1) << 16;

    const auto put = [&to](const u8 px) { *to++ = px; };

    const auto vma_started = [this]() { return gs.vmri > 0; };

    int shifts = 8;
    const auto shift = [&, this]() {
        gs.pipeline <<= 1;
        if (gs.pipeline & vmd_timer) {
            // 'timer' bit continues travel as an 'even shift' indicator
            gs.pipeline &= 0xfffff8000001ffff; // (also, old one gets cleared)
            if (vma_started()) gs.vmd = gs.vm[gs.vmoi++];
        }
        return --shifts;
    };

    const auto latch_2bit_px = [this]() {
        if (gs.pipeline & 0x0000000001550000) gs.px = gs.pipeline >> 62;
    };

    const u16 c_base = (reg[R::mptr] & MPTR::cg) << 10;

    if (gs.blocked) gs.pipeline &= 0x00ffffffffffffff; // flush gd (just the upcoming 8 bits)

    switch (gs.mode) {
        case scm:
            do put(gs.pipeline & 0x8000000000000000
                    ? gs.vmd.col | foreground
                    : reg[R::bgc0]);
            while (shift());

            gs.gfx_addr = c_base | (gs.vm[gs.vmri].data << 3) | gs.rc;
            return;

        case mccm:
            if (gs.vmd.col & multicolor) {
                do {
                    latch_2bit_px();

                    {
                        const u8 col = gs.px == 0b11
                                        ? gs.vmd.col & 0x7
                                        : reg[R::bgc0 + (gs.px)];
                        put(col | (gs.px << 6)); // sets also fg-gfx flag
                    }

                    gs.pipeline <<= 1;
                    if (gs.pipeline & vmd_timer) {
                        gs.pipeline &= 0xfffff8000001ffff;
                        if (vma_started()) gs.vmd = gs.vm[gs.vmoi++];
                        if (!(gs.vmd.col & multicolor)) goto _scm;
                    }
                    _mccm:
                    --shifts;
                } while (shifts);
            } else {
                do {
                    put(gs.pipeline & 0x8000000000000000
                            ? gs.vmd.col | foreground
                            : reg[R::bgc0]);

                    gs.pipeline <<= 1;
                    if (gs.pipeline & vmd_timer) {
                        gs.pipeline &= 0xfffff8000001ffff;
                        if (vma_started()) gs.vmd = gs.vm[gs.vmoi++];
                        if (gs.vmd.col & multicolor) goto _mccm;
                    }
                    _scm:
                    --shifts;
                } while (shifts);
            }

            gs.gfx_addr = c_base | (gs.vm[gs.vmri].data << 3) | gs.rc;
            return;

        case sbmm:
            do put(gs.pipeline & 0x8000000000000000
                        ? (gs.vmd.data >> 4) | foreground
                        : (gs.vmd.data & 0xf));
            while (shift());

            gs.gfx_addr = (c_base & 0x2000) | (gs.vc << 3) | gs.rc;
            return;

        case mcbmm:
            do {
                latch_2bit_px();

                switch (gs.px) {
                    case 0b00: put(reg[R::bgc0]);                     break;
                    case 0b01: put(gs.vmd.data >> 4);                 break;
                    case 0b10: put((gs.vmd.data & 0xf) | foreground); break;
                    case 0b11: put(gs.vmd.col | foreground);          break;
                }
            } while (shift());

            gs.gfx_addr = (c_base & 0x2000) | (gs.vc << 3) | gs.rc;
            return;

        case ecm:
            do put(gs.pipeline & 0x8000000000000000
                        ? gs.vmd.col | foreground
                        : reg[R::bgc0 + (gs.vmd.data >> 6)]);
            while (shift());

            gs.gfx_addr = (c_base | (gs.vm[gs.vmri].data << 3) | gs.rc)
                                & addr_ecm_mask;
            return;

        case icm:
            if (gs.vmd.col & multicolor) {
                do {
                    latch_2bit_px();

                    put(gs.px << 6); // sets col.0 & fg-gfx flag

                    gs.pipeline <<= 1;
                    if (gs.pipeline & vmd_timer) {
                        gs.pipeline &= 0xfffff8000001ffff;
                        if (vma_started()) gs.vmd = gs.vm[gs.vmoi++];
                        if (!(gs.vmd.col & multicolor)) goto _icm_scm;
                    }
                    _icm_mccm:
                    --shifts;
                } while (shifts);
            } else {
                do {
                    put((gs.pipeline >> 56) & 0x80); // sets col.0 & fg-gfx flag

                    gs.pipeline <<= 1;
                    if (gs.pipeline & vmd_timer) {
                        gs.pipeline &= 0xfffff8000001ffff;
                        if (vma_started()) gs.vmd = gs.vm[gs.vmoi++];
                        if (gs.vmd.col & multicolor) goto _icm_mccm;
                    }
                    _icm_scm:
                    --shifts;
                } while (shifts);
            }

            gs.gfx_addr = (c_base | (gs.vm[gs.vmri].data << 3) | gs.rc)
                                & addr_ecm_mask;
            return;

        case ibmm1:
            do put((gs.pipeline >> 56) & 0x80); while (shift()); // sets col.0 & fg-gfx flag

            gs.gfx_addr = ((c_base & 0x2000) | (gs.vc << 3) | gs.rc)
                                & addr_ecm_mask;
            return;

        case ibmm2:
            do {
                latch_2bit_px();
                put(gs.px << 6); // sets col.0 & fg-gfx flag
            } while (shift());

            gs.gfx_addr = ((c_base & 0x2000) | (gs.vc << 3) | gs.rc)
                                & addr_ecm_mask;
            return;
    }
}


void VIC_II::Core::GFX::output_border(u8* to) {
    const auto put = [&](const u8 c) {
        to[0] = to[1] = to[2] = to[3] = to[4] = to[5] = to[6] = to[7] = c;
    };

    switch (gs.mode) {
        case scm: case mccm:
            put(reg[R::bgc0]);
            return;
        case sbmm:
            put(gs.vmd.data & 0xf);
            return;
        case mcbmm:
            put(reg[R::bgc0]);
            return;
        case ecm:
            put(reg[R::bgc0 + (gs.vmd.data >> 6)]);
            return;
        case icm: case ibmm1: case ibmm2:
            put(Color::black);
            return;
    }
}


void VIC_II::Core::output_border() {
    gfx.output_border(beam_ptr());
    mobs.output(s.raster_x(), beam_ptr());
    s.beam_pos += 8;
    border.output(s.beam_pos);
}


void VIC_II::Core::output() {
    gfx.output(beam_ptr());
    mobs.output(s.raster_x(), beam_ptr());
    s.beam_pos += 8;
    border.output(s.beam_pos);
}


void VIC_II::Core::tick() {
    using V_blank = VS::V_blank;

    ++s.cycle;

    lp.tick();

    switch (s.line_cycle() + s.v_blank) {
        case  0:
            mobs.do_dma(2);
            update_mobs();

            ++s.raster_y;
            check_raster_irq();

            // TODO: reveal the magic numbers....
            if (s.raster_y == GFX::vma_start_ry) {
                gfx.vma_start();
            } else if (s.raster_y == GFX::vma_done_ry) {
                gfx.vma_done();
            } else if (s.raster_y == (250 + BORDER_SZ_H)) {
                s.v_blank = V_blank::vb_on;
            }

            gfx.ba_init();

            return;
        case  1: mobs.prep_dma(5); update_mobs(); return;
        case  2: mobs.do_dma(3);   update_mobs(); return;
        case  3: mobs.prep_dma(6); update_mobs(); return;
        case  4: mobs.do_dma(4);   update_mobs(); return;
        case  5: mobs.prep_dma(7); update_mobs(); return;
        case  6: mobs.do_dma(5);
            // fall through
        case  7: update_mobs(); return;
        case  8: mobs.do_dma(6);
            // fall through
        case  9: update_mobs(); return;
        case 10:
            mobs.do_dma(7);
            update_mobs();
            return;
        case 11: gfx.ba_start();
            // fall through
        case 12:
            update_mobs();
            return;
        case 13:
            output_border();
            gfx.row_start();
            return;
        case 14:
            output_border();
            gfx.init_vmd();
            gfx.read_vm();
            return;
        case 15:
            output();
            gfx.read_gd();
            gfx.read_vm();
            mobs.upd_dma();
            return;
        case 16:
            gfx.feed_pipeline();
            border.check_left(CR2::csel);
            output();
            gfx.read_gd();
            gfx.read_vm();
            return;
        case 17:
            gfx.feed_pipeline();
            border.check_left(CR2::csel ^ CR2::csel);
            output();
            gfx.read_gd();
            gfx.read_vm();
            return;
        case 18: case 19:
        case 20: case 21: case 22: case 23: case 24: case 25: case 26: case 27: case 28: case 29:
        case 30: case 31: case 32: case 33: case 34: case 35: case 36: case 37: case 38: case 39:
        case 40: case 41: case 42: case 43: case 44: case 45: case 46: case 47: case 48: case 49:
        case 50: case 51: case 52: case 53:
            gfx.feed_pipeline();
            output();
            gfx.read_gd();
            gfx.read_vm();
            return;
        case 54:
            gfx.feed_pipeline();
            output();
            gfx.read_gd();
            gfx.ba_done();
            mobs.check_dma_ye();
            mobs.check_dma_start();
            mobs.prep_dma(0);
            return;
        case 55:
            gfx.feed_pipeline();
            border.check_right(CR2::csel ^ CR2::csel);
            output();
            mobs.check_dma_start();
            //mobs.prep_dma(0); ????
            return;
        case 56:
            border.check_right(CR2::csel);
            output();
            mobs.prep_dma(1);
            return;
        case 57:
            mobs.check_disp();
            output();
            gfx.row_done();
            return;
        case 58:
            output_border();
            mobs.prep_dma(2);
            return;
        case 59:
            mobs.do_dma(0);
            output_border();
            return;
        case 60:
            output_border();
            mobs.prep_dma(3);
            return;
        case 61:
            mobs.do_dma(1);
            update_mobs();
            return;
        case 62:
            border.line_done();
            mobs.prep_dma(4);
            update_mobs();
            return;
        case  0 + V_blank::vb_on:
            mobs.do_dma(2);
            update_mobs();

            if (s.raster_y < (FRAME_LINE_COUNT - 1)) {
                ++s.raster_y;
                check_raster_irq();

                if (s.raster_y == (50 - BORDER_SZ_H)) {
                    s.v_blank = V_blank::vb_off;
                    border.vb_end();
                }
            } // else raster_y updated in the next cycle

            return;
        case  1 + V_blank::vb_on:
            mobs.prep_dma(5);
            update_mobs();
            if (s.frame_cycle() == 1) { // last cycle of the (extended) last line?
                s.raster_y = 0;
                check_raster_irq();
                s.beam_pos = 0;
                lp.reset();
            }
            return;
        case  2 + V_blank::vb_on: mobs.do_dma(3);   update_mobs(); return;
        case  3 + V_blank::vb_on: mobs.prep_dma(6); update_mobs(); return;
        case  4 + V_blank::vb_on: mobs.do_dma(4);   update_mobs(); return;
        case  5 + V_blank::vb_on: mobs.prep_dma(7); update_mobs(); return;
        case  6 + V_blank::vb_on: mobs.do_dma(5);
            // fall through
        case  7 + V_blank::vb_on: update_mobs(); return;
        case  8 + V_blank::vb_on: mobs.do_dma(6);
            // fall through
        case  9 + V_blank::vb_on: update_mobs(); return;
        case 10 + V_blank::vb_on: mobs.do_dma(7);
            // fall through
        case 11 + V_blank::vb_on: case 12 + V_blank::vb_on: case 13 + V_blank::vb_on: case 14 + V_blank::vb_on:
            update_mobs();
            return;
        case 15 + V_blank::vb_on:
            mobs.upd_dma();
            // fall through
        case 16 + V_blank::vb_on: case 17 + V_blank::vb_on: case 18 + V_blank::vb_on: case 19 + V_blank::vb_on:
        case 20 + V_blank::vb_on: case 21 + V_blank::vb_on: case 22 + V_blank::vb_on: case 23 + V_blank::vb_on: case 24 + V_blank::vb_on: case 25 + V_blank::vb_on: case 26 + V_blank::vb_on: case 27 + V_blank::vb_on: case 28 + V_blank::vb_on: case 29 + V_blank::vb_on:
        case 30 + V_blank::vb_on: case 31 + V_blank::vb_on: case 32 + V_blank::vb_on: case 33 + V_blank::vb_on: case 34 + V_blank::vb_on: case 35 + V_blank::vb_on: case 36 + V_blank::vb_on: case 37 + V_blank::vb_on: case 38 + V_blank::vb_on: case 39 + V_blank::vb_on:
        case 40 + V_blank::vb_on: case 41 + V_blank::vb_on: case 42 + V_blank::vb_on: case 43 + V_blank::vb_on: case 44 + V_blank::vb_on: case 45 + V_blank::vb_on: case 46 + V_blank::vb_on: case 47 + V_blank::vb_on: case 48 + V_blank::vb_on: case 49 + V_blank::vb_on:
        case 50 + V_blank::vb_on: case 51 + V_blank::vb_on: case 52 + V_blank::vb_on: case 53 + V_blank::vb_on:
            update_mobs();
            return;
        case 54 + V_blank::vb_on:
            update_mobs();
            mobs.check_dma_ye();
            mobs.check_dma_start();
            mobs.prep_dma(0);
            return;
        case 55 + V_blank::vb_on:
            update_mobs();
            border.check_right_vb(CR2::csel ^ CR2::csel);
            mobs.check_dma_start();
            // mobs.prep_dma(0); ???
            return;
        case 56 + V_blank::vb_on:
            mobs.prep_dma(1);
            update_mobs();
            border.check_right_vb(CR2::csel);
            return;
        case 57 + V_blank::vb_on:
            mobs.check_disp();
            update_mobs();
            return;
        case 58 + V_blank::vb_on:
            mobs.prep_dma(2);
            update_mobs();
            return;
        case 59 + V_blank::vb_on:
            mobs.do_dma(0);
            update_mobs();
            return;
        case 60 + V_blank::vb_on:
            mobs.prep_dma(3);
            update_mobs();
            return;
        case 61 + V_blank::vb_on:
            mobs.do_dma(1);
            update_mobs();
            return;
        case 62 + V_blank::vb_on:
            mobs.prep_dma(4);
            update_mobs();
            return;
    }
}