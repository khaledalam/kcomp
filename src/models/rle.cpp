#include "rle.hpp"
#include <stdexcept>

std::vector<uint8_t> CompressRLE(const std::vector<uint8_t> &in) {
  std::vector<uint8_t> out;
  out.reserve(in.size());
  size_t i = 0;
  while (i < in.size()) {
    uint8_t v = in[i];
    size_t j = i + 1;
    while (j < in.size() && in[j] == v && (j - i) < 255)
      ++j;
    uint8_t run = (uint8_t)(j - i);
    out.push_back(run);
    out.push_back(v);
    i = j;
  }
  return out;
}

std::vector<uint8_t> DecompressRLE(const std::vector<uint8_t> &in) {
  if (in.size() % 2 != 0)
    throw std::runtime_error("bad rle");
  std::vector<uint8_t> out;
  for (size_t i = 0; i < in.size(); i += 2) {
    uint8_t run = in[i];
    uint8_t v = in[i + 1];
    out.insert(out.end(), run, v);
  }
  return out;
}
