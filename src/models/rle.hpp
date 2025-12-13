#pragma once

#include <cstdint>
#include <vector>

std::vector<uint8_t> CompressRLE(const std::vector<uint8_t> &in);
std::vector<uint8_t> DecompressRLE(const std::vector<uint8_t> &in);
