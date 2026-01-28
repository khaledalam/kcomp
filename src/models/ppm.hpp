#pragma once

#include <cstdint>
#include <vector>

std::vector<uint8_t> CompressPPM1(const std::vector<uint8_t> &in);
std::vector<uint8_t> DecompressPPM1(const std::vector<uint8_t> &in);
std::vector<uint8_t> CompressPPM2(const std::vector<uint8_t> &in);
std::vector<uint8_t> DecompressPPM2(const std::vector<uint8_t> &in);
std::vector<uint8_t> CompressPPM3(const std::vector<uint8_t> &in);
std::vector<uint8_t> DecompressPPM3(const std::vector<uint8_t> &in);
std::vector<uint8_t> CompressPPM4(const std::vector<uint8_t> &in);
std::vector<uint8_t> DecompressPPM4(const std::vector<uint8_t> &in);
std::vector<uint8_t> CompressPPM5(const std::vector<uint8_t> &in);
std::vector<uint8_t> DecompressPPM5(const std::vector<uint8_t> &in);
std::vector<uint8_t> CompressPPM6(const std::vector<uint8_t> &in);
std::vector<uint8_t> DecompressPPM6(const std::vector<uint8_t> &in);

// Hybrid: Auto-selects best algorithm
std::vector<uint8_t> CompressHybrid(const std::vector<uint8_t> &in);
std::vector<uint8_t> DecompressHybrid(const std::vector<uint8_t> &in);
