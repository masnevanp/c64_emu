
#include "vic_ii.h"


const u8 VIC_II::Banker::PLA[4][4] = { // [mode]
    // TODO: full mode handling (ultimax + special modes)
    { ram_3, ram_3, ram_3, ram_3 },
    { ram_2, chr_r, ram_2, ram_2 },
    { ram_1, ram_1, ram_1, ram_1 },
    { ram_0, chr_r, ram_0, ram_0 },
};
