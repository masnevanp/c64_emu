#ifndef IEC_H_INCLUDED
#define IEC_H_INCLUDED

#include <vector>
#include <map>
#include <algorithm>
#include "nmos6502/nmos6502.h"
#include "system.h"


namespace IEC {
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


namespace Virtual {

class Device {
public:
    virtual IO_ST talk(u8 a) = 0;
    virtual IO_ST listen(u8 a) = 0;
    virtual void  sleep() { }

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


class Volatile_disk : public Device {
public:

    virtual IO_ST talk(u8 sa) {
        act_ch = &ch[sa & 0xf];
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

        u8 ch_id = sa & 0xf;
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
    Ch* act_ch;
    buffer cmd_buf;
    std::map<std::string, buffer> storage;
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
    IO_ST select(u8 pa) { dev[act]->sleep(); act = pa & pa_mask; return IO_ST::ok; }

    Device* dev[device_count];
    u8 act = 0x1f;
};


enum IEC_routine : u8 {
    untlk = 0, talk, unlsn, listen, tksa, second, acptr, ciout
};


void patch_kernal_traps(u8* kernal) {
    static const u16 IEC_routine_addr[8] = {
    //  untlk,  talk,   unlsn,  listen, tksa,   second, acptr,  ciout
        0xedef, 0xed09, 0xedfe, 0xed0c, 0xedc7, 0xedb9, 0xee13, 0xeddd
    };
    for (int i = 0; i < 8; ++i) {
        u16 addr = IEC_routine_addr[i] & 0x1fff;
        kernal[addr] = (i << 4) | 0x02; // use halting instructions 0x02, 0x12,... ,0x72
        kernal[addr + 1] = 0x60; // 'rts'
    }
}


bool handle_trap(System::C64& c64, IEC::Virtual::Controller& iec_ctrl) {
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

    --c64.cpu.pc;  // step back to 'rts'

    return true;
}


} // namespace Virtual

} // namespace IEC


#endif // IEC_H_INCLUDED
