#ifndef UTILS_H_INCLUDED
#define UTILS_H_INCLUDED

#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <iostream>

class Clock {
public:
    using clock = std::chrono::high_resolution_clock;
    using ms    = std::chrono::milliseconds;

    Clock() { reset(); }

    void reset() { t1 = clock::now(); }

    int elapsed() const {
        auto diff = clock::now() - t1;
        return std::chrono::duration_cast<ms>(diff).count();
    }

    int wait_until(int elapsed_ms) const { // since last reset()
        auto wait_ms = elapsed_ms - elapsed();
        if (wait_ms > 0) std::this_thread::sleep_for(ms(wait_ms));
        return wait_ms;
    }

private:
    clock::time_point t1;

};


std::vector<char> read_bin_file(const std::string& filename);

size_t read_bin_file(const std::string& filename, char* buf);


#endif // UTILS_H_INCLUDED
