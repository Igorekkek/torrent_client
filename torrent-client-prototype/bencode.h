#pragma once

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
#include "torrent_file.h"

namespace Bencode {
    
    using Bvalue = std::variant<int64_t, std::string, std::vector<std::shared_ptr<struct BNode>>, std::map<std::string, std::shared_ptr<struct BNode>>>;
    using Bint = int64_t;
    using Bstring = std::string;
    using Blist = std::vector<std::shared_ptr<struct BNode>>;
    using Bmap = std::map<std::string, std::shared_ptr<struct BNode>>;
    
    struct BNode {
        Bvalue value;
        BNode(const Bvalue& value);
    };
    
    class BencodeParser {
    private:
        const std::string& data;
        int pos;
    
        char peek() const;
        char get();
        std::shared_ptr<BNode> parseValue();
        std::shared_ptr<BNode> parseInt();
        std::shared_ptr<BNode> parseString();
        std::shared_ptr<BNode> parseList();
        std::shared_ptr<BNode> parseMap();
    
    public:
        BencodeParser(const std::string& data_);
        std::shared_ptr<BNode> parse();
    };

    class BencodeEncoder {
    private:
        const std::shared_ptr<BNode> data;
    
        std::string encodeValue(const std::shared_ptr<BNode> data) const;
        std::string encodeString(const Bstring& value) const;
        std::string encodeInt(const Bint& value) const;
        std::string encodeList(const Blist& value) const;
        std::string encodeMap(const Bmap& value) const;
    
    public:
        BencodeEncoder(std::shared_ptr<BNode> data_);
        std::string encode();
    };
    
    std::string extract_info(const std::shared_ptr<BNode> info);
    std::string sha1_raw(const std::string& input);
};