#pragma once

#include <string>
#include "torrent_file.h"
#include "bencode.h"
#include "peer.h"

class TorrentTracker {
public:
    TorrentTracker(const std::string& url);

    void UpdatePeers(const TorrentFile& tf, std::string peerId, int port);
    const std::vector<Peer>& GetPeers() const;

private:
    std::string url_;
    std::vector<Peer> peers_;

    std::string urlEncode(const std::string& value);
    std::string InfoHashToHexString(const std::string& info_hash);
};