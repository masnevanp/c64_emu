#ifndef CARTRIDGE_H_INCLUDED
#define CARTRIDGE_H_INCLUDED

#include "common.h"
#include "files.h"



namespace Cartridge {


bool attach(const Files::CRT& crt, Expansion_ctx& exp_ctx);
void detach(Expansion_ctx& exp_ctx);


} // namespace Cartridge


#endif // CARTRIDGE_H_INCLUDED