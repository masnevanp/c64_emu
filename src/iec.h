#ifndef IEC_H_INCLUDED
#define IEC_H_INCLUDED


#include "nmos6502/nmos6502.h"


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


class Controller : public Device {
public:
    Controller() {
        static Null_device null_dev;
        for (u8 pa = 0; pa < 0x20; ++pa) {
            dev[pa] = &null_dev;
        }
        act = &null_dev;
    }

    virtual IO_ST talk(u8 pa)        { act->sleep(); act = dev[pa & 0x1f]; return IO_ST::ok; }
    virtual IO_ST listen(u8 pa)      { act->sleep(); act = dev[pa & 0x1f]; return IO_ST::ok; }

    virtual IO_ST read(u8& d)        { return act->read(d);  }
    virtual IO_ST write(const u8& d) { return act->write(d); }

    IO_ST talk_s(u8 sa)              { return act->talk(sa);   }
    IO_ST listen_s(u8 sa)            { return act->listen(sa); }

    bool attach(Device& d, u8 pa) {
        if (pa >= 0x1f) return false; // keeping 0x1f as null device
        dev[pa] = &d;
        return true;
    }

private:
    Device* dev[32];
    Device* act;
};


} // namespace Virtual

} // namespace IEC


#endif // IEC_H_INCLUDED
