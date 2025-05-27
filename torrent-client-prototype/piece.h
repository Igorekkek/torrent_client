#pragma once

#include <string>
#include <vector>
#include <optional>
#include <memory>

struct Block {

    enum Status {
        Missing = 0,
        Pending,
        Retrieved,
    };

    uint32_t piece;  // id части файла, к которой относится данный блок
    uint32_t offset;  // смещение начала блока относительно начала части файла в байтах
    uint32_t length;  // длина блока в байтах
    Status status;  // статус загрузки данного блока
    std::string data;  // бинарные данные
};

/*
 * Часть скачиваемого файла
 */
class Piece {
public:
    /*
     * index -- номер части файла, нумерация начинается с 0
     * length -- длина части файла. Все части, кроме последней, имеют длину, равную `torrentFile.pieceLength`
     * hash -- хеш-сумма части файла, взятая из `torrentFile.pieceHashes`
     */
    Piece(size_t index, size_t length, std::string hash);

    bool HashMatches() const;

    Block* FirstMissingBlock();

    size_t GetIndex() const;

    void SaveBlock(size_t blockOffset, std::string data);

    bool AllBlocksRetrieved() const;

    std::string GetData() const;

    std::string GetDataHash() const;

    const std::string& GetHash() const;

    void Reset();

private:
    const size_t index_, length_;
    const std::string hash_;
    std::vector<Block> blocks_;
};

using PiecePtr = std::shared_ptr<Piece>;
