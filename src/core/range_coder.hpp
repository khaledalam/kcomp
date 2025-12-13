#pragma once

#include "../io/buffer.hpp"
#include <cstdint>

struct RangeEnc {
  OutBuf *out{};
  uint32_t low = 0;
  uint32_t high = 0xFFFFFFFFu;

  void Init(OutBuf &o);
  void Encode(uint32_t cum_low, uint32_t cum_high, uint32_t total);
  void Finish();
};

struct RangeDec {
  InBuf *in{};
  uint32_t low = 0;
  uint32_t high = 0xFFFFFFFFu;
  uint32_t code = 0;

  void Init(InBuf &ib);
  uint32_t GetFreq(uint32_t total);
  void Decode(uint32_t cum_low, uint32_t cum_high, uint32_t total);
};
