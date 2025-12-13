#include "model257.hpp"

void Rescale(std::array<uint16_t, 257> &cnt, uint32_t &total) {
  uint32_t t = 0;
  for (int i = 0; i < 257; ++i) {
    uint16_t v = cnt[i];
    v = (uint16_t)((v + 1) >> 1);
    if (v == 0 && i == 256)
      v = 1;
    cnt[i] = v;
    t += v;
  }
  total = t;
  if (total == 0) {
    cnt[256] = 1;
    total = 1;
  }
}

void Model257::InitEscOnly() {
  cnt.fill(0);
  cnt[256] = 1;
  total = 1;
  FenwickBuild();
}

void Model257::InitUniform256() {
  cnt.fill(0);
  for (int i = 0; i < 256; ++i)
    cnt[i] = 1;
  cnt[256] = 1;
  total = 257;
  FenwickBuild();
}

uint16_t Model257::Get(int sym) const { return cnt[sym]; }

void Model257::Bump(int sym) {
  cnt[sym] += 1;
  total += 1;
  FenwickAdd(sym, 1);

  if (total >= (1u << 15)) {
    Rescale(cnt, total);
    FenwickBuild();
  }
}

void Model257::Cum(int sym, uint32_t &lo, uint32_t &hi) {
  const uint32_t pref = FenwickPrefix(sym);
  hi = pref;
  if (sym <= 0)
    lo = 0;
  else
    lo = FenwickPrefix(sym - 1);
}

int Model257::FindByFreq(uint32_t f) {
  int idx = 0;
  uint32_t bitmask = 1;
  while ((bitmask << 1) <= 257)
    bitmask <<= 1;

  while (bitmask) {
    int next = idx + (int)bitmask;
    if (next <= 257 && bit[next] <= f) {
      idx = next;
      f -= bit[next];
    }
    bitmask >>= 1;
  }

  if (idx > 256)
    return 256;
  return idx;
}

void Model257::FenwickBuild() {
  bit.fill(0);
  for (int sym = 0; sym < 257; ++sym)
    FenwickAdd(sym, cnt[sym]);
}

void Model257::FenwickAdd(int sym, uint32_t delta) {
  int i = sym + 1;
  while (i <= 257) {
    bit[i] += delta;
    i += i & -i;
  }
}

uint32_t Model257::FenwickPrefix(int sym) const {
  if (sym < 0)
    return 0;
  uint32_t s = 0;
  int i = sym + 1;
  while (i > 0) {
    s += bit[i];
    i -= i & -i;
  }
  return s;
}
