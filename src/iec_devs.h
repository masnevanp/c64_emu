#ifndef IEC_DEVS_H_INCLUDED
#define IEC_DEVS_H_INCLUDED

#include <string>
#include <vector>
#include <map>

#include "iec.h"
#include "utils.h"


// TODO: namespace?

class Dummy_device : public IEC::Virtual::Device {
public:
    virtual IEC::IO_ST talk(u8 a)         { msg("t", a); x = a; c = 0; return IEC::IO_ST::ok; }
    virtual IEC::IO_ST listen(u8 a)       { msg("l", a); x = a; c = 0; return IEC::IO_ST::ok; }
    virtual void sleep()                  { msg("s", c); }
    virtual IEC::IO_ST read(u8& d)        { msg("r", x); d = x; ++c; return IEC::IO_ST::eof;}
    virtual IEC::IO_ST write(const u8& d) { msg("w", d); x = d; ++c; return IEC::IO_ST::ok; }
private:
    u8 x = 0x00;
    int c = 0;
    void msg(const char* m, int d) { std::cout << " DD:" << m << ',' << d; }
};


class Host_drive : public IEC::Virtual::Device  {
public:
    // TODO:
    //   - host dir navigation using entries in dir listing (see 'insta_load')
    //     (current dir is drive-global..?)
    Host_drive(const std::string& host_dir_) : host_dir(host_dir_) {}

    virtual IEC::IO_ST talk(u8 sa) {
        cur_ch = &ch[sa & IEC::ch_mask];
        return IEC::IO_ST::ok;
    }

    virtual IEC::IO_ST listen(u8 sa) {
        enum command : u8 { select = 0x6, close = 0xe, open = 0xf, };

        cur_ch = &ch[sa & IEC::ch_mask];

        command cmd = (command)(sa >> 4);
        if (cmd == command::close) cur_ch->close();
        else if (cmd == command::open) cur_ch->start_opening(); // TODO: write/append modes

        return IEC::IO_ST::ok;
    }

    virtual void sleep() { cur_ch->check_opening(host_dir /*bruh...*/); }

    virtual IEC::IO_ST read(u8& d)        { return cur_ch->read(d); }
    virtual IEC::IO_ST write(const u8& d) { return cur_ch->write(d); }

private:
    using buffer = std::vector<char>;

    struct Ch {
        enum Mode : u8 { r, w, a };
        enum Status : u8 { opening, open, closed };

        void start_opening(Mode m = Mode::r) {
            status = Status::opening;
            mode = m;
            name.clear();
            // now waiting to receive the filename before opening...
        }

        void check_opening(const std::string& host_dir) {
            if (status == Status::opening && name.size() > 0) {
                if (mode == Mode::r) {
                    data = read_bin_file(host_dir + std::string(name.begin(), name.end()));
                    if (data.size() < 2) return close(); // TODO: 'sz < 2' ok for non-prg files...
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

        IEC::IO_ST read(u8& d) {
            if (mode == Mode::r && status == Status::open) {
                if (pos < (i32)data.size()) d = data[pos++];
                if (pos == (i32)data.size()) return IEC::IO_ST::eof;
                return IEC::IO_ST::ok;
            } else {
                return IEC::IO_ST::timeout_r;
            }
        }

        IEC::IO_ST write(const u8& d) {
            if (status == Status::opening) {
                name.push_back(d);
                return IEC::IO_ST::ok;
            } // else check mode, etc... TODO
            return IEC::IO_ST::timeout_w;
        }

        Mode mode;
        Status status = Status::closed;
        buffer name; // TODO: has to stay around?
        buffer data;
        i32 pos;
    };

    Ch ch[16];
    Ch* cur_ch = &ch[0];

    const std::string host_dir; // TODO
};


class Volatile_disk : public IEC::Virtual::Device {
public:

    virtual IEC::IO_ST talk(u8 sa) {
        act_ch = &ch[sa & IEC::ch_mask];
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

        return IEC::IO_ST::ok;
    }

    virtual IEC::IO_ST listen(u8 sa) {
        enum command : u8 { select = 0x6, close = 0xe, open = 0xf, };

        u8 ch_id = sa & IEC::ch_mask;
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

        return IEC::IO_ST::ok;
    }

    virtual IEC::IO_ST read(u8& d) {
        if (act_ch->buf && act_ch->read_pos > -1) {
            if (act_ch->read_pos < (int)act_ch->buf->size())
                d = (*act_ch->buf)[act_ch->read_pos++];
            if (act_ch->read_pos == (int)act_ch->buf->size()) return IEC::IO_ST::eof;
            else return IEC::IO_ST::ok;
        } else {
            return IEC::IO_ST::timeout_r;
        }
    }

    virtual IEC::IO_ST write(const u8& d) {
        if (act_ch->buf) {
            act_ch->buf->push_back(d);
            return IEC::IO_ST::ok;
        } else {
            return IEC::IO_ST::timeout_w;
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


#endif // IEC_DEVS_H_INCLUDED
