
#include "nmos6502.h"


const std::string NMOS6502::Ri16_str[] = {
    "zp16", "pc", "d_ir", "a1", "a2", "sp16", "p_a", "y_x", "a3", "a4" // TODO: big-endian versions
};

const std::string NMOS6502::Ri8_str[] = {
    "zp", "zph", "pcl", "pch", "d", "ir", "a1l", "a1h", "a2l", "a2h",
    "sp", "sph", "p", "a", "x", "y",
};
