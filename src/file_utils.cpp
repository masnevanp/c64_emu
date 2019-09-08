
#include <regex>
#include "file_utils.h"
#include "utils.h"


struct Basic_line {
    u16 num;
    std::string text;
};

using Basic_listing = std::vector<Basic_line>;

// treats text as... text.. (that is, no tokenization)
std::vector<u8> to_bin(const Basic_listing& bl) {
    std::vector<u8> bin;

    u16 addr = 0x0801;
    bin.push_back(addr); // load addr. (lo)
    bin.push_back(addr >> 8); // load addr. (hi)

    for(auto& line : bl) {
        u16 line_len = 2 + 2 + line.text.length() + 1;
        addr += line_len;
        bin.push_back(addr); // lo
        bin.push_back(addr >> 8); // hi
        bin.push_back(line.num); // lo
        bin.push_back(line.num >> 8); // hi
        for (auto c : line.text) bin.push_back(c); // text
        bin.push_back(0x00); // end of line
    }

    bin.push_back(0x00); // end of program (null pointer)
    bin.push_back(0x00);

    return bin;
}


// generates a basic program listing of the dir listing
std::vector<u8> dir_basic_listing(const std::string& dir) {
    static const u8 first_line[] = {
        0x9b, 0x36, 0x34, 0x00 // 'list' token, '6', '4', eol
    };

    Basic_listing bl{
        { 0x00, std::string((char*)first_line) }, // 0 LIST64
        { 0x40, std::string("::") + (char)0x12 + "  " + as_upper(dir) + "  " } // 64 <header>
    };

    for (const auto& name : dir_listing(dir)) {
        bl.push_back({ 0x40, std::string(": \"") + name + '"' }); // 64 : "<name>"
    }

    return to_bin(bl);
}


std::vector<u8> read_file(const std::string& dir, const std::string& name_pattern) {
    if ((name_pattern.find("*") == std::string::npos)
            && (name_pattern.find("?") == std::string::npos)) {
        return read_file(dir + "/" + name_pattern);
    }

    // TODO: special chars (e.g. "+" --> "\+")
    std::string rs = replace(name_pattern, "*", ".*");
    rs = replace(rs, "?", ".");
    std::regex re(rs);
    for (const auto& name : dir_listing(dir)) {
        if (std::regex_match(name, re)) {
            return read_file(dir + "/" + name);
        }
    }

    std::cout << "\nNot found: '" << name_pattern << "'";
    return std::vector<u8>(0); // TODO: something else...
}
