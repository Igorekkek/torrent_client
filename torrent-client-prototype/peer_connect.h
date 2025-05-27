#pragma once

#include "tcp_connect.h"
#include "peer.h"
#include "torrent_file.h"
#include "piece_storage.h"
#include <atomic>

/*
Структура, хранящая информацию о доступности частей скачиваемого файла у данного пира
*/
class PeerPiecesAvailability {
public:
    PeerPiecesAvailability();

    /*
    bitfield -- массив байтов, в котором i-й бит означает наличие или отсутствие i-й части файла у пира
    */
    explicit PeerPiecesAvailability(std::string bitfield);

    bool IsPieceAvailable(size_t pieceIndex) const;

    void SetPieceAvailability(size_t pieceIndex);

    size_t Size() const;
private:
    std::string bitfield_;
};

/*
Класс, представляющий соединение с одним пиром.
*/
class PeerConnect {
public:
    PeerConnect(const Peer& peer, const TorrentFile& tf, std::string selfPeerId, PieceStorage& pieceStorage);

    void Run();

    void Terminate();

    bool Failed() const;
private:
    const TorrentFile& tf_;
    TcpConnect socket_; 
    const std::string selfPeerId_;  
    std::string peerId_; 
    PeerPiecesAvailability piecesAvailability_;
    std::atomic<bool> terminated_; 
    bool choked_;  
    PiecePtr pieceInProgress_;
    PieceStorage& pieceStorage_;
    bool pendingBlock_;  
    std::atomic<bool> failed_; 

    bool isCorrectPeerResponse(const std::string& handshake, const std::string& ProtocolName, const std::string& response);
    std::string createHandShakeMessage(const std::string& ProtocolName);
    void PerformHandshake();

    bool EstablishConnection();

    void ReceiveBitfield();

    void SendInterested();

    void RequestPiece();

    void MainLoop();
};
