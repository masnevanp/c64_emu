
#include "nmos6502.h"


const std::string NMOS6502::Ri16_str[] = {
    "zpaf", "pc", "d_ir", "a1", "a2", "spf", "p_a", "y_x", "a3", "a4" // TODO: big-endian versions
};

const std::string NMOS6502::Ri8_str[] = {
    "zpa", "pcl", "pch", "d", "ir", "a1l", "a1h", "a2l", "a2h",
    "sp", "sph", "p", "a", "x", "y",
};
