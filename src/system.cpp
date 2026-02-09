
#include "system.h"
#include <fstream>



/*
     m  x   g   c   h   l  00-0f 10-7f 80-9f a0-bf c0-cf d0-df e0-ff  pla_idx
    =========================================================================
    31  1   1   1   1   1   RAM   RAM   RAM   BAS   RAM   I/O   KER   : 13

    30  1   1   1   1   0   RAM   RAM   RAM   RAM   RAM   I/O   KER   : 12
    14  0   1   1   1   0   RAM   RAM   RAM   RAM   RAM   I/O   KER

    29  1   1   1   0   1   RAM   RAM   RAM   RAM   RAM   I/O   RAM   : 11
    13  0   1   1   0   1   RAM   RAM   RAM   RAM   RAM   I/O   RAM
    5   0   0   1   0   1   RAM   RAM   RAM   RAM   RAM   I/O   RAM

    27  1   1   0   1   1   RAM   RAM   RAM   BAS   RAM   CHRR  KER   : 10

    26  1   1   0   1   0   RAM   RAM   RAM   RAM   RAM   CHRR  KER   : 09
    10  0   1   0   1   0   RAM   RAM   RAM   RAM   RAM   CHRR  KER

    25  1   1   0   0   1   RAM   RAM   RAM   RAM   RAM   CHRR  RAM   : 08
    9   0   1   0   0   1   RAM   RAM   RAM   RAM   RAM   CHRR  RAM

    23  1   0   1   1   1   RAM   ---   CRL   ---   ---   I/O   CRH   : 07
    22  1   0   1   1   0   RAM   ---   CRL   ---   ---   I/O   CRH
    21  1   0   1   0   1   RAM   ---   CRL   ---   ---   I/O   CRH
    20  1   0   1   0   0   RAM   ---   CRL   ---   ---   I/O   CRH
    19  1   0   0   1   1   RAM   ---   CRL   ---   ---   I/O   CRH
    18  1   0   0   1   0   RAM   ---   CRL   ---   ---   I/O   CRH
    17  1   0   0   0   1   RAM   ---   CRL   ---   ---   I/O   CRH
    16  1   0   0   0   0   RAM   ---   CRL   ---   ---   I/O   CRH

    15  0   1   1   1   1   RAM   RAM   CRL   BAS   RAM   I/O   KER   : 06

    11  0   1   0   1   1   RAM   RAM   CRL   BAS   RAM   CHRR  KER   : 05

    7   0   0   1   1   1   RAM   RAM   CRL   CRH   RAM   I/O   KER   : 04

    6   0   0   1   1   0   RAM   RAM   RAM   CRH   RAM   I/O   KER   : 03

    3   0   0   0   1   1   RAM   RAM   CRL   CRH   RAM   CHRR  KER   : 02

    2   0   0   0   1   0   RAM   RAM   RAM   CRH   RAM   CHRR  KER   : 01

    28  1   1   1   0   0   RAM   RAM   RAM   RAM   RAM   RAM   RAM   : 00
    24  1   1   0   0   0   RAM   RAM   RAM   RAM   RAM   RAM   RAM
    12  0   1   1   0   0   RAM   RAM   RAM   RAM   RAM   RAM   RAM
    8   0   1   0   0   0   RAM   RAM   RAM   RAM   RAM   RAM   RAM
    4   0   0   1   0   0   RAM   RAM   RAM   RAM   RAM   RAM   RAM
    1   0   0   0   0   1   RAM   RAM   RAM   RAM   RAM   RAM   RAM
    0   0   0   0   0   0   RAM   RAM   RAM   RAM   RAM   RAM   RAM

*/

const System::PLA::Array System::PLA::array[14] = {
    /*
    PLA::Array {{
        {ram0_w, ram0_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r},
        {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r},  }},
    PLA::Array {{
        {ram0_w, ram0_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r},
        {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, romh_r}, {ram_w, romh_r}, {ram_w, ram_r}, {ram_w, charr_r}, {ram_w, kern_r}, {ram_w, kern_r},  }},
    PLA::Array {{
        {ram0_w, ram0_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r},
        {ram_w, roml_r}, {ram_w, roml_r}, {ram_w, romh_r}, {ram_w, romh_r}, {ram_w, ram_r}, {ram_w, charr_r}, {ram_w, kern_r}, {ram_w, kern_r},  }},
    PLA::Array {{
        {ram0_w, ram0_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r},
        {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, romh_r}, {ram_w, romh_r}, {ram_w, ram_r}, {io_w, io_r}, {ram_w, kern_r}, {ram_w, kern_r},  }},
    PLA::Array {{
        {ram0_w, ram0_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r},
        {ram_w, roml_r}, {ram_w, roml_r}, {ram_w, romh_r}, {ram_w, romh_r}, {ram_w, ram_r}, {io_w, io_r}, {ram_w, kern_r}, {ram_w, kern_r},  }},
    PLA::Array {{
        {ram0_w, ram0_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r},
        {ram_w, roml_r}, {ram_w, roml_r}, {ram_w, bas_r}, {ram_w, bas_r}, {ram_w, ram_r}, {ram_w, charr_r}, {ram_w, kern_r}, {ram_w, kern_r},  }},
    PLA::Array {{
        {ram0_w, ram0_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r},
        {ram_w, roml_r}, {ram_w, roml_r}, {ram_w, bas_r}, {ram_w, bas_r}, {ram_w, ram_r}, {io_w, io_r}, {ram_w, kern_r}, {ram_w, kern_r},  }},
    PLA::Array {{
        {ram0_w, ram0_r}, {none_w, none_r}, {none_w, none_r}, {none_w, none_r}, {none_w, none_r}, {none_w, none_r}, {none_w, none_r}, {none_w, none_r},
        {roml_w, roml_r}, {roml_w, roml_r}, {none_w, none_r}, {none_w, none_r}, {none_w, none_r}, {io_w, io_r}, {romh_w, romh_r}, {romh_w, romh_r},  }},
    PLA::Array {{
        {ram0_w, ram0_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r},
        {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, charr_r}, {ram_w, ram_r}, {ram_w, ram_r},  }},
    PLA::Array {{
        {ram0_w, ram0_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r},
        {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, charr_r}, {ram_w, kern_r}, {ram_w, kern_r},  }},
    PLA::Array {{
        {ram0_w, ram0_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r},
        {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, bas_r}, {ram_w, bas_r}, {ram_w, ram_r}, {ram_w, charr_r}, {ram_w, kern_r}, {ram_w, kern_r},  }},
    PLA::Array {{
        {ram0_w, ram0_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r},
        {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {io_w, io_r}, {ram_w, ram_r}, {ram_w, ram_r},  }},
    PLA::Array {{
        {ram0_w, ram0_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r},
        {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {io_w, io_r}, {ram_w, kern_r}, {ram_w, kern_r},  }},
    PLA::Array {{
        {ram0_w, ram0_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r},
        {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, bas_r}, {ram_w, bas_r}, {ram_w, ram_r}, {io_w, io_r}, {ram_w, kern_r}, {ram_w, kern_r},  }},
    */
    PLA::Array {{ // 00: modes 0, 1, 4, 8, 12, 24, 28
        { ram0_w,   ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,
          ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w   },
        { ram0_r,   ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,
          ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r   } }},
    PLA::Array {{ // 01: mode 2
        { ram0_w,   ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,
          ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w   },
        { ram0_r,   ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,
          ram_r,    ram_r,    romh_r,   romh_r,   ram_r,    charr_r,  kern_r,   kern_r } }},
    PLA::Array {{ // 02: mode 3
        { ram0_w,   ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,
          ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w   },
        { ram0_r,   ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,
          roml_r,   roml_r,   romh_r,   romh_r,   ram_r,    charr_r,  kern_r,   kern_r } }},
    PLA::Array {{ // 03: mode 6
        { ram0_w,   ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,
          ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    io_w,     ram_w,    ram_w   },
        { ram0_r,   ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,
          ram_r,    ram_r,    romh_r,   romh_r,   ram_r,    io_r,     kern_r,   kern_r } }},
    PLA::Array {{ // 04: mode 7
        { ram0_w,   ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,
          ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    io_w,     ram_w,    ram_w   },
        { ram0_r,   ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,
          roml_r,   roml_r,   romh_r,   romh_r,   ram_r,    io_r,     kern_r,   kern_r } }},
    PLA::Array {{ // 05: mode 11
        { ram0_w,   ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,
          ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w   },
        { ram0_r,   ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,
          roml_r,   roml_r,   bas_r,    bas_r,    ram_r,    charr_r,  kern_r,   kern_r } }},
    PLA::Array {{ // 06: mode 15
        { ram0_w,   ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,
          ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    io_w,     ram_w,    ram_w   },
        { ram0_r,   ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,
          roml_r,   roml_r,   bas_r,    bas_r,    ram_r,    io_r,     kern_r,   kern_r } }},
    PLA::Array {{ // 07: modes 16..23
        { ram0_w,   none_w,   none_w,   none_w,   none_w,   none_w,   none_w,   none_w,
          roml_w,   roml_w,   none_w,   none_w,   none_w,   io_w,     romh_w,   romh_w },
        { ram0_r,   none_r,   none_r,   none_r,   none_r,   none_r,   none_r,   none_r,
          roml_r,   roml_r,   none_r,   none_r,   none_r,   io_r,     romh_r,   romh_r } }},
    PLA::Array {{ // 08: modes 9, 25
        { ram0_w,   ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,
          ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w   },
        { ram0_r,   ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,
          ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    charr_r,  ram_r,    ram_r   } }},
    PLA::Array {{ // 09: modes 10, 26
        { ram0_w,   ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,
          ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w   },
        { ram0_r,   ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,
          ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    charr_r,  kern_r,   kern_r } }},
    PLA::Array {{ // 10: modes 27
        { ram0_w,   ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,
          ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w   },
        { ram0_r,   ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,
          ram_r,    ram_r,    bas_r,    bas_r,    ram_r,    charr_r,  kern_r,   kern_r } }},
    PLA::Array {{ // 11: modes 5, 13, 29
        { ram0_w,   ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,
          ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    io_w,     ram_w,    ram_w   },
        { ram0_r,   ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,
          ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    io_r,     ram_r,    ram_r   } }},
    PLA::Array {{ // 12: modes 14, 30
        { ram0_w,   ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,
          ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    io_w,     ram_w,    ram_w   },
        { ram0_r,   ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,
          ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    io_r,     kern_r,   kern_r } }},
    PLA::Array {{ // 13: mode 31
        { ram0_w,   ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,
          ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    io_w,     ram_w,    ram_w   },
        { ram0_r,   ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,
          ram_r,    ram_r,    bas_r,    bas_r,    ram_r,    io_r,     kern_r,   kern_r } }},
};


void System::Input_matrix::update_matrix() {
    auto get_row = [](int n, u64& from) -> u64 { return (from >> (8 * n)) & 0xff; };
    auto set_row = [](int n, u64& to, u64 val) { to |= (val << (8 * n)); };

    s.kb_matrix = s.key_states;

    if (s.kb_matrix) { // any key down?
        // emulate GND propagation in the matrix (can produce 'ghost' keys)
        for (int ra = 0; ra < 7; ++ra) for (int rb = ra + 1; rb < 8; ++rb) {
            const u64 a = get_row(ra, s.kb_matrix);
            const u64 b = get_row(rb, s.kb_matrix);
            if (a & b) {
                const u64 r = a | b;
                set_row(ra, s.kb_matrix, r);
                set_row(rb, s.kb_matrix, r);
            }
        }
    }
}


void System::Input_matrix::output() {
    u8 pa = s.pa_state & s.cp2_state;
    u8 pb = s.pb_state & s.cp1_state;

    if (s.kb_matrix) { // any key down?
        u64 key = 0b1;
        for (int n = 0; n < 64; ++n, key <<= 1) {
            const bool key_down = s.kb_matrix & key;
            if (key_down) {
                // figure out connected lines & states
                const auto row_bit = (0b1 << (n / 8));
                const auto pa_bit = pa & row_bit;
                const auto col_bit = (0b1 << (n % 8));
                const auto pb_bit = pb & col_bit;

                if (!pa_bit || !pb_bit) { // at least one connected line is low?
                    // pull both lines low
                    pa &= (~row_bit);
                    pb &= (~col_bit);
                }
            }
        }
    }

    pa_in(0b11111111, pa);
    pb_in(0b11111111, pb);

    vic.set_lp(VIC_II::LP_src::cia, pb & VIC_II::CIA1_PB_LP_BIT);
}


void System::Menu::handle_key(u8 code) {
    using kc = Key_code::System;

    if (active) { // TODO: redundant check...?
        switch (code) {
            case kc::menu_ent:  root.enter(); break;
            case kc::menu_back: root.back();  break;
            case kc::menu_up:   root.up();    break;
            case kc::menu_down: root.down();  break;
        }
    }
}


void System::C64::run(Mode init_mode) {
    s.mode = init_mode;

    reset_cold();

    do {
        switch (s.mode) {
            case Mode::none: break;
            case Mode::clocked: run_clocked(); break;
            case Mode::stepped: run_stepped(); break;
        }
        pre_run();
    }
    while (s.mode != Mode::none);
}


void System::C64::check_deferred() {
    if (deferred) {
        deferred();
        deferred = nullptr;
    }
}


/*

L/C: 023[232]/07  B: RRRBRIK  I: [N.]
A:00  X:01  Y:14  SP:F3  P: 81 [N......C]  PC:E5D4 [K]  > BEQ $E5CD

*/
// pla (the 7 zones, e.g. "RRRBRIK" for standard basic mode), irq/nmi status, cycle/frame
// status reg
// pc mapping (e.g. "B", "K", "R", or "L")
void log_status(const State::System& s, System::Bus& bus) {
    char buffer[64];

    const auto sys_status = [&]() {
        const int line = (s.vic.cycle / LINE_CYCLE_COUNT) % FRAME_LINE_COUNT;
        const int line_cycle = s.vic.cycle % LINE_CYCLE_COUNT;

        const char* banking = "RRRBRIK"; // TODO (mapping of the 7 zones)

        const char nmi_status = s.int_hub.nmi_act ? 'N' : '.';
        const char irq_status = s.int_hub.irq_act ? 'I' : '.';
    
        const char* format = "L/C:%03d/%02d  B: %s  I: [%c%c]";
        sprintf(buffer, format, line, line_cycle, banking, nmi_status, irq_status);

        Log::info("%s", buffer);
    };

    const auto cpu_status = [&]() {
        const auto& dr = NMOS6502::MC::code[s.cpu.mcc].dr;
        const auto& pc = s.cpu.pc;

        std::string instr_txt = "";
        if (const bool fetch = (dr == NMOS6502::Ri8::ir); fetch) {
            const auto bytes = Bytes{{bus.peek(pc), bus.peek(pc + 1), bus.peek(pc + 2)}};
            instr_txt = "> " + Dbg::disasm_first(bytes, pc).text;
        } else {
            instr_txt = "> " + NMOS6502::instruction[s.cpu.ir].mnemonic;
        }

        // TODO: pc mapping, e.g. PC:E5D4 [K]  > BEQ $E5CD
        const char* format = "A:%02X  X:%02X  Y:%02X  SP:%02X  P:%02X [..todo..]  PC:%04X  %s";
        sprintf(buffer, format, s.cpu.a, s.cpu.x, s.cpu.y, u8(s.cpu.sp), s.cpu.p, s.cpu.pc,
                    instr_txt.c_str());

        Log::info("%s", buffer);
    };

    sys_status();
    cpu_status();
}


void System::C64::pre_run() {
    switch (s.mode) {
        case Mode::none: break;
        case Mode::clocked:
            vid_out.flip();
            sid.flush();
            frame_timer.reset();
            watch.start();
            break;
        case Mode::stepped:
            sid.flush();
            log_status(s, bus);
            break;
    }
}


void System::C64::run_clocked() {
    auto sync = [&]() {
        const auto frame_duration = [&]() { return Timer::one_second() / perf.frame_rate.chosen; };

        const bool frame_done = (s.vic.cycle % FRAME_CYCLE_COUNT) == 0;
        if (frame_done) {
            watch.stop();
            
            if (vid_out.v_synced()) {
                output_frame();
                frame_timer.reset();
            } else {
                frame_timer.wait_elapsed(frame_duration(), true);
                output_frame();
            }
            
            watch.start();
            
            sid.output();

            host_input.poll();
            
            watch.stop();
            watch.reset();

            check_deferred();
        } else {
            watch.stop();
            
            const auto frame_progress = double(s.vic.raster_y) / double(FRAME_LINE_COUNT);
            const auto frame_progress_time = frame_progress * frame_duration();
            frame_timer.wait_elapsed(frame_progress_time);
            
            watch.start();
            
            sid.output();

            host_input.poll();
        }
    };

    // TODO: consider special loops for different configs (e.g. REU with no 1541)
    while (s.mode == Mode::clocked) {
        run_cycle();
        const bool sync_point = (s.vic.cycle % perf.latency.chosen.sync_freq) == 0;
        if (sync_point) sync();
    }
}


void System::C64::run_stepped() {
    frame_timer.reset();

    while (s.mode == Mode::stepped) {
        output_frame();
        frame_timer.wait_elapsed(Timer::one_second() / 50.0, true);
        host_input.poll();
        check_deferred();
    }
}


// TODO: when stepping cycle/instr/line, add a beam_pos indicator (line/dot?)
void System::C64::step_forward(u8 key_code) {
    using kc = Key_code::System;

    // 'clear' remaining pixels (yes, most of the time this is redundant, but meh... )
    for (auto bp = s.vic.beam_pos; bp < VIC_II::FRAME_SIZE; ++bp)
        s.vic.frame[bp] = Color::black;

    switch (key_code) {
        case kc::step_cycle:
            run_cycle();
            break;
        case kc::step_instr:
            do run_cycle(); while (cpu.mop().dr != NMOS6502::Ri8::ir);
            break;
        case kc::step_line:
            do run_cycle(); while (s.vic.line_cycle() < (LINE_CYCLE_COUNT - 1));
            break;
        case kc::step_frame:
            do run_cycle(); while (s.vic.frame_cycle() < (FRAME_CYCLE_COUNT - 1));
            break;
    }

    sid.output();

    log_status(s, bus);
}


struct PETSCII_Draw { // user is trusted, no checks...
    const u8* charrom;
    u8* tgt;
    const u16 tgt_w = VIC_II::FRAME_WIDTH;

    //void clear(u8 col) { for (int p = 0; p < tgt_w * tgt_h; ++p) tgt[p] = col; }

    void chr(u16 chr, u16 cx, u16 cy, Color fg, Color bg) {
        const u8* src = &charrom[chr * 8];
        for (int px_row = 0; px_row < 8; ++px_row, ++src) {
            u8* t = &tgt[(px_row + cy) * tgt_w + cx];
            for (u8 px = 0b10000000; px; px >>= 1) *t++ = (*src & px) ? fg : bg;
        }
    }

    void txt(const std::string& txt, u16 tx, u16 ty, Color fg, Color bg) {
        for (u8 ci = 0; ci < txt.length(); ++ci, tx += 8) {
            const auto c = (txt[ci] % 64) + 256; // map ascii code to petscii (using the 'lower case' half)
            chr(c, tx, ty, fg, bg);
        }
    }
};


void System::C64::output_frame() {
    static const int menu_pos_y = (VIC_II::FRAME_HEIGHT - VIC_II::BORDER_SZ_H) + 12;

    auto draw_menu = [&]() {
        static const int pos_x = VIC_II::BORDER_SZ_V + 4;
        static const int width_chr = 36;
        static const int pad_px = 4;
        static const Color col_fg = Color::light_green;
        static const Color col_bg = Color::gray_1;

        PETSCII_Draw pd{rom.charr, s.vic.frame};

        pd.txt(std::string(width_chr, ' '), pos_x, menu_pos_y, col_fg, col_bg);
        pd.txt(menu.text(), pos_x + pad_px, menu_pos_y, col_fg, col_bg);

        /*
        // draw char.rom (fun & profit...)
        const int x = 64;
        const int y = 64 + 1;

        for (u16 c = 0; c < 512; ++c) {
            const int col = c % 32;
            const int row = c / 32;
            pd.chr(c, x + (col * 8), y + (row * 8), col_fg, col_bg);
        }
        */
    };

    auto draw_c1541_status = [&]() {
        static const int pos_x = VIC_II::FRAME_WIDTH - VIC_II::BORDER_SZ_V - 22;
        static const int pos_y = menu_pos_y;
        static const Color col_bg = Color::black;
        static const Color col_led_on = Color::light_green;
        static const Color col_led_off = Color::gray_1;

        static const u16 ch_led_wp = 0x0051;
        static const u16 ch_led    = 0x0057;

        const auto led_ch = c1541.dc.status.write_prot_on() ? ch_led_wp : ch_led;
        const auto led_col = c1541.dc.status.led_on() ? col_led_on : col_led_off;

        PETSCII_Draw{rom.charr, s.vic.frame}.chr(led_ch, pos_x, pos_y, led_col, col_bg);

        /*static constexpr u16 ch_zero   = 0x0030;
        const auto track_n = (status.head.track_n / 2) + 1;
        const auto tn_col = status.head.active() ? Color::green : Color::gray_2;
        draw_char(ch_zero + track_n / 10, x + 8 + 2,  y, tn_col, col_bg);
        draw_char(ch_zero + track_n % 10, x + 16 + 2, y, tn_col, col_bg);*/
    };

    auto draw_expansion_status = [&]() {
        static const int pos_x = VIC_II::FRAME_WIDTH - VIC_II::BORDER_SZ_V - 12;
        static const int pos_y = menu_pos_y;
        static const Color col_bg = Color::black;
        static const Color col_attached = Color::green;
        static const Color col_detached = Color::gray_1;
        static const u16 ch = 0x118;

        const auto col = (s.exp.type == Expansion::Type::none) ? col_detached : col_attached;

        PETSCII_Draw{rom.charr, s.vic.frame}.chr(ch, pos_x, pos_y, col, col_bg);
    };

    if (menu.active) {
        draw_menu();
        draw_c1541_status();
        draw_expansion_status();
    } else if (c1541.dc.status.head.active()) {
        draw_c1541_status();
    }

    vid_out.put(s.vic.frame);
}


void System::C64::reset_warm() {
    Log::info("System reset (warm)");
    cia1.reset_warm(); // need to reset for correct irq handling
    cia2.reset_warm();
    cpu.reset();
    int_hub.reset();
}


void System::C64::reset_cold() {
    Log::info("System reset (cold)");

    auto init_ram = [&]() { // TODO: parameterize pattern (+ add 'randomness'?)
        for (int addr = 0x0000; addr <= 0xffff; ++addr)
            s.ram[addr] = (addr & 0x80) ? 0xff : 0x00;
    };

    init_ram();

    s.ba = false;
    s.dma = false;

    cia1.reset_cold();
    cia2.reset_cold();
    sid.reset();
    vic.reset();
    input_matrix.reset();
    bus.reset();
    cpu.reset();
    int_hub.reset();
    c1541.reset();

    pre_run();
}


void System::C64::save_state_req() {
    static const std::string dir = "./_local"; // TODO...

    // TODO: spawn thread (a copy of sys_snap required though...)?
    deferred = [&]() {
        sys_snap.sid = sid.core.read_state();

        const std::string filepath = as_lower(dir + "/emu.state"); // TODO...
        // TODO: hadle exceptions?
        if (auto f = std::ofstream(filepath, std::ios::binary)) {
            f.write((const char*)&sys_snap, sizeof(sys_snap));
            if (f) Log::info("State saved: %s", filepath.c_str());
            else Log::error("save state failed");
        }
    };
};


bool System::C64::handle_file(Files::File& file) {
    auto inject = [&](const Bytes& data) {
        // load addr (used if 2nd.addr == 0)
        s.ram[0xc3] = cpu.s.x;
        s.ram[0xc4] = cpu.s.y;

        const u8 scnd_addr = s.ram[0xb9];
        u16 addr = (scnd_addr == 0)
            ? cpu.s.y * 0x100 + cpu.s.x
            : data[1] * 0x100 + data[0];

        // 'load'
        for (u32 b = 2; b < data.size(); ++b) s.ram[addr++] = data[b];

        // end pointer
        s.ram[0xae] = cpu.s.x = addr;
        s.ram[0xaf] = cpu.s.y = addr >> 8;
    };

    using Type = Files::File::Type;

    // NOTE: 'crt' & 'sys_snap' loads are deferred, because otherwise we could be
    //       jumping in mid-cycle (because loading can be triggerd also by the cpu.tick())
    switch (file.type) {
        case Type::crt: {
            Log::info("CRT '%s' ...", file.name.c_str());
            deferred = [&, data = std::move(file.data)]() {
                Expansion::attach(s, Files::CRT{data});
                reset_cold();
            };
            return true;
        }
        case Type::d64:
            // auto slot = s.ram[0xb9]; // secondary address
            // slot=0 --> first free slot
            c1541.disk_carousel.insert(0,
                    new C1541::D64(Files::D64{file.data}), file.name);
            return true;
        case Type::g64:
            c1541.disk_carousel.insert(0,
                    new C1541::G64(std::move(file.data)), file.name);
            return true;
        case Type::c64_bin:
            inject(file.data);
            return true;
        case Type::sys_snap: {
            deferred = [&, d = std::move(file.data)]() {
                Files::System_snapshot& iss = *((Files::System_snapshot*)d.data()); // brutal...
                sys_snap.sys_state = iss.sys_state;
                sid.core.write_state(iss.sid);
                pre_run(); // NOTE: required for now (see 'sid.h' for more info)
            };
            return true;
        }
        default:
            Log::info("'%s' ignored", file.name.c_str());
            return false;
    }
    // TODO: support 't64' (e.g. cold reset & feed the appropriate 'LOAD'
    // command + 'RETURN' key into the C64 keyboard input buffer)
};


std::string get_filename(const u8* ram) {
    const u16 filename_addr = ram[0xbc] * 0x100 + ram[0xbb];
    const u8 filename_len = ram[0xb7];
    return std::string(&ram[filename_addr], &ram[filename_addr + filename_len]);
}


void System::C64::do_load() {
    // TODO: use secondary address as an action id, e.g.:
    //           'LOAD "SOME.CRT",1,2' --> inspect only (i.e. generate_basic_info_list & inject)
    auto file = loader(get_filename(s.ram));
    if (file.identified() && handle_file(file)) {
        if (auto info_file = generate_basic_info_list(file); info_file) {
            // TOFIX: Here we rely on the fact, that 'generate_basic_info_list'
            //        currently supports only D64s (so for example we don't corrupt
            //        a restored snapshot with an injected info listing...)
            handle_file(info_file);
        }

        //'return' status to kernal routine
        cpu.s.clr(NMOS6502::Flag::C); // no error
        s.ram[0x90] = 0x00; // io status ok
    } else {
        cpu.s.pc = 0xf704; // --> file not found
    }
}


// TODO: identify the addresses used
void System::C64::do_save() {
    static const std::string dir = "./_local"; // TODO...

    auto do_save = [](const std::string& filename, u16 start_addr, u8* data, size_t sz) -> bool {
        // TODO: hadle exceptions?
        if (auto f = std::ofstream(filename, std::ios::binary)) {
            f << (u8)(start_addr) << (u8)(start_addr >> 8);
            f.write((char*)data, sz);
            if (f) return true;
        }

        return false;
    };

    const std::string filename = get_filename(s.ram);
    if (filename.length() == 0) {
        // TODO: error_code enum(s)
        cpu.s.a = 0x08; // missing filename
        cpu.s.set(NMOS6502::Flag::C); // error
        return;
    }

    const std::string filepath = as_lower(dir + "/" + filename);

    // TODO: end < start ??
    const u16 start_addr = s.ram[0xc2] * 0x100 + s.ram[0xc1];
    const u16 end_addr = s.ram[0xaf] * 0x100 + s.ram[0xae]; // 1 beyond end
    const u16 sz = end_addr - start_addr;

    if (do_save(filepath, start_addr, &s.ram[start_addr], sz)) {
        // status
        cpu.s.clr(NMOS6502::Flag::C); // no error
        s.ram[0x90] = 0x00; // io status ok
        Log::info("Saved '%s', %d bytes", filepath.c_str(), int(sz + 2));
    } else {
        cpu.s.a = 0x07; // not output file
        cpu.s.set(NMOS6502::Flag::C); // error
        Log::error("Failed to save '%s'", filename.c_str());
    }
}


// 'halt' instructions used for trapping (i.e. 'illegal' instructions that normally
// would halt the CPU). The byte following the 'halt' is used to identify the 'request'.
void System::C64::install_kernal_tape_traps(u8* kernal, u8 trap_opc) {
    static constexpr u16 kernal_start = 0xe000;

    // trap loading (device 1)
    kernal[0xf539 - kernal_start] = trap_opc;
    kernal[0xf53a - kernal_start] = Trap_ID::load;
    kernal[0xf53b - kernal_start] = NMOS6502::OPC::rts;

    // trap saving (device 1)
    kernal[0xf65f - kernal_start] = trap_opc;
    kernal[0xf660 - kernal_start] = Trap_ID::save;
    kernal[0xf661 - kernal_start] = NMOS6502::OPC::rts;
}
