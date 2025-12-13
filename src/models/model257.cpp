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
  cum_valid = false;
}

void Model257::InitUniform256() {
  cnt.fill(0);
  for (int i = 0; i < 256; ++i)
    cnt[i] = 1;
  cnt[256] = 1;
  total = 257;
  cum_valid = false;
}

uint16_t Model257::Get(int sym) const { return cnt[sym]; }

void Model257::Bump(int sym) {
  cnt[sym] += 1;
  total += 1;
  cum_valid = false;
  if (total >= 1u << 15) {
    Rescale(cnt, total);
    cum_valid = false;
  }
}

void Model257::UpdateCum() {
  if (cum_valid)
    return;
  uint32_t c = 0;
  for (int i = 0; i < 257; ++i) {
    cum[i] = c;
    c += cnt[i];
  }
  cum_valid = true;
}

void Model257::Cum(int sym, uint32_t &lo, uint32_t &hi) {
  UpdateCum();
  lo = cum[sym];
  hi = cum[sym] + cnt[sym];
}

int Model257::FindByFreq(uint32_t f) {
  UpdateCum();
  int lo = 0, hi = 256;
  while (lo < hi) {
    int mid = (lo + hi + 1) >> 1;
    if (cum[mid] <= f)
      lo = mid;
    else
      hi = mid - 1;
  }
  return lo;
}
