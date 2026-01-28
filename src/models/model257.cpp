#include "model257.hpp"

void Rescale(std::array<uint16_t, 257> &cnt, uint32_t &total, uint16_t &unique) {
  uint32_t t = 0;
  uint16_t u = 0;
  for (int i = 0; i < 257; ++i) {
    uint16_t v = cnt[i];
    v = (uint16_t)((v + 1) >> 1);
    if (v == 0 && i == 256)
      v = 1;
    cnt[i] = v;
    t += v;
    if (i < 256 && v > 0)
      u++;
  }
  total = t;
  unique = u;
  if (total == 0) {
    cnt[256] = 1;
    total = 1;
  }
}

void Model257::InitEscOnly() {
  cnt.fill(0);
  cnt[256] = 1;
  total = 1;
  unique_count = 0;
  FenwickBuild();
}

void Model257::InitUniform256() {
  cnt.fill(0);
  for (int i = 0; i < 256; ++i)
    cnt[i] = 1;
  cnt[256] = 1;
  total = 257;
  unique_count = 256;
  FenwickBuild();
}

uint16_t Model257::Get(int sym) const { return cnt[sym]; }

void Model257::Bump(int sym) {
  if (sym < 256 && cnt[sym] == 0) {
    unique_count++;
  }
  cnt[sym] += 1;
  total += 1;
  FenwickAdd(sym, 1);

  if (total >= (1u << 14)) { // Lower threshold for better precision
    Rescale(cnt, total, unique_count);
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

// Witten-Bell escape estimation:
// escape_prob = unique_count / (symbol_total + unique_count)

uint32_t Model257::GetWBTotal() const {
  uint32_t esc = unique_count > 0 ? unique_count : 1;
  return (total - cnt[256]) + esc;
}

void Model257::CumWB(int sym, uint32_t &lo, uint32_t &hi, uint32_t &tot) {
  uint32_t esc = unique_count > 0 ? unique_count : 1;
  tot = (total - cnt[256]) + esc;

  if (sym == 256) {
    lo = total - cnt[256];
    hi = tot;
  } else {
    const uint32_t pref = FenwickPrefix(sym);
    hi = pref;
    lo = (sym <= 0) ? 0 : FenwickPrefix(sym - 1);
  }
}

int Model257::FindByFreqWB(uint32_t f) {
  uint32_t symbol_total = total - cnt[256];
  if (f >= symbol_total)
    return 256;

  int idx = 0;
  uint32_t bitmask = 1;
  while ((bitmask << 1) <= 257)
    bitmask <<= 1;

  while (bitmask) {
    int next = idx + (int)bitmask;
    if (next <= 256 && bit[next] <= f) {
      idx = next;
      f -= bit[next];
    }
    bitmask >>= 1;
  }
  return (idx > 255) ? 255 : idx;
}

// Witten-Bell with exclusion support

void Model257::FillExclusion(std::bitset<256> &excl) const {
  for (int i = 0; i < 256; ++i) {
    if (cnt[i] > 0)
      excl[i] = true;
  }
}

uint32_t Model257::GetWBTotalEx(const std::bitset<256> &excl) const {
  uint32_t sym_total = 0;
  uint16_t unique_ex = 0;
  for (int i = 0; i < 256; ++i) {
    if (!excl[i] && cnt[i] > 0) {
      sym_total += cnt[i];
      unique_ex++;
    }
  }
  uint32_t esc = unique_ex > 0 ? unique_ex : 1;
  return sym_total + esc;
}

void Model257::CumWBEx(int sym, const std::bitset<256> &excl, uint32_t &lo,
                       uint32_t &hi, uint32_t &tot) {
  uint32_t sym_total = 0;
  uint16_t unique_ex = 0;
  for (int i = 0; i < 256; ++i) {
    if (!excl[i] && cnt[i] > 0) {
      sym_total += cnt[i];
      unique_ex++;
    }
  }
  uint32_t esc = unique_ex > 0 ? unique_ex : 1;
  tot = sym_total + esc;

  if (sym == 256) {
    lo = sym_total;
    hi = tot;
  } else {
    uint32_t c = 0;
    for (int i = 0; i < sym; ++i) {
      if (!excl[i])
        c += cnt[i];
    }
    lo = c;
    hi = c + cnt[sym];
  }
}

int Model257::FindByFreqWBEx(uint32_t f, const std::bitset<256> &excl) {
  uint32_t sym_total = 0;
  for (int i = 0; i < 256; ++i) {
    if (!excl[i] && cnt[i] > 0)
      sym_total += cnt[i];
  }
  if (f >= sym_total)
    return 256;

  uint32_t c = 0;
  for (int i = 0; i < 256; ++i) {
    if (excl[i])
      continue;
    uint32_t next = c + cnt[i];
    if (f < next)
      return i;
    c = next;
  }
  return 256;
}
