#include "torrent_tracker.h"
#include "bencode.h"
#include "byte_tools.h"
#include <cpr/cpr.h>

#include <iostream>
#include "peer.h"
#include <string>
#include <vector>
#include <fstream>
#include <variant>
#include <iomanip>
#include <list>
#include <map>
#include <sstream>
#include <memory>

using namespace Bencode;

TorrentTracker::TorrentTracker(const std::string& url) : url_(url) {}

void TorrentTracker::UpdatePeers(const TorrentFile& tf, std::string peerId, int port) {
    cpr::Response response;
    for (const auto& url : tf.announceList) {
        std::cout << "Try to connect : " << url << std::endl;
        response = cpr::Get(
            cpr::Url{url},
            cpr::Parameters{
                {"info_hash", tf.infoHash},
                {"peer_id", peerId},
                {"port", std::to_string(port)},
                {"uploaded", "0"},
                {"downloaded", "0"},
                {"left", std::to_string(tf.length)},
                {"compact", "1"}
            },
            cpr::Timeout{1000}
        );

        if (response.status_code == 200) break;
    }

    if (response.status_code != 200) {
        throw std::runtime_error("Failed to connect to tracker");
    }

    auto data = BencodeParser(response.text).parse();
    std::string peersRaw = std::get<Bstring>(std::get<Bmap>(data->value)["peers"]->value);

    for (size_t i = 0; i + 6 <= peersRaw.size(); i += 6) {
        Peer p;
        p.ip = std::to_string((uint8_t)peersRaw[i]) + "." +
               std::to_string((uint8_t)peersRaw[i + 1]) + "." +
               std::to_string((uint8_t)peersRaw[i + 2]) + "." +
               std::to_string((uint8_t)peersRaw[i + 3]);
        p.port = ((uint8_t)peersRaw[i + 4] << 8) | (uint8_t)peersRaw[i + 5];
        peers_.push_back(p);
    }
}

const std::vector<Peer>& TorrentTracker::GetPeers() const {
    return peers_;
}

std::string TorrentTracker::urlEncode(const std::string& value) {
    std::stringstream result;
    for (char c : value) {
        if (std::isdigit(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            result << c;
        } else {
            std::stringstream tmp;
            tmp << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << (int)(unsigned char)c;
            result << '%' << tmp.str();
        }
    }
    return result.str();
}

std::string TorrentTracker::InfoHashToHexString(const std::string& info_hash) {
    std::ostringstream oss;
    for (unsigned char byte : info_hash) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
    }
    return oss.str();
}