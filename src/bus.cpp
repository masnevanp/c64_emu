#include "bus.h"


/*
     m  x   g   c   h   l  00-0f 10-7f 80-9f a0-bf c0-cf d0-df e0-ff  pla_idx
    =========================================================================
    31  1   1   1   1   1   RAM   RAM   RAM   BAS   RAM   I/O   KER   : 13

    30  1   1   1   1   0   RAM   RAM   RAM   RAM   RAM   I/O   KER   : 12
    14  0   1   1   1   0   RAM   RAM   RAM   RAM   RAM   I/O   KER

    29  1   1   1   0   1   RAM   RAM   RAM   RAM   RAM   I/O   RAM   : 11
    13  0   1   1   0   1   RAM   RAM   RAM   RAM   RAM   I/O   RAM
    5   0   0   1   0   1   RAM   RAM   RAM   RAM   RAM   I/O   RAM

    27  1   1   0   1   1   RAM   RAM   RAM   BAS   RAM   CHRR  KER   : 10

    26  1   1   0   1   0   RAM   RAM   RAM   RAM   RAM   CHRR  KER   : 09
    10  0   1   0   1   0   RAM   RAM   RAM   RAM   RAM   CHRR  KER

    25  1   1   0   0   1   RAM   RAM   RAM   RAM   RAM   CHRR  RAM   : 08
    9   0   1   0   0   1   RAM   RAM   RAM   RAM   RAM   CHRR  RAM

    23  1   0   1   1   1   RAM   ---   CRL   ---   ---   I/O   CRH   : 07
    22  1   0   1   1   0   RAM   ---   CRL   ---   ---   I/O   CRH
    21  1   0   1   0   1   RAM   ---   CRL   ---   ---   I/O   CRH
    20  1   0   1   0   0   RAM   ---   CRL   ---   ---   I/O   CRH
    19  1   0   0   1   1   RAM   ---   CRL   ---   ---   I/O   CRH
    18  1   0   0   1   0   RAM   ---   CRL   ---   ---   I/O   CRH
    17  1   0   0   0   1   RAM   ---   CRL   ---   ---   I/O   CRH
    16  1   0   0   0   0   RAM   ---   CRL   ---   ---   I/O   CRH

    15  0   1   1   1   1   RAM   RAM   CRL   BAS   RAM   I/O   KER   : 06

    11  0   1   0   1   1   RAM   RAM   CRL   BAS   RAM   CHRR  KER   : 05

    7   0   0   1   1   1   RAM   RAM   CRL   CRH   RAM   I/O   KER   : 04

    6   0   0   1   1   0   RAM   RAM   RAM   CRH   RAM   I/O   KER   : 03

    3   0   0   0   1   1   RAM   RAM   CRL   CRH   RAM   CHRR  KER   : 02

    2   0   0   0   1   0   RAM   RAM   RAM   CRH   RAM   CHRR  KER   : 01

    28  1   1   1   0   0   RAM   RAM   RAM   RAM   RAM   RAM   RAM   : 00
    24  1   1   0   0   0   RAM   RAM   RAM   RAM   RAM   RAM   RAM
    12  0   1   1   0   0   RAM   RAM   RAM   RAM   RAM   RAM   RAM
    8   0   1   0   0   0   RAM   RAM   RAM   RAM   RAM   RAM   RAM
    4   0   0   1   0   0   RAM   RAM   RAM   RAM   RAM   RAM   RAM
    1   0   0   0   0   1   RAM   RAM   RAM   RAM   RAM   RAM   RAM
    0   0   0   0   0   0   RAM   RAM   RAM   RAM   RAM   RAM   RAM

*/

const PLA::Line PLA::line[14] = {
    /*
    PLA::Line {{
        {ram0_w, ram0_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r},
        {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r},  }},
    PLA::Line {{
        {ram0_w, ram0_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r},
        {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, romh_r}, {ram_w, romh_r}, {ram_w, ram_r}, {ram_w, charr_r}, {ram_w, kern_r}, {ram_w, kern_r},  }},
    PLA::Line {{
        {ram0_w, ram0_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r},
        {ram_w, roml_r}, {ram_w, roml_r}, {ram_w, romh_r}, {ram_w, romh_r}, {ram_w, ram_r}, {ram_w, charr_r}, {ram_w, kern_r}, {ram_w, kern_r},  }},
    PLA::Line {{
        {ram0_w, ram0_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r},
        {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, romh_r}, {ram_w, romh_r}, {ram_w, ram_r}, {io_w, io_r}, {ram_w, kern_r}, {ram_w, kern_r},  }},
    PLA::Line {{
        {ram0_w, ram0_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r},
        {ram_w, roml_r}, {ram_w, roml_r}, {ram_w, romh_r}, {ram_w, romh_r}, {ram_w, ram_r}, {io_w, io_r}, {ram_w, kern_r}, {ram_w, kern_r},  }},
    PLA::Line {{
        {ram0_w, ram0_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r},
        {ram_w, roml_r}, {ram_w, roml_r}, {ram_w, bas_r}, {ram_w, bas_r}, {ram_w, ram_r}, {ram_w, charr_r}, {ram_w, kern_r}, {ram_w, kern_r},  }},
    PLA::Line {{
        {ram0_w, ram0_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r},
        {ram_w, roml_r}, {ram_w, roml_r}, {ram_w, bas_r}, {ram_w, bas_r}, {ram_w, ram_r}, {io_w, io_r}, {ram_w, kern_r}, {ram_w, kern_r},  }},
    PLA::Line {{
        {ram0_w, ram0_r}, {none_w, none_r}, {none_w, none_r}, {none_w, none_r}, {none_w, none_r}, {none_w, none_r}, {none_w, none_r}, {none_w, none_r},
        {roml_w, roml_r}, {roml_w, roml_r}, {none_w, none_r}, {none_w, none_r}, {none_w, none_r}, {io_w, io_r}, {romh_w, romh_r}, {romh_w, romh_r},  }},
    PLA::Line {{
        {ram0_w, ram0_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r},
        {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, charr_r}, {ram_w, ram_r}, {ram_w, ram_r},  }},
    PLA::Line {{
        {ram0_w, ram0_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r},
        {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, charr_r}, {ram_w, kern_r}, {ram_w, kern_r},  }},
    PLA::Line {{
        {ram0_w, ram0_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r},
        {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, bas_r}, {ram_w, bas_r}, {ram_w, ram_r}, {ram_w, charr_r}, {ram_w, kern_r}, {ram_w, kern_r},  }},
    PLA::Line {{
        {ram0_w, ram0_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r},
        {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {io_w, io_r}, {ram_w, ram_r}, {ram_w, ram_r},  }},
    PLA::Line {{
        {ram0_w, ram0_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r},
        {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {io_w, io_r}, {ram_w, kern_r}, {ram_w, kern_r},  }},
    PLA::Line {{
        {ram0_w, ram0_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, ram_r},
        {ram_w, ram_r}, {ram_w, ram_r}, {ram_w, bas_r}, {ram_w, bas_r}, {ram_w, ram_r}, {io_w, io_r}, {ram_w, kern_r}, {ram_w, kern_r},  }},
    */
    PLA::Line {{ // 00: modes 0, 1, 4, 8, 12, 24, 28
        { ram0_w,   ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,
          ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w   },
        { ram0_r,   ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,
          ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r   } }},
    PLA::Line {{ // 01: mode 2
        { ram0_w,   ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,
          ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w   },
        { ram0_r,   ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,
          ram_r,    ram_r,    romh_r,   romh_r,   ram_r,    charr_r,  kern_r,   kern_r } }},
    PLA::Line {{ // 02: mode 3
        { ram0_w,   ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,
          ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w   },
        { ram0_r,   ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,
          roml_r,   roml_r,   romh_r,   romh_r,   ram_r,    charr_r,  kern_r,   kern_r } }},
    PLA::Line {{ // 03: mode 6
        { ram0_w,   ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,
          ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    io_w,     ram_w,    ram_w   },
        { ram0_r,   ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,
          ram_r,    ram_r,    romh_r,   romh_r,   ram_r,    io_r,     kern_r,   kern_r } }},
    PLA::Line {{ // 04: mode 7
        { ram0_w,   ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,
          ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    io_w,     ram_w,    ram_w   },
        { ram0_r,   ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,
          roml_r,   roml_r,   romh_r,   romh_r,   ram_r,    io_r,     kern_r,   kern_r } }},
    PLA::Line {{ // 05: mode 11
        { ram0_w,   ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,
          ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w   },
        { ram0_r,   ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,
          roml_r,   roml_r,   bas_r,    bas_r,    ram_r,    charr_r,  kern_r,   kern_r } }},
    PLA::Line {{ // 06: mode 15
        { ram0_w,   ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,
          ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    io_w,     ram_w,    ram_w   },
        { ram0_r,   ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,
          roml_r,   roml_r,   bas_r,    bas_r,    ram_r,    io_r,     kern_r,   kern_r } }},
    PLA::Line {{ // 07: modes 16..23
        { ram0_w,   none_w,   none_w,   none_w,   none_w,   none_w,   none_w,   none_w,
          roml_w,   roml_w,   none_w,   none_w,   none_w,   io_w,     romh_w,   romh_w },
        { ram0_r,   none_r,   none_r,   none_r,   none_r,   none_r,   none_r,   none_r,
          roml_r,   roml_r,   none_r,   none_r,   none_r,   io_r,     romh_r,   romh_r } }},
    PLA::Line {{ // 08: modes 9, 25
        { ram0_w,   ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,
          ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w   },
        { ram0_r,   ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,
          ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    charr_r,  ram_r,    ram_r   } }},
    PLA::Line {{ // 09: modes 10, 26
        { ram0_w,   ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,
          ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w   },
        { ram0_r,   ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,
          ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    charr_r,  kern_r,   kern_r } }},
    PLA::Line {{ // 10: modes 27
        { ram0_w,   ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,
          ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w   },
        { ram0_r,   ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,
          ram_r,    ram_r,    bas_r,    bas_r,    ram_r,    charr_r,  kern_r,   kern_r } }},
    PLA::Line {{ // 11: modes 5, 13, 29
        { ram0_w,   ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,
          ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    io_w,     ram_w,    ram_w   },
        { ram0_r,   ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,
          ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    io_r,     ram_r,    ram_r   } }},
    PLA::Line {{ // 12: modes 14, 30
        { ram0_w,   ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,
          ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    io_w,     ram_w,    ram_w   },
        { ram0_r,   ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,
          ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    io_r,     kern_r,   kern_r } }},
    PLA::Line {{ // 13: mode 31
        { ram0_w,   ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    ram_w,
          ram_w,    ram_w,    ram_w,    ram_w,    ram_w,    io_w,     ram_w,    ram_w   },
        { ram0_r,   ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,    ram_r,
          ram_r,    ram_r,    bas_r,    bas_r,    ram_r,    io_r,     kern_r,   kern_r } }},
};


const u8 PLA::Mode_to_line[32] = {
    0,  0,  1,  2,  0, 11,  3,  4, 0,  8,  9,  5,  0, 11, 12,  6,
    7,  7,  7,  7,  7,  7,  7,  7, 0,  8,  9, 10,  0, 11, 12, 13,
};
