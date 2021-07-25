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
    ~Stopwatch() {
        std::cout << "\nTotal avg: " << (total_elapsed / total_n) << "\n";
    }

    void start() { QueryPerformanceCounter(&li); t0 = li.QuadPart; }

    void stop() {
        QueryPerformanceCounter(&li);
        cur_elapsed += (((li.QuadPart - t0) * 1000000) / freq);
        t0 = li.QuadPart;
    }

    void reset() {
        total_elapsed += cur_elapsed;
        hist[total_n % hist_len] = cur_elapsed;
        ++total_n;
        cur_elapsed = 0;

        if ((total_n % 100) == 0) {
            auto avg = [&](int win_sz) {
                int64_t sum = 0;
                for (int i = 1; i <= win_sz; ++i) {
                    sum += hist[((total_n + hist_len) - i) % hist_len];
                }
                return sum / win_sz;
            };
            std::cout << avg(100);
            if ((total_n % 200) == 0) std::cout << ' ' << avg(200);
            if ((total_n % 400) == 0) std::cout << ' ' << avg(400);
            if ((total_n % 800) == 0) std::cout << ' ' << avg(800);
            std::cout << std::endl;
        }

        start();
    }

private:
    static constexpr int hist_len = 800;

    LARGE_INTEGER li;
    decltype(li.QuadPart) t0;
    int freq;

    int cur_elapsed = 0;
    int total_n = 0;
    int64_t total_elapsed = 0;

    int hist[hist_len] = {};
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
