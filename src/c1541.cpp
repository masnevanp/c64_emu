#include "c1541.h"


/*
TODO:
    - setting to disable idle trap
        - remove dos patch & clear cpu.sig_halt handler
          (do inside the cpu trap handler to avoid any races...?)
    - 'power off' feature? (just keep in idle mode, until powerd on (and do a reset at that point))
*/


void C1541::Disk_format::GCR_output::enc(const u8 byte) {
    const u8 gcr_nyb_1 = gcr_enc[byte >> 4];
    const u8 gcr_nyb_2 = gcr_enc[byte & 0b1111];

    switch (phase++) {
        case 0:
            *out = gcr_nyb_1 << 3;
            *out |= (gcr_nyb_2 >> 2);
            ++out;
            *out = gcr_nyb_2 << 6;
            return;
        case 1:
            *out |= (gcr_nyb_1 << 1);
            *out |= (gcr_nyb_2 >> 4);
            ++out;
            *out = gcr_nyb_2 << 4;
            return;
        case 2:
            *out |= (gcr_nyb_1 >> 1);
            ++out;
            *out = (gcr_nyb_1 << 7);
            *out |= (gcr_nyb_2 << 2);
            return;
        case 3:
            phase = 0;
            *out |= (gcr_nyb_1 >> 3);
            ++out;
            *out = (gcr_nyb_1 << 5);
            *out |= gcr_nyb_2;
            ++out;
            return;
    }
}

void C1541::Disk_format::GCR_output::raw(const u8 byte) {
    if (phase == 0) *out++ = byte;
    else {
        const auto shift = 2 * phase;
        *out |= (byte >> shift);
        ++out;
        *out = (byte << (8 - shift));
    }
}


void C1541::IEC::reset() {
    irq.reset();

    r_orb = r_ora = r_ddrb = r_ddra = r_acr = r_pcr = 0x00;
    r_t1c = r_t1l = 0xffff;

    t1_irq = VIA::IRQ::Src::none;
    via_pb_in = 0b11111111;
}


void C1541::IEC::update_iec_lines() {
    /*
        - Commodore 64 Programmers Reference Guide, Chapter 8 - Schematics
        - Commodore 1541 Troubleshooting and Repair Guide, Fig. 7-30 (p. 170..171)
    */
    const pin_state clk = invert(cia2_pa(4)) & cia2_pa(6) & invert(via_pb(3));

    const pin_state atn_now = cia2_pa(3) & via_pb(7); // pa3 inverted twice --> taken as such
    if (atn != atn_now) {
        atn = atn_now;
        ca1_edge(atn);
    }

    const pin_state ud3a_out = via_pb(4) ^ atn;
    const pin_state data = invert(cia2_pa(5)) & cia2_pa(7) & invert(via_pb(1)) & invert(ud3a_out);

    const pin_state cia2_pa7_pa6 = (data << 7) | (clk << 6);
    cia2_pa_in(0b11000000, cia2_pa7_pa6);

    const pin_state via_pb720_in = (atn << 7) | (invert(clk) << 2) | (invert(data) << 0);
    const pin_state via_pb65_in = (dev_num - 8) << 5;
    const pin_state pb_in = via_pb720_in | via_pb65_in | 0b00011010;
    via_pb_in = via_pb_out & pb_in;
}


void C1541::Disk_ctrl::reset() {
    irq.reset();

    r_orb = r_ora = r_ddrb = r_ddra = r_acr = r_pcr = 0x00;
    r_t1c = r_t1l = 0xffff;

    t1_irq = VIA::IRQ::Src::none;

    via_pb_in = via_pa_in = 0b11111111;
    via_pb_out = 0xff;

    head.mode = Head_status::Mode::uninit;
    change_track((dir_track - 1) * 2);
}


void C1541::Disk_ctrl::output_pb() {
    const u8 via_pb_out_now = (r_orb & r_ddrb) | ~r_ddrb;

    head.mode = Head_status::Mode((via_pb_out_now & PB::motor) | (head.mode & ~PB::motor));

    if (via_pb_out_now & PB::motor) step_head(via_pb_out_now);

    via_pb_out = via_pb_out_now;
}


void C1541::Disk_ctrl::load_disk(const Disk_image* disk) {
    for (int tn = 0; tn < track_count; ++tn) {
        if (disk->track(tn).len > max_track_len) Log::error("Disk image track too long: %d", disk->track(tn).len);

        const auto& src_track = disk->track(tn).len <= max_track_len
            ? disk->track(tn)
            : Disk_image::null_track();

        // TODO: fill the remaining track data with gap byte?
        std::copy(&src_track.data[0], &src_track.data[src_track.len], &track_data[tn][0]);

        track_len[tn] = src_track.len;
    }

    head.rotation = head.rotation % track_len[head.track_num]; // sort of random...
}


void C1541::Disk_ctrl::step_head(const u8 via_pb_out_now) {
    auto should_step = [&](const int step) -> bool {
        return (via_pb_out_now & PB::head_step) == ((via_pb_out + step) & 0b11);
    };

    if (should_step(+1)) {
        if (head.track_num < Disk_ctrl::last_track) change_track(head.track_num + 1);
    } else if (should_step(-1)) {
        if (head.track_num > Disk_ctrl::first_track) change_track(head.track_num - 1);
    }
}


void C1541::Disk_carousel::insert(int in_slot, const Disk_image* disk, const std::string& name) {
    if (in_slot == 0) {
        in_slot = find_free_slot();
        if (in_slot == 0) {
            Log::error("Carousel full (TODO)");
            return;
        }
    } else if (slots[in_slot].disk) {
        delete slots[in_slot].disk; // TODO: save changes...
    }
    slots[in_slot] = Slot{disk, name, true};
    select(in_slot);
}


void C1541::Disk_carousel::rotate() {
    for (int r = 0; r <= slot_count; ++r) {
        selected_slot = (selected_slot + 1) % slot_count;
        if (selected().disk) break;
    }
    load();
}


void C1541::Disk_carousel::load() {
    disk_ctrl.load_disk(selected().disk);
    disk_ctrl.set_write_prot(selected().write_prot);

    Log::info("Disk selected: %s (slot %d)", selected().disk_name.c_str(), selected_slot);

    // works...? or breaks some custom loader(s)?
    // (would need to fiddle with the PB WP bit then...)
    dos_wp_change_flag = 0x01;
}


void C1541::System::reset() {
    cpu.reset();
    iec.reset();
    dc.reset();
    disk_carousel.reset();
    irq_state = 0x00;
    //idle = false;
}


void C1541::System::tick() {
    address_space_op(cpu.mar(), cpu.mdr(), cpu.mrw());
    cpu.tick();
    iec.tick();
    dc.tick();
    check_irq();
}

/*
void C1541::System::install_idle_trap() {

    u8* rom_w = const_cast<u8*>(rom);

    // patch the DOS idle loop
    u8 checksum_fix = Trap_OPC::idle_trap - rom_w[0x2c9b];
    rom_w[0x2c9b] = Trap_OPC::idle_trap;
    rom_w[0x2c9d] -= checksum_fix;

    cpu.sig_halt = cpu_trap;
}
*/

/*void C1541::System::_dump(const CPU& cpu, u8* mem) {
    using namespace Dbg;
    using namespace NMOS6502;

    std::cout << "\npc: " << print_u16(cpu.pc);
    std::cout << "   sp: " << print_u16(cpu.spf);
    std::cout << " [";
    for (int sp = cpu.spf, i = 1; i < 9 && (sp + i) <= 0x1ff; ++i) {
        std::cout << " " << print_u8(mem[sp + i]);
    }
    std::cout << " ]";
    std::cout << "\n a: " << print_u8(cpu.a);
    std::cout << "   x: " << print_u8(cpu.x);
    std::cout << "   y: " << print_u8(cpu.y);
    std::cout << "   p: " << print_u8(cpu.p);
    std::cout << " [" << flag_str(cpu.p) << "]";
    //// zpaf, a1, a2, a3, a4
    std::cout << "\nzp: " << print_u16(cpu.zpaf);
    std::cout << " a1: " << print_u16(cpu.a1);
    std::cout << " a2: " << print_u16(cpu.a2);
    std::cout << " a3: " << print_u16(cpu.a3);
    std::cout << " a4: " << print_u16(cpu.a4);
    std::cout << " d: " << print_u8(cpu.d);
    std::cout << " ir: " << print_u8(cpu.ir);
    std::cout << "\n==> mar: " << print_u16(cpu.mar()) << " (" << R16_str[cpu.mop().ar] << ")";
    std::cout << "   mdr: " << (cpu.mrw() == MC::RW::w ? print_u8(cpu.mdr()) : "??");
    std::cout << " (" << R8_str[cpu.mop().dr] << ")";
    std::cout << "   r/w: " << MC::RW_str[cpu.mrw()];
    

    std::cout << "\n\nVIA-1:";
    for (u8 r = 0x0; r <= 0x7; ++r) std::cout << ' ' << print_u8(via1._r(r));
    std::cout << "\n      ";
    for (u8 r = 0x8; r <= 0xf; ++r) std::cout << ' ' << print_u8(via1._r(r));
    std::cout << std::endl;
    std::cout << "\nVIA-2:";
    for (u8 r = 0x0; r <= 0x7; ++r) std::cout << ' ' << print_u8(via2._r(r));
    std::cout << "\n      ";
    for (u8 r = 0x8; r <= 0xf; ++r) std::cout << ' ' << print_u8(via2._r(r));
    std::cout << std::endl;
}*/