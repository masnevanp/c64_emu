#ifndef CARTRIDGE_H_INCLUDED
#define CARTRIDGE_H_INCLUDED

#include "common.h"
#include "files.h"


/*
    Cartridge support is possible only due to these (thanks!):
        https://vice-emu.sourceforge.io/vice_17.html
        https://sourceforge.net/p/vice-emu/code/HEAD/tree/trunk/vice/src/c64/cart/
*/


namespace Cartridge {

bool attach(const Files::CRT& crt, Expansion_ctx& exp_ctx);
void detach(Expansion_ctx& exp_ctx);

}


#endif // CARTRIDGE_H_INCLUDED