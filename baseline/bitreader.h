#pragma once

#include <istream>
#include <vector>
#include <cstdint>

class BitReader {
public:
    BitReader(std::istream& input);

    uint8_t Read1Byte();
    void ReadSos();

    bool GetBit(size_t idx);
    bool NextBit();

    size_t GetIndex();

    bool flag_ = 0;
    int cnt = 0;

private:
    std::istream& input_;
    uint8_t buf_ = 0;
    int pos_ = 7;
    std::vector<bool> buffer_sos_;
    size_t idx_ = 0;
};