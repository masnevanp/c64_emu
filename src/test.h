#ifndef TEST_H_INCLUDED
#define TEST_H_INCLUDED


namespace Test {

using namespace NMOS6502;

void run_6502_func_test(u16 step_from_pc = 0xffff, u16 output_from_pc = 0xffff) {
    auto mem_ = read_file("data/6502_functional_test/6502_functional_test.bin");

    if (!mem_ || (*mem_).size() != 0x10000) return;

    u8* mem = (u8*)(*mem_).data();

    mem[0xfffc] = 0x00; mem[0xfffd] = 0x04;

    Core::State cpu_state;
    Sig sig_halt = [](){ Log::error("CPU halted!"); };
    Core cpu{cpu_state, sig_halt};
    cpu.reset();

    for (int i = 0; i < 7; ++i) { // do reset...
        if (cpu.mrw() == MC::RW::r) cpu.mdr() = mem[cpu.mar()];
        else mem[cpu.mar()] = cpu.mdr();
        cpu.tick();
    }
    cpu.s.p = 0x00;

    int op_cnt = 0;
    Timer t;
    for (int prev_pc = cpu.s.pc, psc = 0, step = false, output = false; psc < 15; ++psc) {
        // BEWARE
        if (cpu.mcp->mopc >= MC::MOPC::dispatch_cli && cpu.mcp->mopc <= MC::MOPC::dispatch_brk) {
            if (cpu.s.pc == step_from_pc) step = true;
            if (cpu.s.pc == output_from_pc) output = true;
            if (step || output) {
                std::cout << "=========================================================";
                Dbg::print_status(cpu, mem);
                if (step) getchar();
            }

            if (prev_pc != cpu.s.pc) {
                prev_pc = cpu.s.pc;
                psc = 0; // 'pc stuck' counter
            }
            //getchar();
            ++op_cnt;
        }

        if (cpu.mrw() == MC::RW::r) cpu.mdr() = mem[cpu.mar()];
        else mem[cpu.mar()] = cpu.mdr();
        cpu.tick();
    }
    int te = t.elapsed();

    std::cout << "stopped on: " << Dbg::print_u16(cpu.s.pc) << "\n";
    Dbg::print_status(cpu, mem);
    std::cout << "\nop cnt: " << std::dec << int(op_cnt) << ", ";
    std::cout << " time: " << te << ", ";
    std::cout << " op/sec: " << (int)(op_cnt / te * 1000) << "\n";
    std::cout << "=========================================================\n";
    std::cout << ((cpu.s.pc == 0x3469) ? "### PASS ###" : "### FAIL ###");
    std::cout << "\n";
}


void run_test_suite()
{
    u8 mem[0x10000];
    Dbg::System sys{mem};
    sys.do_reset();

    auto init = [&]() {
        static u8 irq[] = {
            0x48, 0x8A, 0x48, 0x98, 0x48, 0xBA, 0xBD, 0x04,
            0x01, 0x29, 0x10, 0xF0, 0x03, 0x6C, 0x16, 0x03,
            0x6C, 0x14, 0x03, 0x00,
        };
        mem[0x0002]  = 0x00; mem[0xa002]  = 0x00; mem[0xa003]  = 0x80;
        mem[0xfffe]  = 0x48; mem[0xffff]  = 0xff; mem[0x01fe]  = 0xff; mem[0x01ff]  = 0x7f;

        for (unsigned int i = 0, m = 0xff48; irq[i] != 0; ++i)
            mem[m + i] = irq[i];

        sys.cpu.s.sp = 0xfd;
        sys.cpu.s.p = 0x04;
        sys.cpu.s.pc = 0x0801;
    };

    auto load = [&](const std::string& filename) {
        const std::string fn = "data/testsuite-2.15/bin/" + as_lower(filename);
        auto file = read_file(fn);
        auto sz = file ? (int)(*file).size() : -1;
        if (sz < 2) exit(1);
        u8* bin = (*file).data();
        auto addr = bin[1] * 0x100 + bin[0];
        for (int b = 2; b < sz; ++b)
            mem[addr++] = bin[b];
        init();

        if (filename == "ANEB") {
            // without this, the 'ANEB' test might get stuck in the 'waitborder' loop
            // (since there is no VIC chip at 0xd011...)
            sys.mem[0xd011] = 0xff;
        }
    };

    auto to_ascii_chr = [&](u8 petscii_chr) -> char { // partial 'support'...
        if (petscii_chr == 13) return '\n';
        if (petscii_chr == 145) { std::cout << "\x1b[A"; return '\r'; }
        if (petscii_chr == 147) return '\n';
        if (petscii_chr == 194) return '|';
        if (petscii_chr == 195) return '-';
        if (petscii_chr == 196) return '-';
        if (petscii_chr == 197) return '-';
        return (char)petscii_chr;
    };

    auto rts = [&]() {
        ++sys.cpu.s.sp; sys.cpu.s.pcl = mem[sys.cpu.s.sp16];
        ++sys.cpu.s.sp; sys.cpu.s.pch = mem[sys.cpu.s.sp16];
        ++sys.cpu.s.pc;
    };

    load("_start");

    Timer t;
    for (;;) {
        //if (sys.cpu.halted()) { std::cout << "\n[HALTED]"; goto exit; }
        if (sys.tn == 0) {
            //print_status(sys.cpu, sys.mem);
            switch (sys.cpu.s.pc) {
                case 0xffd2: // print chr
                    mem[0x030c] = 0;
                    std::cout << to_ascii_chr(sys.cpu.s.a);
                    rts();
                    break;
                case 0xe16f: { // load
                    auto filename_addr = mem[0xbc] * 0x100 + mem[0xbb];
                    std::string filename;
                    for (int c = 0; c < mem[0xb7]; ++c)
                        filename += to_ascii_chr(mem[filename_addr++]);
                    if (filename.find("TRAP"/*"trap17"*/) != std::string::npos) {
                        std::cout << "\n[ALL PASSED]";
                        goto exit;
                    }
                    std::cout << " -> ";
                    load(filename);
                    rts();
                    sys.cpu.s.pc = 0x0816;
                    break;
                }
                case 0xffe4: // scan keyboard
                    sys.cpu.s.a = 0x03; // run/stop
                    rts();
                    break;
                case 0x8000: case 0xa474: // exit
                    std::cout << "\n[FAILED]"; goto exit;
            }
        }
        sys.exec_cycle();
    }

exit:
    int te = t.elapsed();
    //std::cout << "=========================================================\n";
    std::cout << "\ncycle cnt: " << std::dec << sys.cn << ", ";
    std::cout << " time(c1): " << te << ", ";
    std::cout << " cycles/sec: " << (sys.cn / te * 1000) << "\n";
}


} // namespace Test


#endif // TEST_H_INCLUDED
