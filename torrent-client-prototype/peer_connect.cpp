#include "byte_tools.h"
#include "peer_connect.h"
#include "message.h"
#include <iostream>
#include <sstream>
#include <utility>
#include <cassert>
#include <climits>

using namespace std::chrono_literals;

PeerPiecesAvailability::PeerPiecesAvailability() : bitfield_("") {}

PeerPiecesAvailability::PeerPiecesAvailability(std::string bitfield) : bitfield_(std::move(bitfield)) {}

bool PeerPiecesAvailability::IsPieceAvailable(size_t pieceIndex) const {
    size_t byteIndex = pieceIndex / CHAR_BIT;
    int bitOffset = CHAR_BIT - 1 - (pieceIndex % CHAR_BIT);
    return (bitfield_[byteIndex] & (1 << bitOffset)) != 0;
}

void PeerPiecesAvailability::SetPieceAvailability(size_t pieceIndex) {
    size_t byteIndex = pieceIndex / CHAR_BIT;
    int bitOffset = CHAR_BIT - 1 - (pieceIndex % CHAR_BIT);

    bitfield_[byteIndex] |= (1 << bitOffset);
}

size_t PeerPiecesAvailability::Size() const {
    return bitfield_.size() * CHAR_BIT;
}

PeerConnect::PeerConnect(const Peer& peer, const TorrentFile &tf, std::string selfPeerId, PieceStorage& pieceStorage) : 
                                tf_(tf), 
                                socket_(peer.ip, peer.port, 1s, 10s), 
                                selfPeerId_(selfPeerId), 
                                peerId_(""),
                                terminated_(false),
                                choked_(true),
                                pieceInProgress_(nullptr),
                                pieceStorage_(pieceStorage),
                                pendingBlock_(false)  {
}

void PeerConnect::Run() {
    while (!terminated_) {
        if (EstablishConnection()) {
            std::cout << "Connection established to peer" << std::endl;
            MainLoop();
        } else {
            std::cerr << "Cannot establish connection to peer" << std::endl;
            Terminate();
        }
    }
}

std::string PeerConnect::createHandShakeMessage(const std::string& ProtocolName) {
    std::string handshake;
    handshake.push_back(static_cast<char>(ProtocolName.size())); 
    handshake += ProtocolName;
    handshake += std::string(8, 0);  
    handshake += tf_.infoHash;  
    handshake += selfPeerId_; 
    return handshake;
}

bool PeerConnect::isCorrectPeerResponse(const std::string& handshake, const std::string& ProtocolName, const std::string& response) {
    if (handshake.size() != response.size()) return false;
    int protLen = ProtocolName.size();
    int info_hash_size = tf_.infoHash.size();
    return handshake.substr(0, 1 + protLen) == response.substr(0, 1 + protLen) &&
           tf_.infoHash == response.substr(1 + protLen + 8, info_hash_size);
}

void PeerConnect::PerformHandshake() {
    socket_.EstablishConnection();

    const std::string ProtocolName = "BitTorrent protocol";
    std::string handshake = createHandShakeMessage(ProtocolName); 
    socket_.SendData(handshake);

    std::string data = socket_.ReceiveData(handshake.size());
    if (!isCorrectPeerResponse(handshake, ProtocolName, data)) {
        throw std::runtime_error("Bad answer from peer");
    }
    peerId_ = data.substr(1 + ProtocolName.size() + 8 + 20, 20);
}

bool PeerConnect::EstablishConnection() {
    try {
        PerformHandshake();
        ReceiveBitfield();
        SendInterested();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to establish connection with peer " << socket_.GetIp() << ":" <<
            socket_.GetPort() << " -- " << e.what() << std::endl;
        return false;
    }
}



void PeerConnect::ReceiveBitfield() {
    while (true) {
        auto message = Message::Parse(socket_.ReceiveData(0));

        if (message.id == MessageId::KeepAlive) {
            continue;
        }

        if (message.id == MessageId::Unchoke) {
            choked_ = false;
            return;
        }

        if (message.id == MessageId::BitField) {
            piecesAvailability_ = PeerPiecesAvailability(message.payload);
            return;
        }
        
        throw std::runtime_error("<ReceiveBitfield> undefined messageID");
    }
}

void PeerConnect::SendInterested() {
    try {
        int mesid = 2;
        auto message = Message::Init(static_cast<MessageId>(mesid), ""); 
        socket_.SendData(message.ToString());
    } catch (const std::exception& e) {
        throw std::runtime_error("error in send interester");
    }
}

void PeerConnect::RequestPiece() {
    if (pieceInProgress_ != nullptr && pieceInProgress_->AllBlocksRetrieved()) {
        pieceStorage_.PieceProcessed(pieceInProgress_);
        pieceInProgress_ = nullptr;
    }
    while (!pieceInProgress_) {
        // найти новую часть
        if (pieceStorage_.QueueIsEmpty()) break;
        auto piece = pieceStorage_.GetNextPieceToDownload();
        if (!piecesAvailability_.IsPieceAvailable(piece->GetIndex())) continue;
        if (piece->AllBlocksRetrieved()) continue;
        pieceInProgress_ = piece;
    }
    
    if (pieceInProgress_) {
        // отправить блок на скачивание
        Block* blockptr = pieceInProgress_->FirstMissingBlock();
        std::string data = IntToBytes(blockptr->piece) +
                           IntToBytes(blockptr->offset) +
                           IntToBytes(blockptr->length);
        std::string request = Message::Init(MessageId::Request, data).ToString();
        socket_.SendData(request);
        pendingBlock_ = true;
    } else {
        // больше нечего получать
        terminated_ = true;
    }
}

void PeerConnect::Terminate() {
    std::cerr << "Terminate" << std::endl;
    terminated_ = true;
}

void PeerConnect::MainLoop() {
    while (!terminated_) {
        auto message = Message::Parse(socket_.ReceiveData());
        // std::cout << "Parse message" << std::endl;
        if (message.id == MessageId::Choke) {
            choked_ = true;
        } else if (message.id == MessageId::Unchoke) {
            choked_ = false;
        } else if (message.id == MessageId::Have) {
            int64_t pieceIdx = BytesToInt(message.payload);
            piecesAvailability_.SetPieceAvailability(pieceIdx);
        } else if (message.id == MessageId::Piece) {
            if (message.payload.size() < 8) throw std::runtime_error("error in piece message"); 
            int64_t pieceIndex = BytesToInt(message.payload.substr(0, 4));
            if (pieceIndex != pieceInProgress_->GetIndex()) throw std::runtime_error("peice of another index");
            int64_t blockOffset = BytesToInt(message.payload.substr(4, 4));
            std::string data = message.payload.substr(8);
            pieceInProgress_->SaveBlock(blockOffset, data);
            pendingBlock_ = false;
        }
        if (!choked_ && !pendingBlock_) {
            RequestPiece();
        }
    }
}

bool PeerConnect::Failed() const {
    return failed_;
}