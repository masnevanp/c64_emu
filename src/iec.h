#ifndef IEC_H_INCLUDED
#define IEC_H_INCLUDED

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

static const u8 ch_mask = 0xf;


namespace Virtual {

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


enum IEC_routine : u8 {
    untlk = 0, talk, unlsn, listen, tksa, second, acptr, ciout
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


bool on_trap(System::CPU& cpu, u8* ram, IEC::Virtual::Controller& iec_ctrl) {
    enum IEC_command : u8 { listen = 0x20, unlisten = 0x3f, talk = 0x40, untalk = 0x5f };

    // TODO: verify that the pc is what is should be (to dodge faulty traps)?

    IEC::IO_ST status = IEC::IO_ST::ok;
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


} // namespace Virtual

} // namespace IEC


#endif // IEC_H_INCLUDED
