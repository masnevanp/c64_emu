#ifndef IEC_VIRTUAL_H_INCLUDED
#define IEC_VIRTUAL_H_INCLUDED

#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include "nmos6502/nmos6502.h"
#include "system.h"
#include "utils.h"
#include "file_utils.h"



namespace IEC_virtual {
/* Very minimal. E.g. not supported:
   - multiple listeners
   - data transfer between devices
   - ..?
*/

enum IO_ST : u8 { // Kernal I/O Status Word
    ok = 0x00,
    timeout_w = 0x01,
    timeout_r = 0x02,
    error = 0x10,
    eof = 0x40,
    not_present = 0x80,
};


enum IEC_routine : u8 {
    untlk = 0, talk, unlsn, listen, tksa, second, acptr, ciout
};


static const u8 ch_mask = 0xf;


class Device {
public:
    virtual IO_ST talk(u8 a) = 0;   // tksa
    virtual IO_ST listen(u8 a) = 0; // second
    virtual void  sleep() { }       // on untalk/unlisten

    virtual IO_ST read(u8& d) = 0;
    virtual IO_ST write(const u8& d) = 0;

    virtual ~Device() {}
};


class Null_device : public Device {
public:
    virtual IO_ST talk(u8 a)         { UNUSED(a); return IO_ST::not_present; }
    virtual IO_ST listen(u8 a)       { UNUSED(a); return IO_ST::not_present; }
    virtual IO_ST read(u8& d)        { UNUSED(d); return IO_ST::timeout_r; }
    virtual IO_ST write(const u8& d) { UNUSED(d); return IO_ST::timeout_w; }
};


class Controller : public Device {
public:
    enum { pa_mask = 0x1f, device_count = 0x20 };

    Controller() {
        static Null_device null_dev;
        std::fill_n(dev, device_count, &null_dev);
    }

    virtual IO_ST talk(u8 pa)        { return select(pa); }
    virtual IO_ST listen(u8 pa)      { return select(pa); }

    virtual IO_ST read(u8& d)        { return dev[act]->read(d);  }
    virtual IO_ST write(const u8& d) { return dev[act]->write(d); }

    IO_ST talk_s(u8 sa)              { return dev[act]->talk(sa);   }
    IO_ST listen_s(u8 sa)            { return dev[act]->listen(sa); }

    bool attach(Device& d, u8 pa) {
        if (pa >= (device_count - 1)) return false; // last is always null device
        dev[pa] = &d;
        return true;
    }

private:
    IO_ST select(u8 pa) {
        dev[act]->sleep();
        act = pa & pa_mask;
        return IO_ST::ok;
    }

    Device* dev[device_count];
    u8 act = 0x1f;
};


class Dummy_device : public Device {
public:
    virtual IO_ST talk(u8 a)         { msg("t", a); x = a; c = 0; return IO_ST::ok; }
    virtual IO_ST listen(u8 a)       { msg("l", a); x = a; c = 0; return IO_ST::ok; }
    virtual void sleep()             { msg("s", c); }
    virtual IO_ST read(u8& d)        { msg("r", x); d = x; ++c; return IO_ST::eof;}
    virtual IO_ST write(const u8& d) { msg("w", d); x = d; ++c; return IO_ST::ok; }
private:
    u8 x = 0x00;
    int c = 0;
    void msg(const char* m, int d) { std::cout << " DD:" << m << ',' << d; }
};


class Host_drive : public Device  {
public:
    Host_drive(loader& load_file_) : load_file(load_file_) {}

    virtual IO_ST talk(u8 sa) {
        cur_ch = &ch[sa & ch_mask];
        return IO_ST::ok;
    }

    virtual IO_ST listen(u8 sa) {
        enum command : u8 { select = 0x6, close = 0xe, open = 0xf, };

        cur_ch = &ch[sa & ch_mask];

        command cmd = (command)(sa >> 4);
        if (cmd == command::close) cur_ch->close();
        else if (cmd == command::open) cur_ch->start_opening(); // TODO: write/append modes

        return IO_ST::ok;
    }

    virtual void sleep() { cur_ch->check_opening(load_file /*bruh...*/); }

    virtual IO_ST read(u8& d)        { return cur_ch->read(d); }
    virtual IO_ST write(const u8& d) { return cur_ch->write(d); }

private:
    using buffer = std::vector<u8>;

    struct Ch {
        enum Mode : u8 { r, w, a };
        enum Status : u8 { opening, open, closed };

        void start_opening(Mode m = Mode::r) {
            status = Status::opening;
            mode = m;
            name.clear();
            // now waiting to receive the filename before opening...
        }

        void check_opening(loader& load_file) {
            if (status == Status::opening && name.size() > 0) {
                if (mode == Mode::r) {
                    std::string name_str(name.begin(), name.end());
                    if (auto bin = load_file(name_str)) data = std::move(*bin);
                    else return close();
                } else {
                    data.clear(); // TODO: write/append, open/create actual file here?
                }
                pos = 0;
                status = Status::open;
            }
        }

        void close() {
            // TODO: write/append, save here?
            data.clear();
            status = Status::closed;
        }

        IO_ST read(u8& d) {
            if (mode == Mode::r && status == Status::open) {
                if (pos < (i32)data.size()) d = data[pos++];
                if (pos == (i32)data.size()) return IO_ST::eof;
                return IO_ST::ok;
            } else {
                return IO_ST::timeout_r;
            }
        }

        IO_ST write(const u8& d) {
            if (status == Status::opening) {
                name.push_back(d);
                return IO_ST::ok;
            } // else check mode, etc... TODO
            return IO_ST::timeout_w;
        }

        Mode mode;
        Status status = Status::closed;
        buffer name; // TODO: has to stay around?
        buffer data;
        i32 pos;
    };

    Ch ch[16];
    Ch* cur_ch = &ch[0];

    loader& load_file;
};


class Volatile_disk : public Device {
public:

    virtual IO_ST talk(u8 sa) {
        act_ch = &ch[sa & ch_mask];
        if (act_ch->buf == &cmd_buf) {
            std::string name(cmd_buf.begin(), cmd_buf.end());
            auto file = storage.find(name);
            if (file != storage.end()) {
                act_ch->buf = &(file->second);
                act_ch->read_pos = 0;
            } else {
                act_ch->buf = nullptr;
            }
        }

        return IO_ST::ok;
    }

    virtual IO_ST listen(u8 sa) {
        enum command : u8 { select = 0x6, close = 0xe, open = 0xf, };

        u8 ch_id = sa & ch_mask;
        command cmd = (command)(sa >> 4);
        switch (cmd) {
            case command::select:
                act_ch = &ch[ch_id];
                if (act_ch->buf == &cmd_buf) {
                    std::string name(cmd_buf.begin(), cmd_buf.end());
                    act_ch->buf = &storage[name];
                    act_ch->read_pos = -1;
                }
                break;
            case command::close:
                ch[ch_id].buf = nullptr;
                break;
            case command::open:
                cmd_buf.clear();
                act_ch = &ch[ch_id];
                act_ch->buf = &cmd_buf;
                break;
        }

        return IO_ST::ok;
    }

    virtual IO_ST read(u8& d) {
        if (act_ch->buf && act_ch->read_pos > -1) {
            if (act_ch->read_pos < (int)act_ch->buf->size())
                d = (*act_ch->buf)[act_ch->read_pos++];
            if (act_ch->read_pos == (int)act_ch->buf->size()) return IO_ST::eof;
            else return IO_ST::ok;
        } else {
            return IO_ST::timeout_r;
        }
    }

    virtual IO_ST write(const u8& d) {
        if (act_ch->buf) {
            act_ch->buf->push_back(d);
            return IO_ST::ok;
        } else {
            return IO_ST::timeout_w;
        }
    }

private:
    using buffer = std::vector<u8>;

    struct Ch {
        buffer* buf = nullptr;
        int read_pos; // -1 = not for reading
    };

    Ch ch[16];
    Ch* act_ch = &ch[0];
    buffer cmd_buf;
    std::map<std::string, buffer> storage;
};


void install_kernal_traps(u8* kernal, u8 trap_opc) {
    static const u16 IEC_routine_addr[8] = {
    //  untlk,  talk,   unlsn,  listen, tksa,   second, acptr,  ciout
        0xedef, 0xed09, 0xedfe, 0xed0c, 0xedc7, 0xedb9, 0xee13, 0xeddd
    };
    for (int i = 0; i < 8; ++i) {
        u16 addr = IEC_routine_addr[i] & 0x1fff;
        kernal[addr] = trap_opc; // using a halting instruction
        kernal[addr + 1] = i; // IEC routine id
    }
}


bool on_trap(System::CPU& cpu, u8* ram, Controller& iec_ctrl) {
    enum IEC_command : u8 { listen = 0x20, unlisten = 0x3f, talk = 0x40, untalk = 0x5f };

    // TODO: verify that the pc is what is should be (to dodge faulty traps)?

    IO_ST status = IO_ST::ok;
    u8 iec_routine = cpu.d;
    switch (iec_routine) {
        case IEC_routine::untlk:
            cpu.a = IEC_command::untalk;
            iec_ctrl.talk(cpu.a);
            break;
        case IEC_routine::talk:
            cpu.a |= IEC_command::talk;
            iec_ctrl.talk(cpu.a);
            break;
        case IEC_routine::unlsn:
            cpu.a = IEC_command::unlisten;
            iec_ctrl.listen(cpu.a);
            break;
        case IEC_routine::listen:
            cpu.a |= IEC_command::listen;
            iec_ctrl.listen(cpu.a);
            break;
        case IEC_routine::tksa:
            status = iec_ctrl.talk_s(cpu.a);
            cpu.clr(NMOS6502::Flag::N);
            break;
        case IEC_routine::second:
            status = iec_ctrl.listen_s(cpu.a);
            break;
        case IEC_routine::acptr:
            status = iec_ctrl.read(cpu.a);
            break;
        case IEC_routine::ciout:
            status = iec_ctrl.write(cpu.a);
            break;
        default:
            std::cout << "\nUnknown ICE routine: " << (int)iec_routine;
            return false;
    }

    cpu.clr(NMOS6502::Flag::C);
    ram[0x90] |= status; // TODO: should it be cleared in some cases??

    cpu.pc = 0xedee;  // jump to a 'rts'

    return true;
}


} // namespace IEC_virtual


#endif // IEC_VIRTUAL_H_INCLUDED
