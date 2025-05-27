#include "torrent_file.h"
#include "bencode.h"
#include <vector>
#include <openssl/sha.h>
#include <fstream>
#include <variant>
#include <sstream>
#include <set>

using namespace Bencode;

TorrentFile LoadTorrentFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);

    std::ostringstream oss;
    oss << file.rdbuf();
    std::string data = oss.str();
    BencodeParser pb(data);
    auto root = pb.parse();

    auto rootDict = std::get<Bmap>(root->value);

    TorrentFile result;
    result.announce = std::get<Bstring>(rootDict["announce"]->value);    
    result.comment = std::get<Bstring>(rootDict["comment"]->value);
    
    std::set<std::string> announceSet = {result.announce};
    auto it = rootDict.find("announce-list");
    if (it != rootDict.end() && std::holds_alternative<Blist>(it->second->value)) {
        const auto& list = std::get<Blist>(it->second->value);
        for (const auto& tierNode : list) {
            if (tierNode && std::holds_alternative<Blist>(tierNode->value)) {
                const auto& tier = std::get<Blist>(tierNode->value);
                for (const auto& announceNode : tier) {
                    if (announceNode && std::holds_alternative<Bstring>(announceNode->value)) {
                        announceSet.insert(std::get<Bstring>(announceNode->value));
                    }
                }
            }
        }
    }

    result.announceList.insert(result.announceList.end(), announceSet.begin(), announceSet.end());
    std::cout << "URLs" << std::endl;
    for (const auto& url : result.announceList) {
        std::cout << url << std::endl;
    }

    auto infoDict = std::get<Bmap>(rootDict["info"]->value);
    result.name = std::get<Bstring>(infoDict["name"]->value);
    result.pieceLength = std::get<Bint>(infoDict["piece length"]->value);
    result.length = std::get<Bint>(infoDict["length"]->value);
    
    std::string pieces = std::get<Bstring>(infoDict["pieces"]->value);
    for (int i = 0; i < (int)pieces.size(); i += 20) {
        result.pieceHashes.push_back(pieces.substr(i, 20));
    }

    std::string infoString = extract_info(rootDict["info"]);
    result.infoHash = sha1_raw(infoString);

    return result;
}
