#pragma once

#include <cstdint>
#include <vector>

// LZ77 with optimal parsing and large window (up to 16MB)
std::vector<uint8_t> LZOptCompress(const std::vector<uint8_t> &in);
std::vector<uint8_t> LZOptDecompress(const std::vector<uint8_t> &in);
