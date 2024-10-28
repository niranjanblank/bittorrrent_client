#include "lib/sha1.hpp"
#include<string>
#include<sstream>
#include<iomanip>
// get sha1 from the data
std::string sha1_hash(const std::string& data) {
    SHA1 sha1;
    sha1.update(data);
    std::string hash = sha1.final();
    return hash;
}

// Convert raw binary data to hexadecimal format for debugging
std::string to_hex(const std::string& input) {
    std::ostringstream hex_stream;
    for (unsigned char c: input) {
        hex_stream << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(c);
    }
    return hex_stream.str();
}

