#pragma once

#include <cstdint>
#include <vector>

struct OutBuf {
  std::vector<uint8_t> data;
  void Put(uint8_t b) { data.push_back(b); }
};

struct InBuf {
  const uint8_t *p{};
  const uint8_t *e{};
  uint8_t Get() { return (p < e) ? *p++ : 0; }
};
