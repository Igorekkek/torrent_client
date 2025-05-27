#include "torrent_tracker.h"
#include "piece_storage.h"
#include "peer_connect.h"
#include "byte_tools.h"
#include <cassert>
#include <iostream>
#include <filesystem>
#include <random>
#include <thread>
#include <algorithm>

namespace fs = std::filesystem;

std::mutex cerrMutex, coutMutex;

std::string RandomString(size_t length) {
    std::random_device random;
    std::string result;
    result.reserve(length);
    for (size_t i = 0; i < length; ++i) {
        result.push_back(random() % ('Z' - 'A') + 'A');
    }
    return result;
}

const std::string PeerId = "TESTAPPDONTWORRY" + RandomString(4);

bool RunDownloadMultithread(PieceStorage& pieces, const TorrentFile& torrentFile, const std::string& ourId, const TorrentTracker& tracker) {
    using namespace std::chrono_literals;

    std::vector<std::thread> peerThreads;
    std::vector<std::shared_ptr<PeerConnect>> peerConnections;

    for (const Peer& peer : tracker.GetPeers()) {
        peerConnections.emplace_back(std::make_shared<PeerConnect>(peer, torrentFile, ourId, pieces));
    }

    peerThreads.reserve(peerConnections.size());

    for (auto& peerConnectPtr : peerConnections) {
        peerThreads.emplace_back(
                [peerConnectPtr] () {
                    bool tryAgain = true;
                    int attempts = 0;
                    do {
                        try {
                            ++attempts;
                            peerConnectPtr->Run();
                        } catch (const std::runtime_error& e) {
                            std::lock_guard<std::mutex> cerrLock(cerrMutex);
                            std::cerr << "Runtime error: " << e.what() << std::endl;
                        } catch (const std::exception& e) {
                            std::lock_guard<std::mutex> cerrLock(cerrMutex);
                            std::cerr << "Exception: " << e.what() << std::endl;
                        } catch (...) {
                            std::lock_guard<std::mutex> cerrLock(cerrMutex);
                            std::cerr << "Unknown error" << std::endl;
                        }
                        tryAgain = peerConnectPtr->Failed() && attempts < 3;
                    } while (tryAgain);
                }
        );
    }

    for (std::thread& thread : peerThreads) {
        thread.join();
    }

    return false;
}

void DownloadTorrentFile(const TorrentFile& torrentFile, PieceStorage& pieces, const std::string& ourId) {
    std::cout << "Connecting to tracker " << torrentFile.announce << std::endl;
    TorrentTracker tracker(torrentFile.announce);
    bool requestMorePeers = false;
    do {
        tracker.UpdatePeers(torrentFile, ourId, 12345);

        if (tracker.GetPeers().empty()) {
            std::cerr << "No peers found. Cannot download a file" << std::endl;
        }

        std::cout << "Found " << tracker.GetPeers().size() << " peers" << std::endl;
        for (const Peer& peer : tracker.GetPeers()) {
            std::cout << "Found peer " << peer.ip << ":" << peer.port << std::endl;
        }

        requestMorePeers = RunDownloadMultithread(pieces, torrentFile, ourId, tracker);
    } while (requestMorePeers);
}

void TestTorrentFile(const fs::path& file, const fs::path& outputDirectory, int percent) {
    TorrentFile torrentFile;
    try {
        torrentFile = LoadTorrentFile(file);
        std::cout << "Loaded torrent file " << file << ". " << torrentFile.comment << std::endl;
    } catch (const std::invalid_argument& e) {
        std::cerr << e.what() << std::endl;
        return;
    }

    std::filesystem::create_directories(outputDirectory);
    PieceStorage pieces(torrentFile, outputDirectory, percent);

    DownloadTorrentFile(torrentFile, pieces, PeerId);

    // CheckDownloadedPiecesIntegrity(outputDirectory / torrentFile.name, torrentFile, pieces);

}

int main(int argc, char* argv[]) {
    if (argc != 6 || std::string(argv[1]) != "-d" || std::string(argv[3]) != "-p") {
        std::cerr << "Usage: ./torrent-client-prototype -d <output_dir> -p <percent> <.torrent file>\n";
        return 1;
    }

    std::string outputDir = argv[2];
    std::string percentStr = argv[4];
    std::string torrentPathStr = argv[5];

    int percent = std::stoi(percentStr);
    if (percent < 1 || percent > 100) {
        std::cerr << "Percent must be between 1 and 100." << std::endl;
        return 1;
    }

    if (!std::filesystem::exists(torrentPathStr)) {
        std::cerr << "Torrent file does not exist" << std::endl;
        return 2;
    }

    std::filesystem::path outputDirPath(outputDir), torrentPath(torrentPathStr);
    TestTorrentFile(torrentPath, outputDirPath);
    return 0;
}
