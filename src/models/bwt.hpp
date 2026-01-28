#pragma once

#include <cstdint>
#include <vector>

// Burrows-Wheeler Transform
// Groups similar characters together for better PPM compression
std::vector<uint8_t> BWTEncode(const std::vector<uint8_t>& in, uint32_t& primary_index);
std::vector<uint8_t> BWTDecode(const std::vector<uint8_t>& in, uint32_t primary_index);

// Move-to-Front transform (often used with BWT)
std::vector<uint8_t> MTFEncode(const std::vector<uint8_t>& in);
std::vector<uint8_t> MTFDecode(const std::vector<uint8_t>& in);
