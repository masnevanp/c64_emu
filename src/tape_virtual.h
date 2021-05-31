#ifndef TAPE_VIRTUAL_H_INCLUDED
#define TAPE_VIRTUAL_H_INCLUDED


#include "system.h"
#include "file_utils.h"


namespace Tape_virtual {

enum Routine { load = 0x01, save = 0x02, };


void install_kernal_traps(u8* kernal, u8 trap_opc);

bool on_trap(System::CPU& cpu, u8* ram, loader& load_file);


} // namespace Tape_virtual


#endif // TAPE_VIRTUAL_H_INCLUDED
