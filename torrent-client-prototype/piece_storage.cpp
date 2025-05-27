#include "piece_storage.h"
#include <mutex>
#include <memory>
#include <fstream>
#include <filesystem>
#include <iostream>

PieceStorage::PieceStorage(const TorrentFile& tf, const std::filesystem::path& outputDirectory, int percent) : pieceLength_(tf.pieceLength), readingCounter_(0), totalPiecesCount_(tf.pieceHashes.size()) {
    int64_t lastPieceLength = tf.length % tf.pieceLength;
    if (lastPieceLength == 0) lastPieceLength = tf.pieceLength;

    int countPieces = (double)percent * (double)tf.pieceHashes.size() / 100.0;
    std::cout << "Count Pieces = " << countPieces << std::endl;
    for (int pieceIdx = 0; pieceIdx < countPieces; ++pieceIdx) {
        remainPieces_.push(std::make_shared<Piece>(
            pieceIdx, 
            (pieceIdx == (int)tf.pieceHashes.size() - 1 ? lastPieceLength : tf.pieceLength), 
            tf.pieceHashes[pieceIdx]
        ));
    } 

    if (!std::filesystem::exists(outputDirectory)) {
        std::filesystem::create_directories(outputDirectory);
        std::cout << "Creat directories" << std::endl;
    } else {
        std::cout << "Have directories" << std::endl;
    }

    // create file and expand file size
    std::filesystem::path outputFilePath = outputDirectory / tf.name;

    if (!std::filesystem::exists(outputFilePath)) {
        std::ofstream tmp(outputFilePath, std::ios::binary);
        tmp.close();
    }

    std::filesystem::resize_file(outputFilePath, tf.length);
    outputFile_.open(outputFilePath, std::ios::binary | std::ios::in | std::ios::out);

    std::cout << "File size = " << std::filesystem::file_size(outputFilePath) << std::endl;
    if (std::filesystem::file_size(outputFilePath) != tf.length) {
        throw std::runtime_error(std::string("can't expand file size to ") + std::to_string(tf.length));
    }
}

PiecePtr PieceStorage::GetNextPieceToDownload() {
    std::lock_guard lock(mtx_);
    readingCounter_++;
    auto result = remainPieces_.front();
    remainPieces_.pop();
    return result;
}

void PieceStorage::PieceProcessed(const PiecePtr& piece) {
    std::lock_guard lock(mtx_);
    readingCounter_--;
    savedPieceId_.push_back(piece->GetIndex());
    SavePieceToDisk(piece);
}

bool PieceStorage::QueueIsEmpty() const {
    std::lock_guard lock(mtx_);
    return remainPieces_.empty();
}

size_t PieceStorage::TotalPiecesCount() const {
    std::lock_guard lock(mtx_);
    return totalPiecesCount_;
}

void PieceStorage::CloseOutputFile() {
    std::lock_guard lock(mtx_);
    if (outputFile_.is_open()) {
        outputFile_.close();
    }
}

const std::vector<size_t>& PieceStorage::GetPiecesSavedToDiscIndices() const {
    std::lock_guard lock(mtx_);
    return savedPieceId_;
}

size_t PieceStorage::PiecesInProgressCount() const {
    std::lock_guard lock(mtx_);
    return readingCounter_;
}

size_t PieceStorage::PiecesSavedToDiscCount() const {
    std::lock_guard lock(mtx_);
    return savedPieceId_.size();
}

void PieceStorage::SavePieceToDisk(const PiecePtr& piece) {
    if (outputFile_.is_open()) {
        outputFile_.seekp(piece->GetIndex() * pieceLength_);
        auto data = piece->GetData();
        outputFile_.write(data.c_str(), data.size());
        std::cout << "Download piece : " << piece->GetIndex() << std::endl;
    } else {
        throw std::runtime_error("output file closed");
    }
}
