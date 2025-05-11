#pragma once
#include "bitreader.h"
#include <image.h>
#include <unordered_map>
#include <unordered_set>
#include <huffman.h>

class Reader {
    struct Channel {
        uint16_t h1;
        uint16_t v1;
        size_t dqt_idx;
    };

    struct ChannelInfo {
        size_t huffman_dc;
        size_t huffman_ac;
    };

    //  0  1  2  3  4  5  6  7
    //  8  9 10 11 12 13 14 15
    // 16 17 18 19 20 21 22 23
    // 24 25 26 27 28 29 30 31
    // 32 33 34 35 36 37 38 39
    // 40 41 42 43 44 45 46 47
    // 48 49 50 51 52 53 54 55
    // 56 57 58 59 60 61 62 63

    const std::vector<size_t> k_zigzag_order_ = {
        0,  1,  8,  16, 9,  2,  3,  10, 17, 24, 32, 25, 18, 11, 4,  5,  12, 19, 26, 33, 40, 48,
        41, 34, 27, 20, 13, 6,  7,  14, 21, 28, 35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23,
        30, 37, 44, 51, 58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63};

    const uint16_t k_marker_ = 0xFF;
    const uint16_t k_soi_ = 0xD8;
    const uint16_t k_eoi_ = 0xD9;
    const uint16_t k_com_ = 0xFE;
    const uint16_t k_app_from_ = 0xE0;
    const uint16_t k_app_to_ = 0xEF;
    const uint16_t k_dqt_ = 0xDB;
    const uint16_t k_sof0_ = 0xC0;
    const uint16_t k_dht_ = 0xC4;
    const uint16_t k_sos_ = 0xDA;

    const std::unordered_set<uint16_t> k_markers_{k_marker_, k_soi_, k_eoi_,  k_com_, k_app_from_,
                                                  k_app_to_, k_dqt_, k_sof0_, k_dht_, k_sos_};

public:
    Reader(std::istream& input);
    Image DecodeImage();

private:
    uint16_t ReadMarker();
    void ReadSOI();
    void ReadEOI();
    void ReadCOM();
    void ReadApp();
    void ReadDQT();
    void ReadSOF0();
    void ReadDHT();
    void ReadSOS();
    size_t ReadBlockSize();
    RGB YCbCrToRGB(double y, double cb, double cr);

    BitReader bit_reader_;
    std::unordered_map<size_t, std::vector<std::vector<int>>> dqt_;
    std::unordered_map<size_t, Channel> channels_;
    std::unordered_map<size_t, ChannelInfo> channels_info_;
    std::unordered_map<size_t, HuffmanTree> huffmans_[2];
    bool read_sof0_ = false;
    Image image_;
    uint16_t h1_max_ = 0;
    uint16_t v1_max_ = 0;
    std::unordered_map<size_t, int> prev_dc_val_;
};