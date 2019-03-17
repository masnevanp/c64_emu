
#include "utils.h"

#include <fstream>
#include <algorithm>
#include <cmath>



// Colodore by pepto - http://www.pepto.de/projects/colorvic/
void get_Colodore(u32* target_palette, double brightness, double contrast, double saturation) {
    static const u8 levels[] = {  0, 32, 10, 20, 12, 16,  8, 24, 12,  8, 16, 10, 15, 24, 15, 20 };
    static const u8 angles[] = {  0,  0,  4, 12,  2, 10, 15,  7,  5,  6,  4,  0,  0, 10, 15,  0 };

    static const double screen = 1.0 / 5;

    struct Components { double u = 0; double v = 0; double y = 0; };

    auto compose = [&](u8 index) -> Components {
        static const double sector = 360.0 / 16;
        static const double origin = sector / 2;
        static const double radian = 0.017453292519943295;

        Components c; // monochrome (chroma switched off)

        if (angles[index]) {
            double angle = (origin + angles[index] * sector) * radian;
            c.u = (contrast + screen) * (std::cos(angle) * saturation);
            c.v = (contrast + screen) * (std::sin(angle) * saturation);
        }
        c.y = (contrast + screen) * (8 * levels[index] + brightness);

        return c;
    };

    auto convert = [&](const Components& c) -> u32 {
        auto gamma_corr = [](double col) -> u8 {
            static const double src = 2.8; // PAL
            static const double tgt = 2.2; // sRGB

            col = std::max(std::min(col, 255.0), 0.0);
            col = std::pow(255.0, 1.0 - src) * std::pow(col, src);
            col = std::pow(255.0, 1.0 - (1.0 / tgt)) * std::pow(col, 1.0 / tgt);

            return std::round(col);
        };

        // matrix transformation
        double r = c.y + 1.140 * c.v;
        double g = c.y - 0.396 * c.u - 0.581 * c.v;
        double b = c.y + 2.029 * c.u;

        return (gamma_corr(r) << 16) | (gamma_corr(g) << 8) | gamma_corr(b);
    };

    // normalize
    brightness -= 50;
    contrast /= 100;
    saturation *= (1.0 - screen);

    for (int c = 0; c < 16; ++c) {
        target_palette[c] = convert(compose(c));
    }
}


std::vector<char> read_bin_file(const std::string& filename) {
    std::ifstream f(filename, std::ios::binary);
    return std::vector<char>(
        std::istreambuf_iterator<char>(f),
        std::istreambuf_iterator<char>()
    );
}


size_t read_bin_file(const std::string& filename, char* buf) {
    auto bin = read_bin_file(filename);
    std::copy(bin.begin(), bin.end(), buf);
    return bin.size();
}


std::string as_lower(const std::string& src) {
    std::string dst(src);
    std::transform(src.begin(), src.end(), dst.begin(), ::tolower);
    return dst;
}
