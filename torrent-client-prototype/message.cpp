#include <iostream>
#include "message.h"
#include "byte_tools.h"

#include <stdexcept>

Message Message::Parse(const std::string& messageString) {

    size_t length = messageString.size();

    if (length == 0) {
        return Message{MessageId::KeepAlive, 0, ""};
    }

    uint8_t idByte = static_cast<uint8_t>(messageString[0]);
    if (idByte > static_cast<uint8_t>(MessageId::Port)) {
        throw std::runtime_error("Unknown message ID");
    }

    MessageId id = static_cast<MessageId>(idByte);
    std::string payload = messageString.substr(1);

    return Message{id, length, payload};
}

Message Message::Init(MessageId id, const std::string& payload) {
    if (id == MessageId::KeepAlive) return Message{id, 0, ""};
    return Message{id, 1 + payload.size(), payload};
}

std::string Message::ToString() const {
    if (id == MessageId::KeepAlive) return IntToBytes(0);

    std::string result;
    int length = static_cast<int>(messageLength);
    result += IntToBytes(length);
    result.push_back(static_cast<char>(id));
    result += payload;

    return result;
}

