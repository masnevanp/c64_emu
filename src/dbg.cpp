
#include "dbg.h"
#include <sstream>
#include <iostream>
#include <iomanip>


using namespace NMOS6502;


using Am = Dbg::Instruction::Addr_mode;

// Table data derived from (thanks!):
// - https://github.com/michael-0acf4/opcodes-json-6502
// - https://github.com/Esshahn/pydisass6502/

const Dbg::Instruction instruction[256] = {
    {0x00, 1, Am::impl, "BRK", "BRK"},
    {0x01, 2, Am::indx, "ORA", "ORA ($B2,X)"},
    {0x02, 1, Am::impl, "HLT", "HLT"},
    {0x03, 2, Am::indx, "SLO", "SLO ($B2,X)"},
    {0x04, 2, Am::zp, "NOP", "SLO ($B2),Y"},
    {0x05, 2, Am::zp, "ORA", "ORA $B2"},
    {0x06, 2, Am::zp, "ASL", "ASL $B2"},
    {0x07, 2, Am::zp, "SLO", "SLO $B2"},
    {0x08, 1, Am::impl, "PHP", "PHP"},
    {0x09, 2, Am::imm, "ORA", "ORA #$B2"},
    {0x0a, 1, Am::impl, "ASL", "ASL"},
    {0x0b, 2, Am::imm, "ANC", "ANC #$B2"},
    {0x0c, 3, Am::abs, "NOP", "NOP $B3B2"},
    {0x0d, 3, Am::abs, "ORA", "ORA $B3B2"},
    {0x0e, 3, Am::abs, "ASL", "ASL $B3B2"},
    {0x0f, 3, Am::abs, "SLO", "SLO $B3B2"},
    {0x10, 2, Am::rel, "BPL", "BPL $B2"},
    {0x11, 2, Am::indy, "ORA", "ORA ($B2),Y"},
    {0x12, 1, Am::impl, "HLT", "HLT"},
    {0x13, 2, Am::indy, "SLO", "SLO ($B2),Y"},
    {0x14, 2, Am::zpx, "NOP", "NOP $B2,X"},
    {0x15, 2, Am::zpx, "ORA", "ORA $B2,X"},
    {0x16, 2, Am::zpx, "ASL", "ASL $B2,X"},
    {0x17, 2, Am::zpx, "SLO", "SLO $B2,X"},
    {0x18, 1, Am::impl, "CLC", "CLC"},
    {0x19, 3, Am::absy, "ORA", "ORA $B3B2,Y"},
    {0x1a, 1, Am::impl, "NOP", "NOP"},
    {0x1b, 3, Am::absy, "SLO", "SLO $B3B2,Y"},
    {0x1c, 3, Am::absx, "NOP", "NOP $B3B2,X"},
    {0x1d, 3, Am::absx, "ORA", "ORA $B3B2,X"},
    {0x1e, 3, Am::absx, "ASL", "ASL $B3B2,X"},
    {0x1f, 3, Am::absx, "SLO", "SLO $B3B2,X"},
    {0x20, 3, Am::abs, "JSR", "JSR $B3B2"},
    {0x21, 2, Am::indx, "AND", "AND ($B2,X)"},
    {0x22, 1, Am::impl, "HLT", "HLT"},
    {0x23, 2, Am::indx, "RLA", "RLA ($B2,X)"},
    {0x24, 2, Am::zp, "BIT", "BIT $B2"},
    {0x25, 2, Am::zp, "AND", "AND $B2"},
    {0x26, 2, Am::zp, "ROL", "ROL $B2"},
    {0x27, 2, Am::zp, "RLA", "RLA $B2"},
    {0x28, 1, Am::impl, "PLP", "PLP"},
    {0x29, 2, Am::imm, "AND", "AND #$B2"},
    {0x2a, 1, Am::impl, "ROL", "ROL"},
    {0x2b, 2, Am::imm, "ANC", "ANC #$B2"},
    {0x2c, 3, Am::abs, "BIT", "BIT $B3B2"},
    {0x2d, 3, Am::abs, "AND", "AND $B3B2"},
    {0x2e, 3, Am::abs, "ROL", "ROL $B3B2"},
    {0x2f, 3, Am::abs, "RLA", "RLA $B3B2"},
    {0x30, 2, Am::rel, "BMI", "BMI $B2"},
    {0x31, 2, Am::indy, "AND", "AND ($B2),Y"},
    {0x32, 1, Am::impl, "HLT", "HLT"},
    {0x33, 2, Am::indy, "RLA", "RLA ($B2),Y"},
    {0x34, 2, Am::zpx, "NOP", "NOP $B2,X"},
    {0x35, 2, Am::zpx, "AND", "AND $B2,X"},
    {0x36, 2, Am::zpx, "ROL", "ROL $B2,X"},
    {0x37, 2, Am::zpx, "RLA", "RLA $B2,Y"},
    {0x38, 1, Am::impl, "SEC", "SEC"},
    {0x39, 3, Am::absy, "AND", "AND $B3B2,Y"},
    {0x3a, 1, Am::impl, "NOP", "NOP"},
    {0x3b, 3, Am::absy, "RLA", "RLA $B3B2,Y"},
    {0x3c, 3, Am::absx, "NOP", "NOP $B3B2,X"},
    {0x3d, 3, Am::absx, "AND", "AND $B3B2,X"},
    {0x3e, 3, Am::absx, "ROL", "ROL $B3B2,X"},
    {0x3f, 3, Am::absx, "RLA", "RLA $B3B2,X"},
    {0x40, 1, Am::impl, "RTI", "RTI"},
    {0x41, 2, Am::indx, "EOR", "EOR ($B2,X)"},
    {0x42, 1, Am::impl, "HLT", "HLT"},
    {0x43, 2, Am::indx, "SRE", "SRE ($B2,X)"},
    {0x44, 2, Am::zp, "NOP", "NOP $B2"},
    {0x45, 2, Am::zp, "EOR", "EOR $B2"},
    {0x46, 2, Am::zp, "LSR", "LSR $B2"},
    {0x47, 2, Am::zp, "SRE", "SRE $B2"},
    {0x48, 1, Am::impl, "PHA", "PHA"},
    {0x49, 2, Am::imm, "EOR", "EOR #$B2"},
    {0x4a, 1, Am::impl, "LSR", "LSR"},
    {0x4b, 2, Am::imm, "ALR", "ALR #$B2"},
    {0x4c, 3, Am::abs, "JMP", "JMP $B3B2"},
    {0x4d, 3, Am::abs, "EOR", "EOR $B3B2"},
    {0x4e, 3, Am::abs, "LSR", "LSR $B3B2"},
    {0x4f, 3, Am::abs, "SRE", "SRE $B3B2"},
    {0x50, 2, Am::rel, "BVC", "BVC $B2"},
    {0x51, 2, Am::indy, "EOR", "EOR ($B2),Y"},
    {0x52, 1, Am::impl, "HLT", "HLT"},
    {0x53, 2, Am::indy, "SRE", "SRE ($B2),Y"},
    {0x54, 2, Am::zpx, "NOP", "NOP $B2,X"},
    {0x55, 2, Am::zpx, "EOR", "EOR $B2,X"},
    {0x56, 2, Am::zpx, "LSR", "LSR $B2,X"},
    {0x57, 2, Am::zpx, "SRE", "SRE $B2,X"},
    {0x58, 1, Am::impl, "CLI", "CLI"},
    {0x59, 3, Am::absy, "EOR", "EOR $B3B2,Y"},
    {0x5a, 1, Am::impl, "NOP", "NOP"},
    {0x5b, 3, Am::absy, "SRE", "SRE $B3B2,Y"},
    {0x5c, 3, Am::absx, "NOP", "NOP $B3B2,X"},
    {0x5d, 3, Am::absx, "EOR", "EOR $B3B2,X"},
    {0x5e, 3, Am::absx, "LSR", "LSR $B3B2,X"},
    {0x5f, 3, Am::absx, "SRE", "SRE $B3B2,X"},
    {0x60, 1, Am::impl, "RTS", "RTS"},
    {0x61, 2, Am::indx, "ADC", "ADC ($B2,X)"},
    {0x62, 1, Am::impl, "HLT", "HLT"},
    {0x63, 2, Am::indx, "RRA", "RRA ($B2,X)"},
    {0x64, 2, Am::zp, "NOP", "NOP $B2"},
    {0x65, 2, Am::zp, "ADC", "ADC $B2"},
    {0x66, 2, Am::zp, "ROR", "ROR $B2"},
    {0x67, 2, Am::zp, "RRA", "RRA $B2"},
    {0x68, 1, Am::impl, "PLA", "PLA"},
    {0x69, 2, Am::imm, "ADC", "ADC #$B2"},
    {0x6a, 1, Am::impl, "ROR", "ROR"},
    {0x6b, 2, Am::imm, "ARR", "ARR #$B2"},
    {0x6c, 3, Am::ind, "JMP", "JMP ($B3B2)"},
    {0x6d, 3, Am::abs, "ADC", "ADC $B3B2"},
    {0x6e, 3, Am::abs, "ROR", "ROR $B3B2"},
    {0x6f, 3, Am::abs, "RRA", "RRA $B3B2"},
    {0x70, 2, Am::rel, "BVS", "BVS $B2"},
    {0x71, 2, Am::indy, "ADC", "ADC ($B2),Y"},
    {0x72, 1, Am::impl, "HLT", "HLT"},
    {0x73, 2, Am::indy, "RRA", "RRA ($B2),Y"},
    {0x74, 2, Am::zpx, "NOP", "NOP $B2,X"},
    {0x75, 2, Am::zpx, "ADC", "ADC $B2,X"},
    {0x76, 2, Am::zpx, "ROR", "ROR $B2,X"},
    {0x77, 2, Am::zpx, "RRA", "RRA $B2,X"},
    {0x78, 1, Am::impl, "SEI", "SEI"},
    {0x79, 3, Am::absy, "ADC", "ADC $B3B2,Y"},
    {0x7a, 1, Am::impl, "NOP", "NOP"},
    {0x7b, 3, Am::absy, "RRA", "RRA $B3B2,Y"},
    {0x7c, 3, Am::absx, "NOP", "NOP $B3B2,X"},
    {0x7d, 3, Am::absx, "ADC", "ADC $B3B2,X"},
    {0x7e, 3, Am::absx, "ROR", "ROR $B3B2,X"},
    {0x7f, 3, Am::absx, "RRA", "RRA $B3B2,X"},
    {0x80, 2, Am::imm, "NOP", "NOP #$B2"},
    {0x81, 2, Am::indx, "STA", "STA ($B2,X)"},
    {0x82, 2, Am::imm, "NOP", "NOP #$B2"},
    {0x83, 2, Am::indx, "SAX", "SAX ($B2,X)"},
    {0x84, 2, Am::zp, "STY", "STY $B2"},
    {0x85, 2, Am::zp, "STA", "STA $B2"},
    {0x86, 2, Am::zp, "STX", "STX $B2"},
    {0x87, 2, Am::zp, "SAX", "SAX $B2"},
    {0x88, 1, Am::impl, "DEY", "DEY"},
    {0x89, 2, Am::imm, "NOP", "NOP #$B2"},
    {0x8a, 1, Am::impl, "TXA", "TXA"},
    {0x8b, 2, Am::imm, "XAA", "XAA #$B2"},
    {0x8c, 3, Am::abs, "STY", "STY $B3B2"},
    {0x8d, 3, Am::abs, "STA", "STA $B3B2"},
    {0x8e, 3, Am::abs, "STX", "STX $B3B2"},
    {0x8f, 3, Am::abs, "SAX", "SAX $B3B2"},
    {0x90, 2, Am::rel, "BCC", "BCC $B2"},
    {0x91, 2, Am::indy, "STA", "STA ($B2),Y"},
    {0x92, 1, Am::impl, "HLT", "HLT"},
    {0x93, 2, Am::indy, "AHX", "AHX ($B2),Y"},
    {0x94, 2, Am::zpx, "STY", "STY $B2,X"},
    {0x95, 2, Am::zpx, "STA", "STA $B2,X"},
    {0x96, 2, Am::zpy, "STX", "STX $B2,Y"},
    {0x97, 2, Am::zpy, "SAX", "SAX $B2,Y"},
    {0x98, 1, Am::impl, "TYA", "TYA"},
    {0x99, 3, Am::absy, "STA", "STA $B3B2,Y"},
    {0x9a, 1, Am::impl, "TXS", "TXS"},
    {0x9b, 3, Am::absy, "TAS", "TAS $B3B2,Y"},
    {0x9c, 3, Am::absx, "SHY", "SHY $B3B2,X"},
    {0x9d, 3, Am::absx, "STA", "STA $B3B2,X"},
    {0x9e, 3, Am::absy, "SHX", "SHX $B3B2,Y"},
    {0x9f, 3, Am::absy, "AHX", "AHX $B3B2,Y"},
    {0xa0, 2, Am::imm, "LDY", "LDY #$B2"},
    {0xa1, 2, Am::indx, "LDA", "LDA ($B2,X)"},
    {0xa2, 2, Am::imm, "LDX", "LDX #$B2"},
    {0xa3, 2, Am::indx, "LAX", "LAX ($B2,X)"},
    {0xa4, 2, Am::zp, "LDY", "LDY $B2"},
    {0xa5, 2, Am::zp, "LDA", "LDA $B2"},
    {0xa6, 2, Am::zp, "LDX", "LDX $B2"},
    {0xa7, 2, Am::zp, "LAX", "LAX $B2"},
    {0xa8, 1, Am::impl, "TAY", "TAY"},
    {0xa9, 2, Am::imm, "LDA", "LDA #$B2"},
    {0xaa, 1, Am::impl, "TAX", "TAX"},
    {0xab, 2, Am::imm, "LAX", "LAX #$B2"},
    {0xac, 3, Am::abs, "LDY", "LDY $B3B2"},
    {0xad, 3, Am::abs, "LDA", "LDA $B3B2"},
    {0xae, 3, Am::abs, "LDX", "LDX $B3B2"},
    {0xaf, 3, Am::abs, "LAX", "LAX $B3B2"},
    {0xb0, 2, Am::rel, "BCS", "BCS $B2"},
    {0xb1, 2, Am::indy, "LDA", "LDA ($B2),Y"},
    {0xb2, 1, Am::impl, "HLT", "HLT"},
    {0xb3, 2, Am::indy, "LAX", "LAX ($B2),Y"},
    {0xb4, 2, Am::zpx, "LDY", "LDY $B2,X"},
    {0xb5, 2, Am::zpx, "LDA", "LDA $B2,X"},
    {0xb6, 2, Am::zpy, "LDX", "LDX $B2,X"},
    {0xb7, 2, Am::zpy, "LAX", "LAX $B2,Y"},
    {0xb8, 1, Am::impl, "CLV", "CLV"},
    {0xb9, 3, Am::absy, "LDA", "LDA $B3B2,Y"},
    {0xba, 1, Am::impl, "TSX", "TSX"},
    {0xbb, 3, Am::absy, "LAS", "LAS $B3B2,Y"},
    {0xbc, 3, Am::absx, "LDY", "LDY $B3B2,X"},
    {0xbd, 3, Am::absx, "LDA", "LDA $B3B2,X"},
    {0xbe, 3, Am::absy, "LDX", "LDX $B3B2,X"},
    {0xbf, 3, Am::absy, "LAX", "LAX $B3B2,Y"},
    {0xc0, 2, Am::imm, "CPY", "CPY #$B2"},
    {0xc1, 2, Am::indx, "CMP", "CMP ($B2,X)"},
    {0xc2, 2, Am::imm, "NOP", "NOP #$B2"},
    {0xc3, 2, Am::indx, "DCP", "DCP ($B2,X)"},
    {0xc4, 2, Am::zp, "CPY", "CPY $B2"},
    {0xc5, 2, Am::zp, "CMP", "CMP $B2"},
    {0xc6, 2, Am::zp, "DEC", "DEC $B2"},
    {0xc7, 2, Am::zp, "DCP", "DCP $B2"},
    {0xc8, 1, Am::impl, "INY", "INY"},
    {0xc9, 2, Am::imm, "CMP", "CMP #$B2"},
    {0xca, 1, Am::impl, "DEX", "DEX"},
    {0xcb, 2, Am::imm, "AXS", "SBX #$B2"},
    {0xcc, 3, Am::abs, "CPY", "CPY $B3B2"},
    {0xcd, 3, Am::abs, "CMP", "CMP $B3B2"},
    {0xce, 3, Am::abs, "DEC", "DEC $B3B2"},
    {0xcf, 3, Am::abs, "DCP", "DCP $B3B2"},
    {0xd0, 2, Am::rel, "BNE", "BNE $B2"},
    {0xd1, 2, Am::indy, "CMP", "CMP ($B2),Y"},
    {0xd2, 1, Am::impl, "HLT", "HLT"},
    {0xd3, 2, Am::indy, "DCP", "DCP ($B2),Y"},
    {0xd4, 2, Am::zpx, "NOP", "NOP $B2,X"},
    {0xd5, 2, Am::zpx, "CMP", "CMP $B2,X"},
    {0xd6, 2, Am::zpx, "DEC", "DEC $B2,X"},
    {0xd7, 2, Am::zpx, "DCP", "DCP $B2,X"},
    {0xd8, 1, Am::impl, "CLD", "CLD"},
    {0xd9, 3, Am::absy, "CMP", "CMP $B3B2,Y"},
    {0xda, 1, Am::impl, "NOP", "NOP"},
    {0xdb, 3, Am::absy, "DCP", "DCP $B3B2,Y"},
    {0xdc, 3, Am::absx, "NOP", "NOP $B3B2,X"},
    {0xdd, 3, Am::absx, "CMP", "CMP $B3B2,X"},
    {0xde, 3, Am::absx, "DEC", "DEC $B3B2,X"},
    {0xdf, 3, Am::absx, "DCP", "DCP $B3B2,X"},
    {0xe0, 2, Am::imm, "CPX", "CPX #$B2"},
    {0xe1, 2, Am::indx, "SBC", "SBC ($B2,X)"},
    {0xe2, 2, Am::imm, "NOP", "NOP #$B2"},
    {0xe3, 2, Am::indx, "ISC", "ISB ($B2,X)"},
    {0xe4, 2, Am::zp, "CPX", "CPX $B2"},
    {0xe5, 2, Am::zp, "SBC", "SBC $B2"},
    {0xe6, 2, Am::zp, "INC", "INC $B2"},
    {0xe7, 2, Am::zp, "ISC", "ISC $B2"},
    {0xe8, 1, Am::impl, "INX", "INX"},
    {0xe9, 2, Am::imm, "SBC", "SBC #$B2"},
    {0xea, 1, Am::impl, "NOP", "NOP"},
    {0xeb, 2, Am::imm, "SBC", "SBC #$B2"},
    {0xec, 3, Am::abs, "CPX", "CPX $B3B2"},
    {0xed, 3, Am::abs, "SBC", "SBC $B3B2"},
    {0xee, 3, Am::abs, "INC", "INC $B3B2"},
    {0xef, 3, Am::abs, "ISC", "ISC $B3B2"},
    {0xf0, 2, Am::rel, "BEQ", "BEQ $B2"},
    {0xf1, 2, Am::indy, "SBC", "SBC ($B2),Y"},
    {0xf2, 1, Am::impl, "HLT", "HLT"},
    {0xf3, 2, Am::indy, "ISC", "ISC ($B2),Y"},
    {0xf4, 2, Am::zpx, "NOP", "NOP $B2,X"},
    {0xf5, 2, Am::zpx, "SBC", "SBC $B2,X"},
    {0xf6, 2, Am::zpx, "INC", "INC $B2,X"},
    {0xf7, 2, Am::zpx, "ISC", "ISC $B2,X"},
    {0xf8, 1, Am::impl, "SED", "SED"},
    {0xf9, 3, Am::absy, "SBC", "SBC $B3B2,Y"},
    {0xfa, 1, Am::impl, "NOP", "NOP"},
    {0xfb, 3, Am::absy, "ISC", "ISC $B3B2,Y"},
    {0xfc, 3, Am::absx, "NOP", "NOP $B3B2,X"},
    {0xfd, 3, Am::absx, "SBC", "SBC $B3B2,X"},
    {0xfe, 3, Am::absx, "INC", "INC $B3B2,X"},
    {0xff, 3, Am::absx, "ISC", "ISC $B3B2,X"},
};


std::string Dbg::flag_str(u8 r) {
    std::string s = "";
    s = s
        + std::string((r & 0x80 ) ? "n" : ".") + std::string((r & 0x40 ) ? "v" : ".")
        + std::string((r & 0x20 ) ? "1" : ".") + std::string((r & 0x10 ) ? "b" : ".")
        + std::string((r & 0x08 ) ? "d" : ".") + std::string((r & 0x04 ) ? "i" : ".")
        + std::string((r & 0x02 ) ? "z" : ".") + std::string((r & 0x01 ) ? "c" : ".");
    return s;
}

std::string Dbg::print_u16(uint16_t x) {
    std::stringstream ss;
    ss << std::hex << std::setw(4) << std::setfill('0') << (int)x;
    return ss.str();
}

std::string Dbg::print_u8(uint8_t x) {
    std::stringstream ss;
    ss << std::hex << std::setw(2) << std::setfill('0') << (int)x;
    return ss.str();
}

void Dbg::print_mem(u8* mem, u16 from, u16 to) {
    const int BPR = 8; // bytes/row
    // std::string ascii;
    // TODO: set 'to' to max(to, from+16)
    for (from -= from % BPR; from <= to; ++from) {
        if (from % BPR == 0) {
            std::cout << '\n';
            //std::cout << ascii << '\n';
            //ascii.clear();
            std::cout << print_u16(from) << ": ";
        }
        //ascii += (char)mem[from];
        std::cout << print_u8(mem[from]) << ' ';
    }
    std::cout << std::endl;
}


void Dbg::print_mem(u8* mem, u16 page) {
    print_mem(mem, page << 8, (page << 8) + 0xff);
}


void Dbg::print_status(const Core& cpu, u8* mem) {
    std::cout << "\npc: " << print_u16(cpu.s.pc);
    std::cout << " [ " << print_u8(mem[cpu.s.pc]);
    std::cout << " " << print_u8(mem[(cpu.s.pc+1) & 0xffff]) << " " << print_u8(mem[(cpu.s.pc+2) & 0xffff]);
    std::cout << " ]";
    std::cout << "\nsp: " << print_u16(cpu.s.sp);
    std::cout << " [";
    for (int sp = cpu.s.sp, i = 1; i < 9 && (sp + i) <= 0x1ff; ++i) {
        std::cout << " " << print_u8(mem[sp + i]);
    }
    std::cout << " ]";
    std::cout << "\n a: " << print_u8(cpu.s.a);
    std::cout << "   x: " << print_u8(cpu.s.x);
    std::cout << "   y: " << print_u8(cpu.s.y);
    std::cout << "   p: " << print_u8(cpu.s.p);
    std::cout << " [" << flag_str(cpu.s.p) << "]";
    //// zpaf, a1, a2, a3, a4
    std::cout << "\n\nzp: " << print_u16((cpu.s.zph <<8) | cpu.s.zp);
    std::cout << " a1: " << print_u16((cpu.s.a1h << 8) | cpu.s.a1l);
    std::cout << " a2: " << print_u16(cpu.s.a2);
    std::cout << " a3: " << print_u16(cpu.s.a3);
    std::cout << " a4: " << print_u16(cpu.s.a4);
    std::cout << " d: " << print_u8(cpu.s.d);
    std::cout << " ir: " << print_u8(cpu.s.ir);
    std::cout << "\n";
    std::cout << "\n==> mar: " << print_u16(cpu.mar()) << " (" << Ri16_str[cpu.mop().ar] << ")";
    std::cout << "   mdr: " << (cpu.mrw() == MC::RW::w ? print_u8(cpu.mdr()) : "??");
    std::cout << " (" << Ri8_str[cpu.mop().dr] << ")";
    std::cout << "   r/w: " << MC::RW_str[cpu.mrw()];
    std::cout << std::endl;
    /*
    operator std::string() const {
            return "(" + R16_str[ar] + ") " + R8_str[dr] + " " + RW_str[rw]
                    + PC_inc_str[pc_inc] + " " + MOPC_str[mopc];
        }*/
}


// TODO: still working? (reg. indices changed....)
void Dbg::reg_diff(const Core& cpu) {
    static Reg16 r16_snap[Ri16::_cnt16];
    static Reg8 r8_snap[Ri8::_cnt8];

    std::cout << "\n\n";
    for (int r = 0; r < Ri16::_cnt16; ++r) {
        if (r == Ri16::pc || r == Ri16::sp16 || r >= Ri16::a1) {
            if (r16_snap[r] != cpu.s.r16((Ri16)r)) {
                std::cout << Ri16_str[r] << ": ";
                std::cout << print_u16(r16_snap[r]) << " --> " << print_u16(cpu.s.r16((Ri16)r)) << ", ";
                r16_snap[r] = cpu.s.r16((Ri16)r);
            }
        } else if (r <= Ri16::zp16) {
            int r8 = r * 2;
            if (r8_snap[r8] != cpu.s.r8((Ri8)r8)) {
                std::cout << Ri8_str[r8] << ": ";
                std::cout << print_u8(r8_snap[r8]) << " --> " << print_u8(cpu.s.r8((Ri8)r8)) << ", ";
                r8_snap[r8] = cpu.s.r8((Ri8)r8);
            }
        }
    }
    std::cout << std::endl;
}


void Dbg::step(System& sys, u16 until_pc)
{
    reg_diff(sys.cpu);

    print_status(sys.cpu, sys.mem);
    for (;;) {
        if (sys.tn == 0) {
            std::cout << "==========================================================";
            if (sys.cpu.s.pc == until_pc) break;
        } else std::cout << "----------------------------------------------------------";

        //getchar();
        std::cout << std::dec << "C" << (int)sys.cn << " T" << (int)sys.tn << ": " << std::string(sys.cpu.mop());

        sys.exec_cycle();
        reg_diff(sys.cpu);
        print_status(sys.cpu, sys.mem);

        if (sys.cpu.halted()) { std::cout << "\n[HALTED]"; break; }
    }
    std::cout << "\n\n=================== DONE (" << sys.cn << " cycles) ======================";
    print_status(sys.cpu, sys.mem);
}
