#pragma once

#include <cstdint>
#include <string>

/*
https://wiki.theory.org/BitTorrentSpecification#Messages
*/
enum class MessageId : uint8_t {
    Choke = 0,
    Unchoke,
    Interested,
    NotInterested,
    Have,
    BitField,
    Request,
    Piece,
    Cancel,
    Port,
    KeepAlive,
};

struct Message {
    MessageId id;
    size_t messageLength;
    std::string payload;

    /*
        сообщение без первых 4 байт длины
    */
    static Message Parse(const std::string& messageString);

    static Message Init(MessageId id, const std::string& payload);

    std::string ToString() const;
};
