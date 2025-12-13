#pragma once

#include <cstdint>
#include <vector>

std::vector<uint8_t> CompressPPM1(const std::vector<uint8_t> &in);
std::vector<uint8_t> DecompressPPM1(const std::vector<uint8_t> &in);
std::vector<uint8_t> CompressPPM2(const std::vector<uint8_t> &in);
std::vector<uint8_t> DecompressPPM2(const std::vector<uint8_t> &in);
std::vector<uint8_t> CompressPPM4(const std::vector<uint8_t> &in);
std::vector<uint8_t> DecompressPPM4(const std::vector<uint8_t> &in);
