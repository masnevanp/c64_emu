#include <iostream>
#include <string>
#include <vector>
#include <map>

#include "nmos6502/nmos6502.h"
#include "nmos6502/nmos6502_core.h"
#include "utils.h"
#include "dbg.h"
#include "system.h"
#include "iec.h"
#include "test.h"


enum Trap : u8 {
    // OPC of the instruction to be used
    insta_load = 0xf2,
    iec_base = 0x02, // high nybble will encode the IEC routine id
};

enum IEC_routine : u8 {
    untlk = 0, talk, unlsn, listen, tksa, second, acptr, ciout
};


// 'halt' instructions used for trapping
void patch_kernal_traps(u8* kernal) {
    // instant load - trap loading from the tape device (device 1)
    kernal[0x1539] = Trap::insta_load;
    kernal[0x153a] = 0xea; // nop
    kernal[0x153b] = 0x60; // rts

    // trap IEC routines
    static const u16 IEC_routine_addr[8] = {
    //  untlk,  talk,   unlsn,  listen, tksa,   second, acptr,  ciout
        0xedef, 0xed09, 0xedfe, 0xed0c, 0xedc7, 0xedb9, 0xee13, 0xeddd
    };
    for (int i = 0; i < 8; ++i) {
        u16 addr = IEC_routine_addr[i] & 0x0fff;
        kernal[addr] = (i << 4) | Trap::iec_base;
        kernal[addr + 1] = 0x60; // 'rts'
    }
}


void do_insta_load(System::C64& c64) {
    // IDEAs:
    //   - 'LOAD' or 'LOAD""' loads the directory listing (auto-list somehow?)
    //   - listing has entries for sub/parent directories (parent = '..')
    //        LOAD".." --> cd up
    //        LOAD"some-sub-dir" --> cd some-sub-dir
    //   - or 'LOAD' loads & autostart a loader program (native, but can interact with host)
    static const std::string dir = "data/prg/"; // TODO...
    u16 filename_addr = c64.ram[0xbc] * 0x100 + c64.ram[0xbb];
    u8 filename_len = c64.ram[0xb7];
    std::string filename(&c64.ram[filename_addr], &c64.ram[filename_addr + filename_len]);
    filename = as_lower(dir + filename);

    auto bin = read_bin_file(filename);
    auto sz = bin.size();
    if (sz > 2) {
        std::cout << "\nLoaded '" << filename << "', " << sz << " bytes ";

        // load addr (used if 2nd.addr == 0)
        c64.ram[0xc3] = c64.cpu.x;
        c64.ram[0xc4] = c64.cpu.y;

        u8 scnd_addr = c64.ram[0xb9];

        u16 addr = (scnd_addr == 0)
            ? c64.cpu.y * 0x100 + c64.cpu.x
            : bin[1] * 0x100 + bin[0];

        for (u16 b = 2; b < sz; ++b)
            c64.ram[addr++] = bin[b];

        // end pointer
        c64.ram[0xae] = c64.cpu.x = addr;
        c64.ram[0xaf] = c64.cpu.y = addr >> 8;

        // status
        c64.cpu.clr(NMOS6502::Flag::C); // no error
        c64.ram[0x90] = 0x00; // io status ok
    } else {
        std::cout << "\nFailed to load '" << filename << "'";
        c64.cpu.pc = 0xf704; // jmp to 'file not found'
    }

    ++c64.cpu.mcp; // halted --> must bump forward

    return;
}


bool do_iec_routine(System::C64& c64, IEC::Virtual::Controller& iec_ctrl) {
    enum IEC_command : u8 { listen = 0x20, unlisten = 0x3f, talk = 0x40, untalk = 0x5f };

    // TODO: verify that the pc is what is should be (to dodge faulty traps)?

    IEC::IO_ST status = IEC::IO_ST::ok;
    u8 iec_routine = c64.cpu.ir >> 4;
    switch (iec_routine) {
        case IEC_routine::untlk:
            c64.cpu.a = IEC_command::untalk;
            iec_ctrl.talk(c64.cpu.a);
            break;
        case IEC_routine::talk:
            c64.cpu.a |= IEC_command::talk;
            iec_ctrl.talk(c64.cpu.a);
            break;
        case IEC_routine::unlsn:
            c64.cpu.a = IEC_command::unlisten;
            iec_ctrl.listen(c64.cpu.a);
            break;
        case IEC_routine::listen:
            c64.cpu.a |= IEC_command::listen;
            iec_ctrl.listen(c64.cpu.a);
            break;
        case IEC_routine::tksa:
            status = iec_ctrl.talk_s(c64.cpu.a);
            c64.cpu.clr(NMOS6502::Flag::N);
            break;
        case IEC_routine::second:
            status = iec_ctrl.listen_s(c64.cpu.a);
            break;
        case IEC_routine::acptr:
            status = iec_ctrl.read(c64.cpu.a);
            break;
        case IEC_routine::ciout:
            status = iec_ctrl.write(c64.cpu.a);
            break;
        default:
            return false;
    }

    c64.cpu.clr(NMOS6502::Flag::C);
    c64.ram[0x90] |= status; // TODO: should it be cleared in some cases??

    ++c64.cpu.mcp; // halted --> bump forward
    --c64.cpu.pc;  // step back to 'rts'

    return true;
}


class Dummy_device : public IEC::Virtual::Device {
public:
    Dummy_device(u8 id_) : id(id_) {}

    virtual IEC::IO_ST talk(u8 a)         { msg("t", a); x = a; c = 0; return IEC::IO_ST::ok; }
    virtual IEC::IO_ST listen(u8 a)       { msg("l", a); x = a; c = 0; return IEC::IO_ST::ok; }
    virtual void sleep()                  { msg("s", c); }
    virtual IEC::IO_ST read(u8& d)        { msg("r", x); d = x; ++c; return IEC::IO_ST::eof;}
    virtual IEC::IO_ST write(const u8& d) { msg("w", d); x = d; ++c; return IEC::IO_ST::ok; }

    u8 id;
private:
    u8 x = 0x00;
    int c = 0;
    void msg(const char* m, int d) {
        std::cout << " D" << (int)id << ":" << m << ',' << (int)d;
    }
};


//static const std::string fn = "data/prg/giana.prg";
//static auto bin = read_bin_file(fn);
//static auto sz = bin.size();
//static auto pos = sz;
class RAM_disk : public IEC::Virtual::Device {
public:
    using buffer = std::vector<u8>;

    enum command : u8 { select = 0x6, close = 0xe, open = 0xf, };

    struct Ch {
        buffer* buf = nullptr;
        int read_pos = -1; // -1 = not for reading
    };

    RAM_disk() {
        ch[16].buf = &cmd_buf;
    }

    virtual IEC::IO_ST talk(u8 sa) {
        if (open_ch_req != 0xff) {
            std::string name(cmd_buf.begin(), cmd_buf.end());
            auto file = storage.find(name);
            if (file != storage.end()) {
                ch[open_ch_req].buf = &(file->second);
                ch[open_ch_req].read_pos = 0;
            }
            open_ch_req = 0xff; // 'ack'
        }

        act_ch = &ch[sa & 0xf];

        return IEC::IO_ST::ok;
    }

    virtual IEC::IO_ST listen(u8 sa) {
        u8 ch_id = sa & 0xf;
        command cmd = (command)(sa >> 4);
        switch (cmd) {
            case command::select:
                if (open_ch_req != 0xff) {
                    std::string name(cmd_buf.begin(), cmd_buf.end());
                    ch[open_ch_req].buf = &storage[name];
                    ch[open_ch_req].read_pos = -1;
                    open_ch_req = 0xff;
                }
                act_ch = &ch[ch_id];
                break;
            case command::close:
                ch[ch_id].buf = nullptr;
                break;
            case command::open:
                open_ch_req = ch_id;
                cmd_buf.clear();
                act_ch = &ch[16];
                break;
        }

        return IEC::IO_ST::ok;
    }

    // virtual void sleep() { }

    virtual IEC::IO_ST read(u8& d) {
        if (act_ch->buf) { // TODO: check 'read_pos >= 0' ?
            if (act_ch->read_pos < (int)act_ch->buf->size())
                d = (*act_ch->buf)[act_ch->read_pos++];
            if (act_ch->read_pos == (int)act_ch->buf->size()) return IEC::IO_ST::eof;
            else return IEC::IO_ST::ok;
        } else {
            return IEC::IO_ST::timeout_r; // ok? eof?
        }
    }

    virtual IEC::IO_ST write(const u8& d) {
        if (act_ch->buf) {
            act_ch->buf->push_back(d);
            return IEC::IO_ST::ok;
        } else {
            return IEC::IO_ST::timeout_w; // ok?
        }
    }

private:
    u8 open_ch_req = 0xff;
    Ch ch[17];
    Ch* act_ch;
    buffer cmd_buf;
    std::map<std::string, buffer> storage;
};


void run_c64() {
    u8 basic[0x2000];
    u8 kernal[0x2000];
    u8 charr[0x1000];

    const std::string rom_dir = "data/c64_roms/";
    read_bin_file(rom_dir + "basic.rom", (char*)basic);
    read_bin_file(rom_dir + "kernal.rom", (char*)kernal);
    read_bin_file(rom_dir + "char.rom", (char*)charr);

    patch_kernal_traps(kernal);

    System::ROM roms{basic, kernal, charr};
    System::C64 c64(roms);

    IEC::Virtual::Controller iec_ctrl;
    Dummy_device dd_a(20);
    Dummy_device dd_b(30);
    RAM_disk ram_disk;
    iec_ctrl.attach(dd_a, dd_a.id);
    iec_ctrl.attach(dd_b, dd_b.id);
    iec_ctrl.attach(ram_disk, 10);

    NMOS6502::Sig on_halt = [&]() {
        // TODO: verify that it is a kernal trap, e.g. 'banker.mapping(cpu.pc) == kernal' ?
        if (c64.cpu.ir == insta_load) do_insta_load(c64);
        else if (!do_iec_routine(c64, iec_ctrl)) {
            std::cout << "\n****** CPU halted! ******\n";
            Dbg::print_status(c64.cpu, c64.ram);
        }
    };

    c64.cpu.sig_halt = on_halt;

    c64.run();
}


void test()
{
    u8 mem[0x10000]= {
        0x58, 0xea, 0x04, 0xea, 0xea, 0xea, 0xea, 0xea, 0xea, 0xea, 0, 0, 0, 0, 0, 0,
        0xe8, 0x40, 0x58, 0x05, 0xdd, 0x00, 0x32, 0x42, 0, 0, 0, 0, 0, 0, 0, 0,
        0xe8, 0x40, 0x58, 0x05, 0xdd, 0x00, 0x32, 0x42, 0, 0, 0, 0, 0, 0, 0, 0,
        0xe8, 0x60, 0x4c, 0x00, 0x00, 0x00, 0x32, 0x42, 0, 0, 0, 0, 0, 0, 0, 0,
    };
    mem[0xfffa] = 0x10; mem[0xfffb] = 0x00;
    mem[0xfffc] = 0x00; mem[0xfffd] = 0x00;
    mem[0xfffe] = 0x20; mem[0xffff] = 0x00;

    Dbg::System sys{mem};
    sys.cpu.set_irq();
    step(sys);
}


int main(int argv, char** args) {
    UNUSED(argv); UNUSED(args);
    //Test::run_6502_func_test();
    //Test::run_test_suite();
    run_c64();
    //test();
    return 0;
}
