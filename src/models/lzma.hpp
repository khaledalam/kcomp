#pragma once

#include <cstdint>
#include <vector>

// LZMA-style compressor with:
// - 1MB sliding window (larger than LZ77's 64KB)
// - Optimal parsing via dynamic programming (finds minimum cost path)
// - Better match encoding (variable-length codes for offsets)
// - Lazy matching with longer chains

// Maximum dictionary size (1MB)
constexpr size_t LZMA_DICT_SIZE = 1 << 20;

// Minimum/maximum match lengths
constexpr int LZMA_MIN_MATCH = 3;
constexpr int LZMA_MAX_MATCH = 273;  // Standard LZMA max

// Compress using LZMA-style optimal parsing
// Returns LZ-encoded data suitable for entropy coding
std::vector<uint8_t> LZMACompress(const std::vector<uint8_t> &in);
std::vector<uint8_t> LZMADecompress(const std::vector<uint8_t> &in);

// Finite State Entropy (FSE) - tANS-style entropy coding
// More efficient than range coding for symbols with known distributions
std::vector<uint8_t> FSECompress(const std::vector<uint8_t> &in);
std::vector<uint8_t> FSEDecompress(const std::vector<uint8_t> &in);

// Combined LZMA+FSE pipeline
std::vector<uint8_t> LZMAFSECompress(const std::vector<uint8_t> &in);
std::vector<uint8_t> LZMAFSEDecompress(const std::vector<uint8_t> &in);
