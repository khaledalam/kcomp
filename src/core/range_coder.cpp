#include "range_coder.hpp"

void RangeEnc::Init(OutBuf &o) {
  out = &o;
  low = 0;
  high = 0xFFFFFFFFu;
}

void RangeEnc::Encode(uint32_t cum_low, uint32_t cum_high, uint32_t total) {
  uint64_t range = (uint64_t)high - low + 1;
  high = low + (uint32_t)((range * cum_high) / total - 1);
  low = low + (uint32_t)((range * cum_low) / total);

  while ((low ^ high) < (1u << 24)) {
    out->Put((uint8_t)(high >> 24));
    low <<= 8;
    high = (high << 8) | 0xFFu;
  }
}

void RangeEnc::Finish() {
  for (int i = 0; i < 4; ++i) {
    out->Put((uint8_t)(low >> 24));
    low <<= 8;
  }
}

void RangeDec::Init(InBuf &ib) {
  in = &ib;
  low = 0;
  high = 0xFFFFFFFFu;
  code = 0;
  for (int i = 0; i < 4; ++i) {
    code = (code << 8) | in->Get();
  }
}

uint32_t RangeDec::GetFreq(uint32_t total) {
  uint64_t range = (uint64_t)high - low + 1;
  uint64_t off = (uint64_t)code - low;
  return (uint32_t)(((off + 1) * total - 1) / range);
}

void RangeDec::Decode(uint32_t cum_low, uint32_t cum_high, uint32_t total) {
  uint64_t range = (uint64_t)high - low + 1;
  high = low + (uint32_t)((range * cum_high) / total - 1);
  low = low + (uint32_t)((range * cum_low) / total);

  while ((low ^ high) < (1u << 24)) {
    low <<= 8;
    high = (high << 8) | 0xFFu;
    code = (code << 8) | in->Get();
  }
}
