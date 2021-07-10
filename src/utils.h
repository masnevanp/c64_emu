#ifndef UTILS_H_INCLUDED
#define UTILS_H_INCLUDED

#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <utility>
#include <optional>
#include <iostream>
#ifdef __MINGW32__
#include <windows.h>
#endif
#include "common.h"

#ifdef _WIN32
#include "profileapi.h"
#endif


#define UNUSED(x) (void)(x)


class Timer {
public:
    using clock = std::chrono::high_resolution_clock;
    using ms    = std::chrono::milliseconds;

    Timer() { reset(); }

    void reset() { t1 = clock::now(); }

    int elapsed() const {
        return std::chrono::duration_cast<ms>(clock::now() - t1).count();
    }

    int diff(int target_elapsed_ms, bool reset = false) { // since last reset()
        auto t2 = clock::now();
        auto elapsed_ms = std::chrono::duration_cast<ms>(t2 - t1).count();
        auto diff_ms = target_elapsed_ms - elapsed_ms;
        if (reset) t1 = t2;
        return diff_ms;
    }

    int sync(int target_elapsed_ms, bool reset = false) { // since last reset()
        auto diff_ms = diff(target_elapsed_ms, reset);
        if (diff_ms > 0) {
            #ifdef __MINGW32__
                Sleep(diff_ms);
            #else
                std::this_thread::sleep_for(ms(diff_ms));
            #endif
        }
        return diff_ms;
    }

private:
    clock::time_point t1;

};



class _Stopwatch { public: void start() {} void stop() {} void reset() {} };

#ifdef _WIN32
class Stopwatch {
public:
    Stopwatch() {
        QueryPerformanceFrequency(&li);
        freq = li.QuadPart;
        start();
    }

    void start() { QueryPerformanceCounter(&li); t0 = li.QuadPart; }

    void stop() {
        QueryPerformanceCounter(&li);
        cur_elapsed += (((li.QuadPart - t0) * 1000000) / freq);
        t0 = li.QuadPart;
    }

    void reset() {
        total_elapsed += cur_elapsed;
        ++total_n;
        if (cur_elapsed < recent_min_elapsed && cur_elapsed > 1500) recent_min_elapsed = cur_elapsed;
        if (cur_elapsed < min_elapsed && cur_elapsed > 1500) min_elapsed = cur_elapsed;

        std::cout << cur_elapsed / 100 << ' ';
        if (total_n % 50 == 0) std::cout << "> avg: " << (total_elapsed / total_n)
                                            << ", min: " << min_elapsed
                                            << ", recent min: " << recent_min_elapsed << std::endl;

        if (total_n % 500 == 0) recent_min_elapsed = 999999;

        cur_elapsed = 0;
        start();
    }

private:
    LARGE_INTEGER li;
    decltype(li.QuadPart) t0;
    int freq;
    int cur_elapsed = 0;
    int min_elapsed = 999999;
    int recent_min_elapsed = 999999;
    int total_n = 0;
    int64_t total_elapsed = 0;
};
#endif


// Colodore by pepto - http://www.pepto.de/projects/colorvic/
void get_Colodore(u32* target_palette, double brightness = 50, double contrast = 100, double saturation = 50);

std::optional<std::vector<u8>> read_file(const std::string& filepath);

int read_file(const std::string& filepath, u8* buf);

std::string as_lower(const std::string& src);

std::string as_upper(const std::string& src);

std::string replace(std::string s, const std::string& what, const std::string& with);

/*
template<typename T>
constexpr int count_leading_zero_bits(T x) {
    int n = 0;
    for (; (n < (sizeof(x) * 8)); ++n) {
        if ((x & (1 << n))) break;
    }
    return n;
}


template<typename T>
constexpr T bit_field(T of, T field_mask) {
    return (of & field_mask) >> count_leading_zero_bits(field_mask);
}
*/

#endif // UTILS_H_INCLUDED
