
#include "nmos6502.h"


const std::string NMOS6502::Ri16_str[] = {
    "pc", "spf", "a_p", "y_x", "ir_d", "zpaf", "a1", "a2", "a3", "a4" // TODO: big-endian versions
};

const std::string NMOS6502::Ri8_str[] = {
    "pcl", "pch", "sp", "sph", "p", "a", "x", "y",
    "d", "ir", "zpa", "zpah", "a1l", "a1h", "a2l", "a2h"
};
