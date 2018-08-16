# c64_emu (WIP) - An almost unusable [Commodore 64](https://en.wikipedia.org/wiki/Commodore_64) emulator.

A hobby project I work on randomly, and just for fun. Not very usable yet, and it might stay that way... but that's OK. :)

Why do this? Well, the C64 (AKA as 'Tasavallan Tietokone' and 'Kuuslankku') was my first ever computer.

Not really in a distributable format (e.g. no build scripts or makefiles). For development I have been using Code::Blocks on Windows
(project files included in the repo, mostly for my own convenience...).

Tape/disk/cartridge support is not yet implemented, but there is a hack that allows programs to be loaded directly to RAM:
place a 'raw' C64 binary (with first 2 bytes holding the target address) to 'data/prg/' and name it 'bin.prg'. Then in runtime press F11 to load the binary.
Not very convenient, but so far it has been good enough for development/testing (most games I have tried seem to work just fine).

Requirements (to build & run):
* Compiler with C++11 support
* SDL2 library
* C64 ROMs (basic, char, kernal)


## Some notes

### General
* currently works on a little-endian host only

### Host
* no real hosting environment yet
* for now the following keys can be used in runtime:
  * F8 : swap joysticks
  * F9 : reset (warm)
  * F10: reset (cold)
  * F11: load binary program (data/prg/bin.prg)
  * F12: quit

### CPU (6502/6510)
* a cycle accurate (memory access and timing) 6502 core
* for 6510 compatibility the IO port functionality (not 100% there yet) is implemented externally (in System::Banker) 
* implementation totally based on information from the KIM-I Hardware Manual with some verification done using excellent visual6502 tool

### Graphics (VIC-II)
* emulation of the VIC-II chip with cycle level resolution (that is, with '8 pixels/CPU cycle' output)
* not 100% memory access/timing accurate (off by cycle or two here and there, so most demos will propably have glitches)
* for 100% accuracy a (partial) rewrite would be needed (half cycle resolution, or is pixel level required?)
* initially I had a pretty poor understanding of the chip, and how to implement it all (and it propably shows :)

### Audio (SID)
* uses the excellent reSID by Dag Lem for SID emulation
* some kind of output buffering is implemented (with much room for improvement)

### CIA (1&2)
* should be fairly accurate
* serial I/O functionality not yet implemented

### Keyboard mappings (some of them):
* ESC: Run/Stop
* END: Restore
* Tab: <-
* Ctrl: CTRL
* Alt: C=

### Joysticks
* for now KISS: the first 2 that are found during start-up are used
* calibration/configuration not implemented yet
* also keyboard mapppings (hardcoded for now) are available (numpad):
  * 0: fire, 1: left, 2: down, 3: right, 5: up
  * 4: fire, 7: left, 8: down, 9: right, /: up
* F8 to swap the ports


### TODO (major ones):
* tape/disk/cartridge support
* some kind of a hosting environment (for settings, disk image handling, etc...)
* an integrated monitor/debugger (CPU, IO, memory, etc..)
* an assembler (integrated?)
* joysticks: calibration/configuration (including keyboard mappings)


## Thanks to the following for information/inspiration (in no particular order)

* Christian Bauer: [The MOS 6567/6569 video controller (VIC-II) and its application in the Commodore 64](http://www.fairlight.to/docs/text/vic.txt)
* Dag Lem: [reSID](https://en.wikipedia.org/wiki/ReSID)
* Thomas Giesel: [The C64 PLA Dissected, revision 1.1](http://skoe.de/docs/c64-dissected/pla/c64_pla_dissected_a4ds.pdf)
* [C64 Wiki](https://www.c64-wiki.com)
* [Retro Computing](http://retro.hansotten.nl/): [KIM-1 manuals](http://retro.hansotten.nl/6502-sbc/kim-1-manuals-and-software/)
* John West & Marko Mäkelä: [64doc.txt, v1.8](https://github.com/bobsummerwill/VICE/blob/master/doc/html/plain/64doc.txt)
* pepto: [Colodore](http://www.pepto.de/projects/colorvic)
* [The 8-Bit Guy](https://www.youtube.com/user/adric22/featured)
* Jorge Cwik: Flags on Decimal mode in the NMOS 6502
* Ninja/The Dreams: [All_About_Your_64](http://unusedino.de/ec64/technical/aay/c64/)
* [visual6502](http://visual6502.org)
* [6502.org](http://www.6502.org/)
* Klaus Dormann: [6502_65C02_functional_tests](https://github.com/Klaus2m5/6502_65C02_functional_tests)
* Wolfgang Lorenz: [A Software Model of the CIA6526](https://ist.uwaterloo.ca/~schepers/MJK/cia6526.html) and the 'C64 Emulator Test Suite' (data/testsuite-2.15)
* [Commodore 64 Programmers Reference Guide](https://www.commodore.ca/commodore-manuals/commodore-64-programmers-reference-guide/)
