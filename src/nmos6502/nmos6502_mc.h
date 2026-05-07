#ifndef NMOS6502_MC_H_INCLUDED
#define NMOS6502_MC_H_INCLUDED

#include "nmos6502.h"


// Micro-code

namespace NMOS6502::MC {
    
    // mem-op. direction
    enum RW : u8 { w = 0, r = 1 }; // TODO: move out...

} // namespace NMOS6502::MC


#endif // NMOS6502_MC_H_INCLUDED
