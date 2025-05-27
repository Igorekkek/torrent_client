#pragma once

#include "torrent_file.h"
#include "piece.h"
#include <queue>
#include <string>
#include <unordered_set>
#include <mutex>
#include <fstream>
#include <filesystem>

/*
 * Хранилище информации о частях скачиваемого файла.
 * В этом классе отслеживается информация о том, какие части файла осталось скачать
 */
class PieceStorage {
public:
    PieceStorage(const TorrentFile& tf, const std::filesystem::path& outputDirectory, int percent);

    PiecePtr GetNextPieceToDownload();

    void PieceProcessed(const PiecePtr& piece);

    bool QueueIsEmpty() const;

    size_t PiecesSavedToDiscCount() const;

    size_t TotalPiecesCount() const;

    void CloseOutputFile();

    const std::vector<size_t>& GetPiecesSavedToDiscIndices() const;

    size_t PiecesInProgressCount() const;

private:
    std::queue<PiecePtr> remainPieces_;
    std::fstream outputFile_; 
    const int64_t pieceLength_;
    std::vector<size_t> savedPieceId_;
    int64_t readingCounter_;
    const int64_t totalPiecesCount_;
    mutable std::mutex mtx_;

    void SavePieceToDisk(const PiecePtr& piece);
};
