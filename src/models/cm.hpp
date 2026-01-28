#pragma once

#include <cstdint>
#include <vector>

// Context Mixing Compressor (PAQ-style)
// Uses multiple models + neural network mixer for state-of-the-art compression
std::vector<uint8_t> CompressCM(const std::vector<uint8_t>& in);
std::vector<uint8_t> DecompressCM(const std::vector<uint8_t>& in);
