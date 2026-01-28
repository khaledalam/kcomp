#pragma once

#include <array>
#include <bitset>
#include <cstdint>

void Rescale(std::array<uint16_t, 257> &cnt, uint32_t &total, uint16_t &unique);

struct Model257 {
  std::array<uint16_t, 257> cnt{};
  uint32_t total = 0;
  uint16_t unique_count = 0; // Witten-Bell: track unique symbols
  std::array<uint32_t, 258> bit{};

  void InitEscOnly();
  void InitUniform256();

  uint16_t Get(int sym) const;
  void Bump(int sym);

  // Original methods
  void Cum(int sym, uint32_t &lo, uint32_t &hi);
  int FindByFreq(uint32_t f);

  // Witten-Bell methods
  uint32_t GetWBTotal() const;
  void CumWB(int sym, uint32_t &lo, uint32_t &hi, uint32_t &tot);
  int FindByFreqWB(uint32_t f);

  // Witten-Bell with exclusion
  uint32_t GetWBTotalEx(const std::bitset<256> &excl) const;
  void CumWBEx(int sym, const std::bitset<256> &excl, uint32_t &lo,
               uint32_t &hi, uint32_t &tot);
  int FindByFreqWBEx(uint32_t f, const std::bitset<256> &excl);
  void FillExclusion(std::bitset<256> &excl) const;

private:
  void FenwickBuild();
  void FenwickAdd(int sym, uint32_t delta);
  uint32_t FenwickPrefix(int sym) const;
};
