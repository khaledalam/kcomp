#pragma once

#include <cstdint>
#include <vector>

// Simple LZ77 with match encoding for preprocessing
// Match format: escape_byte, offset_hi, offset_lo, length
// Minimum match length: 4 bytes

std::vector<uint8_t> LZ77Compress(const std::vector<uint8_t> &in);
std::vector<uint8_t> LZ77Decompress(const std::vector<uint8_t> &in);

// Run-Length Encoding for files with many repeated bytes
// Format: ESC (0xFF), byte, length-4  for runs of 4+ same bytes
std::vector<uint8_t> RLECompress(const std::vector<uint8_t> &in);
std::vector<uint8_t> RLEDecompress(const std::vector<uint8_t> &in);

// Delta encoding - encodes differences between consecutive bytes
std::vector<uint8_t> DeltaEncode(const std::vector<uint8_t> &in);
std::vector<uint8_t> DeltaDecode(const std::vector<uint8_t> &in);

// Pattern repeat encoding - for files that repeat a short pattern
// Format: pattern_len (1 byte), pattern (N bytes), repeat_count (4 bytes)
std::vector<uint8_t> PatternEncode(const std::vector<uint8_t> &in);
std::vector<uint8_t> PatternDecode(const std::vector<uint8_t> &in);

// Word tokenizer - replaces common words/patterns with single bytes
std::vector<uint8_t> WordEncode(const std::vector<uint8_t> &in);
std::vector<uint8_t> WordDecode(const std::vector<uint8_t> &in);

// Record interleave - groups bytes by position within fixed-size records
// Improves compression of structured data like TAR (512-byte blocks)
std::vector<uint8_t> RecordInterleave(const std::vector<uint8_t> &in, uint16_t record_size);
std::vector<uint8_t> RecordDeinterleave(const std::vector<uint8_t> &in);

// Sparse encoding - efficient encoding for files with many zeros
// Format: 0x00 = literal zero, 0x01-0xFE = literal byte, 0xFF = escape
// 0xFF 0x00 len_hi len_lo = run of zeros (len+4)
// 0xFF 0xFF = literal 0xFF
std::vector<uint8_t> SparseEncode(const std::vector<uint8_t> &in);
std::vector<uint8_t> SparseDecode(const std::vector<uint8_t> &in);
