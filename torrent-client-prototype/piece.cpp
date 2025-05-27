#include "byte_tools.h"
#include "piece.h"
#include <iostream>
#include <algorithm>

namespace {
constexpr size_t BLOCK_SIZE = 1 << 14;
}

Piece::Piece(size_t index, size_t length, std::string hash) : index_(index), length_(length), hash_(hash) {
    int64_t lastBlockLength = length % BLOCK_SIZE;
    int64_t blockCount = length / BLOCK_SIZE + 1;
    if (lastBlockLength == 0) {
        lastBlockLength = BLOCK_SIZE;
        blockCount--;
    }

    blocks_.resize(blockCount);
    for (int blocknum = 0; blocknum < blockCount; ++blocknum) {
        blocks_[blocknum].piece = index;
        blocks_[blocknum].length = (blocknum == (int)length - 1 ? lastBlockLength : BLOCK_SIZE);
        blocks_[blocknum].offset = blocknum * BLOCK_SIZE;
        blocks_[blocknum].status = Block::Status::Missing;
    }
};

bool Piece::HashMatches() const {
    return hash_ == GetHash();
}

Block* Piece::FirstMissingBlock() {
    for (auto& block : blocks_) {
        if (block.status == Block::Status::Missing) {
            block.status = Block::Status::Pending;
            return &block;
        }
    }
    throw std::runtime_error("<FirstMissingBlock> have not missing blocks");
}

size_t Piece::GetIndex() const {
    return index_;
}

void Piece::SaveBlock(size_t blockOffset, std::string data) {
    int blockIdx = blockOffset / BLOCK_SIZE;
    if (blocks_[blockIdx].length != data.size()) throw std::runtime_error("try to safe block of another size");
    blocks_[blockIdx].data = data;
    blocks_[blockIdx].status = Block::Status::Retrieved;
}

bool Piece::AllBlocksRetrieved() const {
    for (const auto& block : blocks_) {
        if (block.status == Block::Status::Missing || block.status == Block::Status::Pending) {
            return false;
        }
    }
    return true;
}

std::string Piece::GetData() const {
    std::string fullData;
    for (const auto& block : blocks_) {
        fullData += block.data;
    }
    return fullData;
}

std::string Piece::GetDataHash() const {
    return CalculateSHA1(GetData());
}

const std::string& Piece::GetHash() const {
    return hash_;
}

void Piece::Reset() {
    for (auto& block : blocks_) {
        block.data.clear();
        block.status = Block::Status::Missing;
    }
}
