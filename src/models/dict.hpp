#pragma once

#include <cstdint>
#include <vector>

// Dictionary-based compression - prepends common patterns to bootstrap PPM
// This helps small files by providing context that PPM can learn from immediately

// Compress with dictionary prepended (for PPM learning)
std::vector<uint8_t> DictEncode(const std::vector<uint8_t> &in);
std::vector<uint8_t> DictDecode(const std::vector<uint8_t> &in);

// Get the static dictionary
const std::vector<uint8_t>& GetStaticDict();
