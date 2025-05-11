#include "bitreader.h"

#include <iostream>

BitReader::BitReader(std::istream& input) : input_(input) {
    // Read1Byte();
}

uint8_t BitReader::Read1Byte() {
    buf_ = input_.get();
    return buf_;
}

void BitReader::ReadSos() {
    while (true) {
        uint8_t byte = input_.get();
        if (byte == 0xFF) {
            if (input_.peek() != 0x00) {
                input_.unget();
                break;
            }
            input_.get();
        }
        for (int b = 7; b >= 0; --b) {
            if ((byte >> b) & 1) {
                buffer_sos_.emplace_back(1);
            } else {
                buffer_sos_.emplace_back(0);
            }
        }
    }
}

bool BitReader::GetBit(size_t idx) {
    if (idx >= buffer_sos_.size()) {
        throw std::out_of_range("SOS buffer out of range");
    }
    return buffer_sos_[idx];
}

bool BitReader::NextBit() {
    return GetBit(idx_++);
}

size_t BitReader::GetIndex() {
    return idx_;
}
