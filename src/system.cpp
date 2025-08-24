
#include "system.h"
#include <fstream>



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

    vic.set_lp(VIC::LP_src::cia, pb & VIC_II::CIA1_PB_LP_BIT);
}


void System::Video_overlay::draw_chr(u16 chr, u16 cx, u16 cy, Color fg, Color bg) {
    const u8* src = &charrom[chr * 8];
    for (int px_row = 0; px_row < 8; ++px_row, ++src) {
        u8* tgt = &pixels[(px_row + cy) * w + cx];
        for (u8 px = 0b10000000; px; px >>= 1) *tgt++ = (*src & px) ? fg : bg;
    }
}


void System::Menu::handle_key(u8 code) {
    using kc = Key_code::System;

    if (video_overlay.visible) {
        switch (code) {
            case kc::menu_ent:  root.enter(); break;
            case kc::menu_back: root.back();  break;
            case kc::menu_up:   root.up();    break;
            case kc::menu_down: root.down();  break;
        }
    } else {
        video_overlay.visible = true;
    }

    update();
}


void System::Menu::toggle(bool on)  {
    video_overlay.visible = on;
    if (on) update();
}


void System::Menu::update() {
    video_overlay.clear(col_bg);

    const std::string text = root.text();
    auto x = 4;
    for (u8 ci = 0; ci < text.length(); ++ci, x += 8) {
        const auto c = (text[ci] % 64) + 256; // map ascii code to petscii (using the 'lower case' half)
        video_overlay.draw_chr(c, x, 0, col_fg, col_bg);
    }
}


void System::C1541_status_panel::update(const C1541::Disk_ctrl::Status& c1541_status) {
    static const u16 ch_led_wp = 0x0051;
    static const u16 ch_led    = 0x0057;

    const auto led_ch = c1541_status.wp_off() ? ch_led : ch_led_wp;
    const auto led_col = c1541_status.led_on() ? col_led_on : col_led_off;
    video_overlay.draw_chr(led_ch, 0, 0, led_col, col_bg);

    /*static constexpr u16 ch_zero   = 0x0030;
    const auto track_n = (status.head.track_n / 2) + 1;
    const auto tn_col = status.head.active() ? Color::green : Color::gray_2;
    draw_char(ch_zero + track_n / 10, x + 8 + 2,  y, tn_col, col_bg);
    draw_char(ch_zero + track_n % 10, x + 16 + 2, y, tn_col, col_bg);*/
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
            do run_cycle(); while (cpu.mcp->dr != NMOS6502::Ri8::ir);
            break;
        case kc::step_line:
            do run_cycle(); while (s.vic.line_cycle() < (LINE_CYCLE_COUNT - 1));
            break;
        case kc::step_frame:
            do run_cycle(); while (s.vic.frame_cycle() < (FRAME_CYCLE_COUNT - 1));
            break;
    }

    sid.output();
}


void System::C64::output_frame() {
    auto output_overlay = [&](Video_overlay& ol) {
        const u8* src = ol.pixels;
        for (int y = 0; y < ol.h; ++y) {
            u8* tgt = &s.vic.frame[((ol.y + y) * VIC_II::FRAME_WIDTH) + ol.x];
            for (int x = 0; x < ol.w; ++x, ++src) {
                const auto col = *(src);
                /*if (col != Video_overlay::transparent)*/ tgt[x] = col;
            }
        }
    };

    if (menu.video_overlay.visible) {
        output_overlay(menu.video_overlay);
        c1541_status_panel.update(c1541.dc.status);
        output_overlay(c1541_status_panel.video_overlay);
    } else if (c1541.dc.status.head.active()) {
        c1541_status_panel.update(c1541.dc.status);
        output_overlay(c1541_status_panel.video_overlay);
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
    cia1.reset_cold();
    cia2.reset_cold();
    sid.reset();
    vic.reset();
    input_matrix.reset();
    bus.reset();
    cpu.reset();
    int_hub.reset();
    c1541.reset();
    exp_ctx.reset();

    s.ba_low = false;
    s.dma_low = false;

    pre_run();
}


void System::C64::save_state_req() {
    static const std::string dir = "./_local"; // TODO...

    // TODO: spawn thread (a copy of sys_snap required though...)?
    deferred = [&]() {
        sys_snap.sid = sid.core.read_state();
        sys_snap.cpu_mcp = cpu.mcp - &NMOS6502::MC::code[0];

        const std::string filepath = as_lower(dir + "/emu.state"); // TODO...
        // TODO: hadle exceptions?
        if (auto f = std::ofstream(filepath, std::ios::binary)) {
            f.write((const char*)&sys_snap, sizeof(sys_snap));
            if (!f) Log::error("save state failed");
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
                const Files::CRT crt{data};
                if (Cartridge::attach(crt, exp_ctx)) reset_cold();
            };
            return true;
        }
        case Type::d64:
            // auto slot = s.ram[0xb9]; // secondary address
            // slot=0 --> first free slot
            c1541.disk_carousel.insert(0,
                    new C1541::D64_disk(Files::D64{file.data}), file.name);
            return true;
        case Type::g64:
            c1541.disk_carousel.insert(0,
                    new C1541::G64_disk(std::move(file.data)), file.name);
            return true;
        case Type::c64_bin:
            inject(file.data);
            return true;
        case Type::sys_snap: {
            deferred = [&, d = std::move(file.data)]() {
                Files::System_snapshot& iss = *((Files::System_snapshot*)d.data()); // brutal...
                sys_snap.sys_state = iss.sys_state;
                sid.core.write_state(sys_snap.sid);
                cpu.mcp = &NMOS6502::MC::code[0] + iss.cpu_mcp;
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
