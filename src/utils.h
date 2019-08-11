#ifndef UTILS_H_INCLUDED
#define UTILS_H_INCLUDED

#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <iostream>
#ifdef __MINGW32__
#include <windows.h>
#endif
#include "common.h"

#define UNUSED(x) (void)(x)


class Clock {
public:
    using clock = std::chrono::high_resolution_clock;
    using ms    = std::chrono::milliseconds;

    Clock() { reset(); }

    void reset() { t1 = clock::now(); }

    int elapsed() const {
        return std::chrono::duration_cast<ms>(clock::now() - t1).count();
    }

    int wait_until(int target_elapsed_ms, bool reset = false) { // since last reset()
        auto t2 = clock::now();
        auto elapsed_ms = std::chrono::duration_cast<ms>(t2 - t1).count();
        auto wait_period_ms = target_elapsed_ms - elapsed_ms;
        if (wait_period_ms > 0) {
            #ifdef __MINGW32__
                Sleep(wait_period_ms);
            #else
                std::this_thread::sleep_for(ms(wait_period_ms));
            #endif
        }
        if (reset) {
            t1 = t2;
        }
        return wait_period_ms;
    }

private:
    clock::time_point t1;

};


// Colodore by pepto - http://www.pepto.de/projects/colorvic/
void get_Colodore(u32* target_palette, double brightness = 50, double contrast = 100, double saturation = 50);

std::vector<char> read_bin_file(const std::string& filename);

size_t read_bin_file(const std::string& filename, char* buf);

std::string as_lower(const std::string& src);

#endif // UTILS_H_INCLUDED
