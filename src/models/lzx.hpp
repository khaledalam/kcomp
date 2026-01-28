#pragma once

#include <cstdint>
#include <vector>

// LZX: Advanced LZ compressor with:
// - 64MB sliding window (larger than xz's 32MB)
// - Suffix array for O(n log n) match finding
// - Optimal parsing with full backward references
// - Variable-length offset encoding with position slots

std::vector<uint8_t> LZXCompress(const std::vector<uint8_t>& in);
std::vector<uint8_t> LZXDecompress(const std::vector<uint8_t>& in);
