#include "bencode.h"
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <openssl/sha.h>
#include <fstream>
#include <variant>
#include <list>
#include <map>
#include <sstream>
#include <memory>

namespace Bencode {

BNode::BNode(const Bvalue& value) : value(value) {}

char BencodeParser::peek() const {
    return data[pos];
}

char BencodeParser::get() {
    return data[pos++];
}

std::shared_ptr<BNode> BencodeParser::parseValue() {
    char c = peek();
    if (c == 'i') return parseInt();
    else if (c == 'l') return parseList();
    else if (c == 'd') return parseMap();
    else if ('0' <= c && c <= '9') return parseString();
    throw 3;
}

std::shared_ptr<BNode> BencodeParser::parseInt() {
    pos++;
    int end = data.find('e', pos);
    Bvalue val = (Bint)std::stoll(data.substr(pos, end - pos));
    pos = end + 1;
    return std::make_shared<BNode>(val);
}

std::shared_ptr<BNode> BencodeParser::parseString() {
    int endLen = data.find(':', pos);
    int len = std::stoi(data.substr(pos, endLen - pos));
    Bvalue val = (Bstring)data.substr(endLen + 1, len);
    pos = endLen + 1 + len;
    return std::make_shared<BNode>(val);
}

std::shared_ptr<BNode> BencodeParser::parseList() {
    pos++;
    Blist list;
    while (peek() != 'e') {
        list.push_back(parseValue());
    }
    pos++;
    return std::make_shared<BNode>((Bvalue)list);
}

std::shared_ptr<BNode> BencodeParser::parseMap() {
    pos++;
    Bmap map;
    while (peek() != 'e') {
        std::string key = std::get<std::string>(parseString()->value);
        std::shared_ptr<BNode> value = parseValue();
        map[key] = value;
    }
    pos++;
    return std::make_shared<BNode>((Bvalue)map);
}

BencodeParser::BencodeParser(const std::string& data_) : data(data_), pos(0) {}

std::shared_ptr<BNode> BencodeParser::parse() {
    return parseValue();
}

BencodeEncoder::BencodeEncoder(std::shared_ptr<BNode> data_) : data(data_) {}

std::string BencodeEncoder::encode() {
    return encodeValue(data);
}

std::string BencodeEncoder::encodeValue(const std::shared_ptr<BNode> data) const {
    if (std::holds_alternative<Bint>(data->value)) {
        return encodeInt(std::get<Bint>(data->value));
    } else if (std::holds_alternative<Bstring>(data->value)) {
        return encodeString(std::get<Bstring>(data->value));
    } else if (std::holds_alternative<Blist>(data->value)) {
        return encodeList(std::get<Blist>(data->value));
    } else if (std::holds_alternative<Bmap>(data->value)) {
        return encodeMap(std::get<Bmap>(data->value));
    } else {
        throw std::runtime_error("Unknown type of BNode");
    }
}

std::string BencodeEncoder::encodeString(const Bstring& value) const {
    return std::to_string(value.size()) + ":" + value;
}

std::string BencodeEncoder::encodeInt(const Bint& value) const {
    return "i" + std::to_string(value) + "e";
}

std::string BencodeEncoder::encodeList(const Blist& value) const {
    std::ostringstream oss;
    oss << "l";
    for (auto& e : value) {
        oss << encodeValue(e);
    } 
    oss << "e";
    return oss.str();
}

std::string BencodeEncoder::encodeMap(const Bmap& value) const {
    std::ostringstream oss;
    oss << "d";
    for (auto& [key, val] : value) {
        oss << encodeString(key) << encodeValue(val);
    }
    oss << "e";
    return oss.str();
}

std::string extract_info(const std::shared_ptr<BNode> info) {
    auto be = BencodeEncoder(info);
    return be.encode();
}

std::string sha1_raw(const std::string& input) {
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char*>(input.data()), input.size(), hash);
    return std::string(reinterpret_cast<char*>(hash), SHA_DIGEST_LENGTH);
}

};