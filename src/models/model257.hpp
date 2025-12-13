#pragma once

#include <array>
#include <cstdint>

void Rescale(std::array<uint16_t, 257> &cnt, uint32_t &total);

struct Model257 {
  std::array<uint16_t, 257> cnt{};
  uint32_t total = 0;

  void InitEscOnly();
  void InitUniform256();
  uint16_t Get(int sym) const;
  void Bump(int sym);
  void Cum(int sym, uint32_t &lo, uint32_t &hi) const;
  int FindByFreq(uint32_t f) const;
};
