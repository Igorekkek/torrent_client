#include "byte_tools.h"
#include <cstdint>
#include <openssl/sha.h>
#include <stdexcept>

int BytesToInt(std::string_view bytes) {
    if (bytes.size() != 4) {
        throw std::invalid_argument("BytesToInt expects exactly 4 bytes");
    }

    return (static_cast<uint8_t>(bytes[0]) << 24) |
           (static_cast<uint8_t>(bytes[1]) << 16) |
           (static_cast<uint8_t>(bytes[2]) << 8)  |
           (static_cast<uint8_t>(bytes[3]));
}

std::string IntToBytes(int value) {
    std::string bytes(4, '\0');
    bytes[0] = static_cast<char>((value >> 24) & 0xFF);
    bytes[1] = static_cast<char>((value >> 16) & 0xFF);
    bytes[2] = static_cast<char>((value >> 8) & 0xFF);
    bytes[3] = static_cast<char>(value & 0xFF);
    return bytes;
}

std::string CalculateSHA1(const std::string& msg) {
    unsigned char hash[SHA_DIGEST_LENGTH];

    SHA1(reinterpret_cast<const unsigned char*>(msg.data()), msg.size(), hash);

    return std::string(reinterpret_cast<char*>(hash), SHA_DIGEST_LENGTH);
}

std::string HexEncode(const std::string& input) {
    std::string res;
    for (auto c : input) {
        if ('0' <= c && c <= '9' || 'a' <= c <= 'f') res.push_back(c);
        else res.push_back(' ');
    }
    return res;
}