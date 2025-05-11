#include "reader.h"

#include <string>
#include <cmath>
#include <fft.h>
#include <iostream>
#include <valarray>

// #define uint16_t uint16_t

Reader::Reader(std::istream& input) : bit_reader_(input) {
}

uint16_t Reader::ReadMarker() {
    uint16_t byte = bit_reader_.Read1Byte();
    if (byte != k_marker_) {
        throw std::runtime_error("Expected marker begin");
    }
    byte = bit_reader_.Read1Byte();
    if (k_app_from_ <= byte && byte <= k_app_to_) {
        byte = k_app_from_;
    }
    if (!k_markers_.contains(byte)) {
        throw std::runtime_error("Expected marker");
    }
    return byte;
}

void Reader::ReadSOI() {
    uint16_t marker = ReadMarker();
    if (marker != k_soi_) {
        throw std::runtime_error("Expected SOI marker");
    }
}

void Reader::ReadEOI() {
    uint16_t marker = ReadMarker();
    if (marker != k_eoi_) {
        throw std::runtime_error("Expected EOI marker");
    }
}

void Reader::ReadCOM() {
    size_t siz = ReadBlockSize();
    std::string com;
    com.reserve(siz);
    for (size_t i = 0; i < siz; ++i) {
        com += bit_reader_.Read1Byte();
    }
    image_.SetComment(com);
}

void Reader::ReadApp() {
    size_t siz = ReadBlockSize();
    for (size_t i = 0; i < siz; ++i) {
        bit_reader_.Read1Byte();
    }
}

void Reader::ReadDQT() {
    size_t siz = ReadBlockSize();
    size_t read_bytes = 0;
    while (read_bytes < siz) {
        uint16_t info = bit_reader_.Read1Byte();
        ++read_bytes;
        uint16_t value_size = (info & 0xF0) >> 4;
        if (value_size > 1) {
            throw std::runtime_error("Invalid value_size in DQT");
        }
        uint16_t idx = info & 0x0F;
        std::vector<std::vector<int>> dqt(8, std::vector<int>(8, 0));
        for (size_t ptr = 0; ptr < 64; ++ptr) {
            uint16_t value = bit_reader_.Read1Byte();
            ++read_bytes;
            if (value_size) {
                value <<= 8;
                value |= bit_reader_.Read1Byte();
                ++read_bytes;
            }
            dqt[k_zigzag_order_[ptr] / 8][k_zigzag_order_[ptr] % 8] = value;
        }
        dqt_[idx] = dqt;
    }
    if (read_bytes != siz) {
        throw std::runtime_error("Invalid dqt format");
    }
}

void Reader::ReadSOF0() {
    if (read_sof0_) {
        throw std::runtime_error("Duplicate SOF0");
    }
    read_sof0_ = true;
    size_t siz = ReadBlockSize();
    size_t read_bytes = 0;
    size_t precision = bit_reader_.Read1Byte();
    ++read_bytes;
    if (precision != 8) {
        throw std::runtime_error("Invalid precision");
    }
    size_t height = bit_reader_.Read1Byte();
    height <<= 8;
    height |= bit_reader_.Read1Byte();
    read_bytes += 2;
    size_t width = bit_reader_.Read1Byte();
    width <<= 8;
    width |= bit_reader_.Read1Byte();
    read_bytes += 2;
    if (height == 0 || width == 0) {
        throw std::runtime_error("Invalid sizes in SOF0");
    }
    image_.SetSize(width, height);
    size_t channels_cnt = bit_reader_.Read1Byte();
    ++read_bytes;
    if (channels_cnt != 1 && channels_cnt != 3) {
        throw std::runtime_error("Invalid number of channels in SOF0");
    }
    for (size_t i = 0; i < channels_cnt; ++i) {
        size_t id = bit_reader_.Read1Byte();
        ++read_bytes;
        uint16_t info = bit_reader_.Read1Byte();
        channels_[id].h1 = (info & 0xF0) >> 4;
        channels_[id].v1 = (info & 0x0F);
        ++read_bytes;
        h1_max_ = std::max(h1_max_, channels_[id].h1);
        v1_max_ = std::max(v1_max_, channels_[id].v1);
        channels_[id].dqt_idx = bit_reader_.Read1Byte();
        ++read_bytes;
    }
    if (read_bytes != siz) {
        throw std::runtime_error("Invalid sof0 format");
    }
}

void Reader::ReadDHT() {
    size_t siz = ReadBlockSize();
    size_t read_bytes = 0;
    while (read_bytes < siz) {
        uint16_t info = bit_reader_.Read1Byte();
        ++read_bytes;
        uint16_t coefs_class = (info & 0xF0) >> 4;
        if (coefs_class > 1) {
            throw std::runtime_error("Invalid value_size in DQT");
        }
        uint16_t idx = info & 0x0F;

        std::vector<uint8_t> code_lengths(16, 0);
        std::vector<uint8_t> values;
        size_t values_size = 0;
        for (size_t i = 0; i < 16; ++i) {
            code_lengths[i] = bit_reader_.Read1Byte();
            ++read_bytes;
            values_size += code_lengths[i];
        }
        values.reserve(values_size);
        for (size_t i = 0; i < values_size; ++i) {
            values.emplace_back(bit_reader_.Read1Byte());
            ++read_bytes;
        }
        huffmans_[coefs_class][idx].Build(code_lengths, values);
    }
    if (read_bytes != siz) {
        throw std::runtime_error("Invalid DHT format");
    }
}

void Reader::ReadSOS() {
    if (!read_sof0_) {
        throw std::runtime_error("No SOF0 content");
    }
    size_t siz = ReadBlockSize();
    size_t read_bytes = 0;
    size_t channels_cnt = bit_reader_.Read1Byte();
    ++read_bytes;
    if (channels_cnt != 1 && channels_cnt != 3) {
        throw std::runtime_error("Invalid number of channels in SOS");
    }
    for (size_t i = 0; i < channels_cnt; ++i) {
        size_t id = bit_reader_.Read1Byte();
        ++read_bytes;
        uint16_t info = bit_reader_.Read1Byte();
        channels_info_[id].huffman_dc = (info & 0xF0) >> 4;
        channels_info_[id].huffman_ac = (info & 0x0F);
        ++read_bytes;
    }
    uint16_t byte = bit_reader_.Read1Byte();
    ++read_bytes;
    if (byte != 0x00) {
        throw std::runtime_error("Invalid SOS format");
    }
    byte = bit_reader_.Read1Byte();
    ++read_bytes;
    if (byte != 0x3F) {
        throw std::runtime_error("Invalid SOS format");
    }
    byte = bit_reader_.Read1Byte();
    ++read_bytes;
    if (byte != 0x00) {
        throw std::runtime_error("Invalid SOS format");
    }

    if (read_bytes != siz) {
        throw std::runtime_error("Invalid SOS format");
    }

    size_t blocks_w = (image_.Width() + 8 * h1_max_ - 1) / (8 * h1_max_);
    size_t blocks_h = (image_.Height() + 8 * v1_max_ - 1) / (8 * v1_max_);

    bit_reader_.ReadSos();
    bit_reader_.flag_ = 1;
    for (size_t block_i = 0; block_i < blocks_h; ++block_i) {
        for (size_t block_j = 0; block_j < blocks_w; ++block_j) {
            std::vector<std::vector<std::vector<std::vector<std::vector<int>>>>> mcu(
                channels_cnt + 1, std::vector<std::vector<std::vector<std::vector<int>>>>(
                                      v1_max_, std::vector<std::vector<std::vector<int>>>(
                                                   h1_max_, std::vector<std::vector<int>>(
                                                                8, std::vector<int>(8, 0)))));
            for (size_t ch = 1; ch < channels_cnt + 1; ++ch) {
                if (!channels_.contains(ch)) {
                    throw std::runtime_error("No meta about channel");
                }
                if (!channels_info_.contains(ch)) {
                    throw std::runtime_error("No info about channel");
                }
                for (int i = 0; i < channels_[ch].v1; ++i) {
                    for (int j = 0; j < channels_[ch].h1; ++j) {
                        int dc00_len = 0;
                        while (!huffmans_[0][channels_info_[ch].huffman_dc].Move(
                            bit_reader_.NextBit(), dc00_len)) {
                        }
                        int val = 0;
                        for (int k = 0; k < dc00_len; ++k) {
                            val <<= 1;
                            val |= bit_reader_.NextBit();
                        }
                        if ((val & (1 << (dc00_len - 1))) == 0) {
                            val = val - (1 << dc00_len) + 1;
                        }
                        mcu[ch][i][j][0][0] = val;
                        mcu[ch][i][j][0][0] += prev_dc_val_[ch];
                        prev_dc_val_[ch] = mcu[ch][i][j][0][0];

                        size_t ptr = 1;
                        for (; ptr < 64;) {
                            val = 0;
                            while (!huffmans_[1][channels_info_[ch].huffman_ac].Move(
                                bit_reader_.NextBit(), val)) {
                            }
                            if (val == 0) {
                                break;
                            }
                            size_t add_zero = (val & 0xF0) >> 4;
                            for (size_t c = 0; c < add_zero; ++c) {
                                mcu[ch][i][j][k_zigzag_order_[ptr] / 8][k_zigzag_order_[ptr] % 8] =
                                    0;
                                ++ptr;
                            }
                            int ac_len = val & 0x0F;
                            val = 0;
                            for (int k = 0; k < ac_len; ++k) {
                                val <<= 1;
                                val |= bit_reader_.NextBit();
                            }
                            if ((val & (1 << (ac_len - 1))) == 0) {
                                val = val - (1 << ac_len) + 1;
                            }
                            mcu[ch][i][j][k_zigzag_order_[ptr] / 8][k_zigzag_order_[ptr] % 8] = val;
                            ++ptr;
                        }
                        for (; ptr < 64; ++ptr) {
                            mcu[ch][i][j][k_zigzag_order_[ptr] / 8][k_zigzag_order_[ptr] % 8] = 0;
                        }
                        if (!dqt_.contains(channels_[ch].dqt_idx)) {
                            throw std::runtime_error("No dqt matrix for channel");
                        }
                        std::vector<double> input(64);
                        std::vector<double> output(64, 0);
                        for (size_t ii = 0; ii < 8; ++ii) {
                            for (size_t jj = 0; jj < 8; ++jj) {
                                mcu[ch][i][j][ii][jj] *= dqt_[channels_[ch].dqt_idx][ii][jj];
                                input[ii * 8 + jj] = mcu[ch][i][j][ii][jj];
                            }
                        }
                        DctCalculator calculator(8, &input, &output);
                        calculator.Inverse();

                        for (size_t ii = 0; ii < 8; ++ii) {
                            for (size_t jj = 0; jj < 8; ++jj) {
                                mcu[ch][i][j][ii][jj] = std::round(output[ii * 8 + jj]);
                                mcu[ch][i][j][ii][jj] =
                                    std::min(std::max(0, mcu[ch][i][j][ii][jj] + 128), 255);
                            }
                        }
                    }
                }
            }
            for (size_t i = 0; i < v1_max_ * 8; ++i) {
                for (size_t j = 0; j < h1_max_ * 8; ++j) {

                    std::vector<int> ycbcr(3, 0);
                    for (size_t c = 1; c <= channels_cnt; ++c) {
                        int a = i * channels_[c].v1 / v1_max_;
                        int b = j * channels_[c].h1 / h1_max_;
                        ycbcr[c - 1] = mcu[c][a / 8][b / 8][a % 8][b % 8];
                    }
                    RGB pixel;
                    if (channels_cnt == 1) {
                        pixel = {ycbcr[0], ycbcr[0], ycbcr[0]};
                    } else {
                        pixel = YCbCrToRGB(ycbcr[0], ycbcr[1], ycbcr[2]);
                    }
                    size_t y = block_i * 8 * v1_max_ + i;
                    size_t x = block_j * 8 * h1_max_ + j;
                    if (y < image_.Height() && x < image_.Width()) {
                        image_.SetPixel(y, x, pixel);
                    }
                }
            }
        }
    }
}

size_t Reader::ReadBlockSize() {
    size_t siz = bit_reader_.Read1Byte();
    siz <<= 8;
    siz |= bit_reader_.Read1Byte();
    siz -= 2;
    return siz;
}

RGB Reader::YCbCrToRGB(double y, double cb, double cr) {
    int r = std::round(y + 1.402 * (cr - 128));
    int g = std::round(y - 0.34414 * (cb - 128) - 0.71414 * (cr - 128));
    int b = std::round(y + 1.772 * (cb - 128));

    r = std::min(std::max(0, r), 255);
    g = std::min(std::max(0, g), 255);
    b = std::min(std::max(0, b), 255);

    return {r, g, b};
}

Image Reader::DecodeImage() {
    ReadSOI();
    while (true) {
        auto marker = ReadMarker();
        if (marker == k_soi_) {
            throw std::runtime_error("SOI only in begin");
        }
        if (marker == k_eoi_) {
            throw std::runtime_error("EOI only in end");
        }
        if (marker == k_com_) {
            ReadCOM();
        } else if (marker == k_app_from_) {
            ReadApp();
        } else if (marker == k_dqt_) {
            ReadDQT();
        } else if (marker == k_sof0_) {
            ReadSOF0();
        } else if (marker == k_dht_) {
            ReadDHT();
        } else if (marker == k_sos_) {
            ReadSOS();
            ReadEOI();
            break;
        } else {
            throw std::runtime_error("Invalid marker");
        }
    }
    return image_;
}