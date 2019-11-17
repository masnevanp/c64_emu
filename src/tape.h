#ifndef TAPE_H_INCLUDED
#define TAPE_H_INCLUDED


#include "system.h"
#include "file_utils.h"


// Loading/saving of raw prg files supported (i.e. no .TAP support etc.)
namespace Tape {

enum Routine { load = 0x01, save = 0x02, };


class Virtual {
public:
    static void install_kernal_traps(u8* kernal, u8 trap_opc);

    static bool on_trap(System::CPU& cpu, u8* ram, Loader& loader) {
        u8 tape_routine = cpu.d;
        switch (tape_routine) {
            case Routine::load:
                load(cpu, ram, loader);
                return true;
            case Routine::save:
                save(cpu, ram);
                return true;
            default:
                std::cout << "\nUnknown tape routine: " << (int)tape_routine;
                return false;
        }
    }

private:
    static void load(System::CPU& cpu, u8* ram, Loader& loader);
    static void save(System::CPU& cpu, u8* ram);

};


} // namespace Tape


#endif // TAPE_H_INCLUDED
