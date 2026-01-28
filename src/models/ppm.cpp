#include "ppm.hpp"
#include "../core/range_coder.hpp"
#include "../io/buffer.hpp"
#include "lz77.hpp"
#include "lzopt.hpp"
#include "lzx.hpp"
#include "bwt.hpp"
#include "cm.hpp"
#include "dict.hpp"
#include "lzma.hpp"
#include "model257.hpp"
#include <array>
#include <unordered_map>
#include <bitset>

std::vector<uint8_t> CompressPPM1(const std::vector<uint8_t> &in) {
  std::array<Model257, 256> ctx{};
  for (auto &m : ctx)
    m.InitEscOnly();

  Model257 order0;
  order0.InitUniform256();

  OutBuf out;
  RangeEnc enc;
  enc.Init(out);

  uint8_t prev = 0;

  for (uint8_t b : in) {
    Model257 &m = ctx[prev];

    if (m.Get(b) != 0) {
      uint32_t lo, hi;
      m.Cum(b, lo, hi);
      enc.Encode(lo, hi, m.total);
    } else {
      uint32_t lo, hi;
      m.Cum(256, lo, hi);
      enc.Encode(lo, hi, m.total);

      uint32_t lo2, hi2;
      order0.Cum(b, lo2, hi2);
      enc.Encode(lo2, hi2, order0.total);
    }

    m.Bump(b);
    order0.Bump(b);
    prev = b;
  }

  {
    Model257 &m = ctx[prev];
    uint32_t lo, hi;
    m.Cum(256, lo, hi);
    enc.Encode(lo, hi, m.total);

    uint32_t lo2, hi2;
    order0.Cum(256, lo2, hi2);
    enc.Encode(lo2, hi2, order0.total);
  }

  enc.Finish();
  return out.data;
}

std::vector<uint8_t> DecompressPPM1(const std::vector<uint8_t> &in) {
  std::array<Model257, 256> ctx{};
  for (auto &m : ctx)
    m.InitEscOnly();

  Model257 order0;
  order0.InitUniform256();

  InBuf ib{in.data(), in.data() + in.size()};
  RangeDec dec;
  dec.Init(ib);

  std::vector<uint8_t> out;
  out.reserve(in.size() * 3);

  uint8_t prev = 0;

  while (true) {
    Model257 &m = ctx[prev];
    uint32_t f = dec.GetFreq(m.total);
    int sym = m.FindByFreq(f);

    uint32_t lo, hi;
    m.Cum(sym, lo, hi);
    dec.Decode(lo, hi, m.total);

    if (sym == 256) {
      uint32_t f0 = dec.GetFreq(order0.total);
      int s0 = order0.FindByFreq(f0);

      uint32_t lo0, hi0;
      order0.Cum(s0, lo0, hi0);
      dec.Decode(lo0, hi0, order0.total);

      if (s0 == 256)
        break;

      uint8_t b = (uint8_t)s0;
      out.push_back(b);
      m.Bump(b);
      order0.Bump(b);
      prev = b;
    } else {
      uint8_t b = (uint8_t)sym;
      out.push_back(b);
      m.Bump(b);
      order0.Bump(b);
      prev = b;
    }
  }

  return out;
}

std::vector<uint8_t> CompressPPM2(const std::vector<uint8_t> &in) {
  std::vector<Model257> ctx2(256 * 256);
  std::vector<Model257> ctx1(256);

  for (auto &m : ctx2)
    m.InitEscOnly();
  for (auto &m : ctx1)
    m.InitEscOnly();

  Model257 order0;
  order0.InitUniform256();

  OutBuf out;
  RangeEnc enc;
  enc.Init(out);

  uint8_t prev1 = 0;
  uint8_t prev2 = 0;

  for (uint8_t b : in) {
    std::bitset<256> excl;
    bool encoded = false;

    uint32_t idx2 = ((uint32_t)prev2 << 8) | prev1;
    Model257 &m2 = ctx2[idx2];
    if (m2.Get(b) != 0) {
      uint32_t lo, hi, tot;
      m2.CumWB(b, lo, hi, tot);
      enc.Encode(lo, hi, tot);
      encoded = true;
    } else {
      m2.FillExclusion(excl);
      uint32_t lo, hi, tot;
      m2.CumWB(256, lo, hi, tot);
      enc.Encode(lo, hi, tot);
    }

    if (!encoded) {
      Model257 &m1 = ctx1[prev1];
      if (m1.Get(b) != 0 && !excl[b]) {
        uint32_t lo, hi, tot;
        m1.CumWBEx(b, excl, lo, hi, tot);
        enc.Encode(lo, hi, tot);
        encoded = true;
      } else {
        uint32_t lo, hi, tot;
        m1.CumWBEx(256, excl, lo, hi, tot);
        enc.Encode(lo, hi, tot);
        // Fill exclusion AFTER encoding for next order
        m1.FillExclusion(excl);
      }
    }

    if (!encoded) {
      uint32_t lo, hi;
      order0.Cum(b, lo, hi);
      enc.Encode(lo, hi, order0.total);
    }

    m2.Bump(b);
    ctx1[prev1].Bump(b);
    order0.Bump(b);

    prev2 = prev1;
    prev1 = b;
  }

  {
    std::bitset<256> excl;
    uint32_t idx2 = ((uint32_t)prev2 << 8) | prev1;
    Model257 &m2 = ctx2[idx2];
    uint32_t lo2, hi2, tot2;
    m2.CumWB(256, lo2, hi2, tot2);
    enc.Encode(lo2, hi2, tot2);
    m2.FillExclusion(excl);

    Model257 &m1 = ctx1[prev1];
    uint32_t lo1, hi1, tot1;
    m1.CumWBEx(256, excl, lo1, hi1, tot1);
    enc.Encode(lo1, hi1, tot1);
    m1.FillExclusion(excl);

    uint32_t lo0, hi0;
    order0.Cum(256, lo0, hi0);
    enc.Encode(lo0, hi0, order0.total);
  }

  enc.Finish();
  return out.data;
}

std::vector<uint8_t> DecompressPPM2(const std::vector<uint8_t> &in) {
  std::vector<Model257> ctx2(256 * 256);
  std::vector<Model257> ctx1(256);

  for (auto &m : ctx2)
    m.InitEscOnly();
  for (auto &m : ctx1)
    m.InitEscOnly();

  Model257 order0;
  order0.InitUniform256();

  InBuf ib{in.data(), in.data() + in.size()};
  RangeDec dec;
  dec.Init(ib);

  std::vector<uint8_t> out;
  out.reserve(in.size() * 3);

  uint8_t prev1 = 0;
  uint8_t prev2 = 0;

  while (true) {
    std::bitset<256> excl;
    int sym = 256;
    bool decoded = false;

    uint32_t idx2 = ((uint32_t)prev2 << 8) | prev1;
    Model257 &m2 = ctx2[idx2];
    {
      uint32_t tot = m2.GetWBTotal();
      uint32_t f = dec.GetFreq(tot);
      sym = m2.FindByFreqWB(f);
      uint32_t lo, hi, t;
      m2.CumWB(sym, lo, hi, t);
      dec.Decode(lo, hi, t);

      if (sym != 256) {
        decoded = true;
      } else {
        m2.FillExclusion(excl);
      }
    }

    if (!decoded) {
      Model257 &m1 = ctx1[prev1];
      uint32_t tot = m1.GetWBTotalEx(excl);
      uint32_t f = dec.GetFreq(tot);
      sym = m1.FindByFreqWBEx(f, excl);
      uint32_t lo, hi, t;
      m1.CumWBEx(sym, excl, lo, hi, t);
      dec.Decode(lo, hi, t);

      if (sym != 256) {
        decoded = true;
      } else {
        m1.FillExclusion(excl);
      }
    }

    if (!decoded) {
      uint32_t f = dec.GetFreq(order0.total);
      sym = order0.FindByFreq(f);
      uint32_t lo, hi;
      order0.Cum(sym, lo, hi);
      dec.Decode(lo, hi, order0.total);

      if (sym == 256)
        break;
    }

    uint8_t b = (uint8_t)sym;
    out.push_back(b);

    m2.Bump(b);
    ctx1[prev1].Bump(b);
    order0.Bump(b);

    prev2 = prev1;
    prev1 = b;
  }

  return out;
}

// PPM3: Order-3 with sparse context and Witten-Bell exclusion
std::vector<uint8_t> CompressPPM3(const std::vector<uint8_t> &in) {
  std::unordered_map<uint32_t, Model257> ctx3;  // Sparse order-3
  std::vector<Model257> ctx2(256 * 256);
  std::vector<Model257> ctx1(256);

  for (auto &m : ctx2)
    m.InitEscOnly();
  for (auto &m : ctx1)
    m.InitEscOnly();

  Model257 order0;
  order0.InitUniform256();

  OutBuf out;
  RangeEnc enc;
  enc.Init(out);

  uint32_t h = 0;  // Rolling hash for context

  for (uint8_t b : in) {
    std::bitset<256> excl;
    bool encoded = false;

    // Order-3
    uint32_t h3 = h & 0xFFFFFF;
    auto it3 = ctx3.find(h3);
    if (it3 != ctx3.end() && it3->second.Get(b) != 0) {
      uint32_t lo, hi, tot;
      it3->second.CumWB(b, lo, hi, tot);
      enc.Encode(lo, hi, tot);
      encoded = true;
    } else if (it3 != ctx3.end()) {
      uint32_t lo, hi, tot;
      it3->second.CumWB(256, lo, hi, tot);
      enc.Encode(lo, hi, tot);
      it3->second.FillExclusion(excl);
    }

    // Order-2
    if (!encoded) {
      uint16_t h2 = h & 0xFFFF;
      Model257 &m2 = ctx2[h2];
      if (m2.Get(b) != 0 && !excl[b]) {
        uint32_t lo, hi, tot;
        m2.CumWBEx(b, excl, lo, hi, tot);
        enc.Encode(lo, hi, tot);
        encoded = true;
      } else {
        uint32_t lo, hi, tot;
        m2.CumWBEx(256, excl, lo, hi, tot);
        enc.Encode(lo, hi, tot);
        m2.FillExclusion(excl);
      }
    }

    // Order-1
    if (!encoded) {
      uint8_t h1 = h & 0xFF;
      Model257 &m1 = ctx1[h1];
      if (m1.Get(b) != 0 && !excl[b]) {
        uint32_t lo, hi, tot;
        m1.CumWBEx(b, excl, lo, hi, tot);
        enc.Encode(lo, hi, tot);
        encoded = true;
      } else {
        uint32_t lo, hi, tot;
        m1.CumWBEx(256, excl, lo, hi, tot);
        enc.Encode(lo, hi, tot);
        m1.FillExclusion(excl);
      }
    }

    // Order-0
    if (!encoded) {
      uint32_t lo, hi;
      order0.Cum(b, lo, hi);
      enc.Encode(lo, hi, order0.total);
    }

    // Update all contexts
    ctx3[h3].Bump(b);
    ctx2[h & 0xFFFF].Bump(b);
    ctx1[h & 0xFF].Bump(b);
    order0.Bump(b);

    h = (h << 8) | b;
  }

  // Encode EOF
  {
    std::bitset<256> excl;
    uint32_t h3 = h & 0xFFFFFF;
    auto it3 = ctx3.find(h3);
    if (it3 != ctx3.end()) {
      uint32_t lo, hi, tot;
      it3->second.CumWB(256, lo, hi, tot);
      enc.Encode(lo, hi, tot);
      it3->second.FillExclusion(excl);
    }

    uint16_t h2 = h & 0xFFFF;
    Model257 &m2 = ctx2[h2];
    uint32_t lo2, hi2, tot2;
    m2.CumWBEx(256, excl, lo2, hi2, tot2);
    enc.Encode(lo2, hi2, tot2);
    m2.FillExclusion(excl);

    uint8_t h1 = h & 0xFF;
    Model257 &m1 = ctx1[h1];
    uint32_t lo1, hi1, tot1;
    m1.CumWBEx(256, excl, lo1, hi1, tot1);
    enc.Encode(lo1, hi1, tot1);
    m1.FillExclusion(excl);

    uint32_t lo0, hi0;
    order0.Cum(256, lo0, hi0);
    enc.Encode(lo0, hi0, order0.total);
  }

  enc.Finish();
  return out.data;
}

std::vector<uint8_t> DecompressPPM3(const std::vector<uint8_t> &in) {
  std::unordered_map<uint32_t, Model257> ctx3;
  std::vector<Model257> ctx2(256 * 256);
  std::vector<Model257> ctx1(256);

  for (auto &m : ctx2)
    m.InitEscOnly();
  for (auto &m : ctx1)
    m.InitEscOnly();

  Model257 order0;
  order0.InitUniform256();

  InBuf ib{in.data(), in.data() + in.size()};
  RangeDec dec;
  dec.Init(ib);

  std::vector<uint8_t> out;
  out.reserve(in.size() * 3);

  uint32_t h = 0;

  while (true) {
    std::bitset<256> excl;
    int sym = 256;
    bool decoded = false;

    // Order-3
    uint32_t h3 = h & 0xFFFFFF;
    auto it3 = ctx3.find(h3);
    if (it3 != ctx3.end()) {
      uint32_t tot = it3->second.GetWBTotal();
      uint32_t f = dec.GetFreq(tot);
      sym = it3->second.FindByFreqWB(f);
      uint32_t lo, hi, t;
      it3->second.CumWB(sym, lo, hi, t);
      dec.Decode(lo, hi, t);

      if (sym != 256) {
        decoded = true;
      } else {
        it3->second.FillExclusion(excl);
      }
    }

    // Order-2
    if (!decoded) {
      uint16_t h2 = h & 0xFFFF;
      Model257 &m2 = ctx2[h2];
      uint32_t tot = m2.GetWBTotalEx(excl);
      uint32_t f = dec.GetFreq(tot);
      sym = m2.FindByFreqWBEx(f, excl);
      uint32_t lo, hi, t;
      m2.CumWBEx(sym, excl, lo, hi, t);
      dec.Decode(lo, hi, t);

      if (sym != 256) {
        decoded = true;
      } else {
        m2.FillExclusion(excl);
      }
    }

    // Order-1
    if (!decoded) {
      uint8_t h1 = h & 0xFF;
      Model257 &m1 = ctx1[h1];
      uint32_t tot = m1.GetWBTotalEx(excl);
      uint32_t f = dec.GetFreq(tot);
      sym = m1.FindByFreqWBEx(f, excl);
      uint32_t lo, hi, t;
      m1.CumWBEx(sym, excl, lo, hi, t);
      dec.Decode(lo, hi, t);

      if (sym != 256) {
        decoded = true;
      } else {
        m1.FillExclusion(excl);
      }
    }

    // Order-0
    if (!decoded) {
      uint32_t f = dec.GetFreq(order0.total);
      sym = order0.FindByFreq(f);
      uint32_t lo, hi;
      order0.Cum(sym, lo, hi);
      dec.Decode(lo, hi, order0.total);

      if (sym == 256)
        break;
    }

    uint8_t b = (uint8_t)sym;
    out.push_back(b);

    ctx3[h3].Bump(b);
    ctx2[h & 0xFFFF].Bump(b);
    ctx1[h & 0xFF].Bump(b);
    order0.Bump(b);

    h = (h << 8) | b;
  }

  return out;
}

struct ModelEx {
  std::array<uint16_t, 257> cnt{};
  uint32_t total = 0;

  void InitEscOnly() {
    cnt.fill(0);
    cnt[256] = 1;
    total = 1;
  }

  uint16_t Get(int sym) const { return cnt[sym]; }

  void Bump(int sym) {
    cnt[sym] += 1;
    total += 1;
    if (total >= 1u << 15) {
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
  }

  void CumEx(int sym, const std::bitset<256> &excl, uint32_t &lo,
             uint32_t &hi, uint32_t &tot) const {
    uint32_t c = 0;
    uint32_t t = 0;
    for (int i = 0; i < 257; ++i) {
      if (i < 256 && excl[i])
        continue;
      if (i == sym)
        lo = c;
      c += cnt[i];
      if (i == sym)
        hi = c;
      t += cnt[i];
    }
    tot = t;
  }

  int FindByFreqEx(uint32_t f, const std::bitset<256> &excl) const {
    uint32_t c = 0;
    for (int i = 0; i < 257; ++i) {
      if (i < 256 && excl[i])
        continue;
      uint32_t n = c + cnt[i];
      if (f < n)
        return i;
      c = n;
    }
    return 256;
  }
};

std::vector<uint8_t> CompressPPM4(const std::vector<uint8_t> &in) {
  std::unordered_map<uint32_t, ModelEx> ctx4;
  std::unordered_map<uint32_t, ModelEx> ctx3;
  std::unordered_map<uint16_t, ModelEx> ctx2;
  std::array<ModelEx, 256> ctx1{};

  for (auto &m : ctx1)
    m.InitEscOnly();

  Model257 order0;
  order0.InitUniform256();

  OutBuf out;
  RangeEnc enc;
  enc.Init(out);

  uint32_t h = 0;

  for (uint8_t b : in) {
    std::bitset<256> excl;
    bool encoded = false;

    auto it4 = ctx4.find(h);
    if (it4 != ctx4.end() && it4->second.Get(b) != 0) {
      uint32_t lo, hi, tot;
      it4->second.CumEx(b, excl, lo, hi, tot);
      enc.Encode(lo, hi, tot);
      encoded = true;
    } else if (it4 != ctx4.end()) {
      for (int i = 0; i < 256; ++i)
        if (it4->second.Get(i) != 0)
          excl[i] = true;
      uint32_t lo, hi, tot;
      it4->second.CumEx(256, excl, lo, hi, tot);
      enc.Encode(lo, hi, tot);
    }

    if (!encoded) {
      uint32_t h3 = h & 0xFFFFFF;
      auto it3 = ctx3.find(h3);
      if (it3 != ctx3.end() && it3->second.Get(b) != 0 && !excl[b]) {
        uint32_t lo, hi, tot;
        it3->second.CumEx(b, excl, lo, hi, tot);
        enc.Encode(lo, hi, tot);
        encoded = true;
      } else if (it3 != ctx3.end()) {
        for (int i = 0; i < 256; ++i)
          if (it3->second.Get(i) != 0)
            excl[i] = true;
        uint32_t lo, hi, tot;
        it3->second.CumEx(256, excl, lo, hi, tot);
        enc.Encode(lo, hi, tot);
      }
    }

    if (!encoded) {
      uint16_t h2 = h & 0xFFFF;
      auto it2 = ctx2.find(h2);
      if (it2 != ctx2.end() && it2->second.Get(b) != 0 && !excl[b]) {
        uint32_t lo, hi, tot;
        it2->second.CumEx(b, excl, lo, hi, tot);
        enc.Encode(lo, hi, tot);
        encoded = true;
      } else if (it2 != ctx2.end()) {
        for (int i = 0; i < 256; ++i)
          if (it2->second.Get(i) != 0)
            excl[i] = true;
        uint32_t lo, hi, tot;
        it2->second.CumEx(256, excl, lo, hi, tot);
        enc.Encode(lo, hi, tot);
      }
    }

    if (!encoded) {
      ModelEx &m1 = ctx1[h & 0xFF];
      if (m1.Get(b) != 0 && !excl[b]) {
        uint32_t lo, hi, tot;
        m1.CumEx(b, excl, lo, hi, tot);
        enc.Encode(lo, hi, tot);
        encoded = true;
      } else {
        for (int i = 0; i < 256; ++i)
          if (m1.Get(i) != 0)
            excl[i] = true;
        uint32_t lo, hi, tot;
        m1.CumEx(256, excl, lo, hi, tot);
        enc.Encode(lo, hi, tot);
      }
    }

    if (!encoded) {
      uint32_t lo, hi;
      order0.Cum(b, lo, hi);
      enc.Encode(lo, hi, order0.total);
    }

    ctx4[h].Bump(b);
    ctx3[h & 0xFFFFFF].Bump(b);
    ctx2[h & 0xFFFF].Bump(b);
    ctx1[h & 0xFF].Bump(b);
    order0.Bump(b);

    h = (h << 8) | b;
  }

  {
    std::bitset<256> excl;
    auto it4 = ctx4.find(h);
    if (it4 != ctx4.end()) {
      for (int i = 0; i < 256; ++i)
        if (it4->second.Get(i) != 0)
          excl[i] = true;
      uint32_t lo, hi, tot;
      it4->second.CumEx(256, excl, lo, hi, tot);
      enc.Encode(lo, hi, tot);
    }

    uint32_t h3 = h & 0xFFFFFF;
    auto it3 = ctx3.find(h3);
    if (it3 != ctx3.end()) {
      for (int i = 0; i < 256; ++i)
        if (it3->second.Get(i) != 0)
          excl[i] = true;
      uint32_t lo, hi, tot;
      it3->second.CumEx(256, excl, lo, hi, tot);
      enc.Encode(lo, hi, tot);
    }

    uint16_t h2 = h & 0xFFFF;
    auto it2 = ctx2.find(h2);
    if (it2 != ctx2.end()) {
      for (int i = 0; i < 256; ++i)
        if (it2->second.Get(i) != 0)
          excl[i] = true;
      uint32_t lo, hi, tot;
      it2->second.CumEx(256, excl, lo, hi, tot);
      enc.Encode(lo, hi, tot);
    }

    ModelEx &m1 = ctx1[h & 0xFF];
    for (int i = 0; i < 256; ++i)
      if (m1.Get(i) != 0)
        excl[i] = true;
    uint32_t lo, hi, tot;
    m1.CumEx(256, excl, lo, hi, tot);
    enc.Encode(lo, hi, tot);

    uint32_t lo0, hi0;
    order0.Cum(256, lo0, hi0);
    enc.Encode(lo0, hi0, order0.total);
  }

  enc.Finish();
  return out.data;
}

std::vector<uint8_t> DecompressPPM4(const std::vector<uint8_t> &in) {
  std::unordered_map<uint32_t, ModelEx> ctx4;
  std::unordered_map<uint32_t, ModelEx> ctx3;
  std::unordered_map<uint16_t, ModelEx> ctx2;
  std::array<ModelEx, 256> ctx1{};

  for (auto &m : ctx1)
    m.InitEscOnly();

  Model257 order0;
  order0.InitUniform256();

  InBuf ib{in.data(), in.data() + in.size()};
  RangeDec dec;
  dec.Init(ib);

  std::vector<uint8_t> out;
  out.reserve(in.size() * 3);

  uint32_t h = 0;

  while (true) {
    std::bitset<256> excl;
    int sym = 256;
    bool decoded = false;

    auto it4 = ctx4.find(h);
    if (it4 != ctx4.end()) {
      uint32_t tot = 0;
      for (int i = 0; i < 257; ++i) {
        if (i < 256 && excl[i])
          continue;
        tot += it4->second.cnt[i];
      }
      uint32_t f = dec.GetFreq(tot);
      sym = it4->second.FindByFreqEx(f, excl);
      uint32_t lo, hi, t;
      it4->second.CumEx(sym, excl, lo, hi, t);
      dec.Decode(lo, hi, t);

      if (sym != 256) {
        decoded = true;
      } else {
        for (int i = 0; i < 256; ++i)
          if (it4->second.Get(i) != 0)
            excl[i] = true;
      }
    }

    if (!decoded) {
      uint32_t h3 = h & 0xFFFFFF;
      auto it3 = ctx3.find(h3);
      if (it3 != ctx3.end()) {
        uint32_t tot = 0;
        for (int i = 0; i < 257; ++i) {
          if (i < 256 && excl[i])
            continue;
          tot += it3->second.cnt[i];
        }
        uint32_t f = dec.GetFreq(tot);
        sym = it3->second.FindByFreqEx(f, excl);
        uint32_t lo, hi, t;
        it3->second.CumEx(sym, excl, lo, hi, t);
        dec.Decode(lo, hi, t);

        if (sym != 256) {
          decoded = true;
        } else {
          for (int i = 0; i < 256; ++i)
            if (it3->second.Get(i) != 0)
              excl[i] = true;
        }
      }
    }

    if (!decoded) {
      uint16_t h2 = h & 0xFFFF;
      auto it2 = ctx2.find(h2);
      if (it2 != ctx2.end()) {
        uint32_t tot = 0;
        for (int i = 0; i < 257; ++i) {
          if (i < 256 && excl[i])
            continue;
          tot += it2->second.cnt[i];
        }
        uint32_t f = dec.GetFreq(tot);
        sym = it2->second.FindByFreqEx(f, excl);
        uint32_t lo, hi, t;
        it2->second.CumEx(sym, excl, lo, hi, t);
        dec.Decode(lo, hi, t);

        if (sym != 256) {
          decoded = true;
        } else {
          for (int i = 0; i < 256; ++i)
            if (it2->second.Get(i) != 0)
              excl[i] = true;
        }
      }
    }

    if (!decoded) {
      ModelEx &m1 = ctx1[h & 0xFF];
      uint32_t tot = 0;
      for (int i = 0; i < 257; ++i) {
        if (i < 256 && excl[i])
          continue;
        tot += m1.cnt[i];
      }
      uint32_t f = dec.GetFreq(tot);
      sym = m1.FindByFreqEx(f, excl);
      uint32_t lo, hi, t;
      m1.CumEx(sym, excl, lo, hi, t);
      dec.Decode(lo, hi, t);

      if (sym != 256) {
        decoded = true;
      } else {
        for (int i = 0; i < 256; ++i)
          if (m1.Get(i) != 0)
            excl[i] = true;
      }
    }

    if (!decoded) {
      uint32_t f = dec.GetFreq(order0.total);
      sym = order0.FindByFreq(f);
      uint32_t lo, hi;
      order0.Cum(sym, lo, hi);
      dec.Decode(lo, hi, order0.total);

      if (sym == 256)
        break;
    }

    uint8_t b = (uint8_t)sym;
    out.push_back(b);

    ctx4[h].Bump(b);
    ctx3[h & 0xFFFFFF].Bump(b);
    ctx2[h & 0xFFFF].Bump(b);
    ctx1[h & 0xFF].Bump(b);
    order0.Bump(b);

    h = (h << 8) | b;
  }

  return out;
}

// PPM5: Order-5 with sparse contexts and Witten-Bell exclusion
std::vector<uint8_t> CompressPPM5(const std::vector<uint8_t> &in) {
  std::unordered_map<uint64_t, Model257> ctx5;
  std::unordered_map<uint32_t, Model257> ctx4;
  std::unordered_map<uint32_t, Model257> ctx3;
  std::vector<Model257> ctx2(256 * 256);
  std::vector<Model257> ctx1(256);

  for (auto &m : ctx2)
    m.InitEscOnly();
  for (auto &m : ctx1)
    m.InitEscOnly();

  Model257 order0;
  order0.InitUniform256();

  OutBuf out;
  RangeEnc enc;
  enc.Init(out);

  uint64_t h = 0;

  for (uint8_t b : in) {
    std::bitset<256> excl;
    bool encoded = false;

    // Order-5
    uint64_t h5 = h & 0xFFFFFFFFFFULL;
    auto it5 = ctx5.find(h5);
    if (it5 != ctx5.end() && it5->second.Get(b) != 0) {
      uint32_t lo, hi, tot;
      it5->second.CumWB(b, lo, hi, tot);
      enc.Encode(lo, hi, tot);
      encoded = true;
    } else if (it5 != ctx5.end()) {
      uint32_t lo, hi, tot;
      it5->second.CumWB(256, lo, hi, tot);
      enc.Encode(lo, hi, tot);
      it5->second.FillExclusion(excl);
    }

    // Order-4
    if (!encoded) {
      uint32_t h4 = h & 0xFFFFFFFF;
      auto it4 = ctx4.find(h4);
      if (it4 != ctx4.end() && it4->second.Get(b) != 0 && !excl[b]) {
        uint32_t lo, hi, tot;
        it4->second.CumWBEx(b, excl, lo, hi, tot);
        enc.Encode(lo, hi, tot);
        encoded = true;
      } else if (it4 != ctx4.end()) {
        uint32_t lo, hi, tot;
        it4->second.CumWBEx(256, excl, lo, hi, tot);
        enc.Encode(lo, hi, tot);
        it4->second.FillExclusion(excl);
      }
    }

    // Order-3
    if (!encoded) {
      uint32_t h3 = h & 0xFFFFFF;
      auto it3 = ctx3.find(h3);
      if (it3 != ctx3.end() && it3->second.Get(b) != 0 && !excl[b]) {
        uint32_t lo, hi, tot;
        it3->second.CumWBEx(b, excl, lo, hi, tot);
        enc.Encode(lo, hi, tot);
        encoded = true;
      } else if (it3 != ctx3.end()) {
        uint32_t lo, hi, tot;
        it3->second.CumWBEx(256, excl, lo, hi, tot);
        enc.Encode(lo, hi, tot);
        it3->second.FillExclusion(excl);
      }
    }

    // Order-2
    if (!encoded) {
      uint16_t h2 = h & 0xFFFF;
      Model257 &m2 = ctx2[h2];
      if (m2.Get(b) != 0 && !excl[b]) {
        uint32_t lo, hi, tot;
        m2.CumWBEx(b, excl, lo, hi, tot);
        enc.Encode(lo, hi, tot);
        encoded = true;
      } else {
        uint32_t lo, hi, tot;
        m2.CumWBEx(256, excl, lo, hi, tot);
        enc.Encode(lo, hi, tot);
        m2.FillExclusion(excl);
      }
    }

    // Order-1
    if (!encoded) {
      uint8_t h1 = h & 0xFF;
      Model257 &m1 = ctx1[h1];
      if (m1.Get(b) != 0 && !excl[b]) {
        uint32_t lo, hi, tot;
        m1.CumWBEx(b, excl, lo, hi, tot);
        enc.Encode(lo, hi, tot);
        encoded = true;
      } else {
        uint32_t lo, hi, tot;
        m1.CumWBEx(256, excl, lo, hi, tot);
        enc.Encode(lo, hi, tot);
        m1.FillExclusion(excl);
      }
    }

    // Order-0
    if (!encoded) {
      uint32_t lo, hi;
      order0.Cum(b, lo, hi);
      enc.Encode(lo, hi, order0.total);
    }

    // Update contexts
    ctx5[h5].Bump(b);
    ctx4[h & 0xFFFFFFFF].Bump(b);
    ctx3[h & 0xFFFFFF].Bump(b);
    ctx2[h & 0xFFFF].Bump(b);
    ctx1[h & 0xFF].Bump(b);
    order0.Bump(b);

    h = (h << 8) | b;
  }

  // Encode EOF
  {
    std::bitset<256> excl;

    uint64_t h5 = h & 0xFFFFFFFFFFULL;
    auto it5 = ctx5.find(h5);
    if (it5 != ctx5.end()) {
      uint32_t lo, hi, tot;
      it5->second.CumWB(256, lo, hi, tot);
      enc.Encode(lo, hi, tot);
      it5->second.FillExclusion(excl);
    }

    uint32_t h4 = h & 0xFFFFFFFF;
    auto it4 = ctx4.find(h4);
    if (it4 != ctx4.end()) {
      uint32_t lo, hi, tot;
      it4->second.CumWBEx(256, excl, lo, hi, tot);
      enc.Encode(lo, hi, tot);
      it4->second.FillExclusion(excl);
    }

    uint32_t h3 = h & 0xFFFFFF;
    auto it3 = ctx3.find(h3);
    if (it3 != ctx3.end()) {
      uint32_t lo, hi, tot;
      it3->second.CumWBEx(256, excl, lo, hi, tot);
      enc.Encode(lo, hi, tot);
      it3->second.FillExclusion(excl);
    }

    uint16_t h2 = h & 0xFFFF;
    Model257 &m2 = ctx2[h2];
    uint32_t lo2, hi2, tot2;
    m2.CumWBEx(256, excl, lo2, hi2, tot2);
    enc.Encode(lo2, hi2, tot2);
    m2.FillExclusion(excl);

    uint8_t h1 = h & 0xFF;
    Model257 &m1 = ctx1[h1];
    uint32_t lo1, hi1, tot1;
    m1.CumWBEx(256, excl, lo1, hi1, tot1);
    enc.Encode(lo1, hi1, tot1);

    uint32_t lo0, hi0;
    order0.Cum(256, lo0, hi0);
    enc.Encode(lo0, hi0, order0.total);
  }

  enc.Finish();
  return out.data;
}

std::vector<uint8_t> DecompressPPM5(const std::vector<uint8_t> &in) {
  std::unordered_map<uint64_t, Model257> ctx5;
  std::unordered_map<uint32_t, Model257> ctx4;
  std::unordered_map<uint32_t, Model257> ctx3;
  std::vector<Model257> ctx2(256 * 256);
  std::vector<Model257> ctx1(256);

  for (auto &m : ctx2)
    m.InitEscOnly();
  for (auto &m : ctx1)
    m.InitEscOnly();

  Model257 order0;
  order0.InitUniform256();

  InBuf ib{in.data(), in.data() + in.size()};
  RangeDec dec;
  dec.Init(ib);

  std::vector<uint8_t> out;
  out.reserve(in.size() * 3);

  uint64_t h = 0;

  while (true) {
    std::bitset<256> excl;
    int sym = 256;
    bool decoded = false;

    // Order-5
    uint64_t h5 = h & 0xFFFFFFFFFFULL;
    auto it5 = ctx5.find(h5);
    if (it5 != ctx5.end()) {
      uint32_t tot = it5->second.GetWBTotal();
      uint32_t f = dec.GetFreq(tot);
      sym = it5->second.FindByFreqWB(f);
      uint32_t lo, hi, t;
      it5->second.CumWB(sym, lo, hi, t);
      dec.Decode(lo, hi, t);

      if (sym != 256) {
        decoded = true;
      } else {
        it5->second.FillExclusion(excl);
      }
    }

    // Order-4
    if (!decoded) {
      uint32_t h4 = h & 0xFFFFFFFF;
      auto it4 = ctx4.find(h4);
      if (it4 != ctx4.end()) {
        uint32_t tot = it4->second.GetWBTotalEx(excl);
        uint32_t f = dec.GetFreq(tot);
        sym = it4->second.FindByFreqWBEx(f, excl);
        uint32_t lo, hi, t;
        it4->second.CumWBEx(sym, excl, lo, hi, t);
        dec.Decode(lo, hi, t);

        if (sym != 256) {
          decoded = true;
        } else {
          it4->second.FillExclusion(excl);
        }
      }
    }

    // Order-3
    if (!decoded) {
      uint32_t h3 = h & 0xFFFFFF;
      auto it3 = ctx3.find(h3);
      if (it3 != ctx3.end()) {
        uint32_t tot = it3->second.GetWBTotalEx(excl);
        uint32_t f = dec.GetFreq(tot);
        sym = it3->second.FindByFreqWBEx(f, excl);
        uint32_t lo, hi, t;
        it3->second.CumWBEx(sym, excl, lo, hi, t);
        dec.Decode(lo, hi, t);

        if (sym != 256) {
          decoded = true;
        } else {
          it3->second.FillExclusion(excl);
        }
      }
    }

    // Order-2
    if (!decoded) {
      uint16_t h2 = h & 0xFFFF;
      Model257 &m2 = ctx2[h2];
      uint32_t tot = m2.GetWBTotalEx(excl);
      uint32_t f = dec.GetFreq(tot);
      sym = m2.FindByFreqWBEx(f, excl);
      uint32_t lo, hi, t;
      m2.CumWBEx(sym, excl, lo, hi, t);
      dec.Decode(lo, hi, t);

      if (sym != 256) {
        decoded = true;
      } else {
        m2.FillExclusion(excl);
      }
    }

    // Order-1
    if (!decoded) {
      uint8_t h1 = h & 0xFF;
      Model257 &m1 = ctx1[h1];
      uint32_t tot = m1.GetWBTotalEx(excl);
      uint32_t f = dec.GetFreq(tot);
      sym = m1.FindByFreqWBEx(f, excl);
      uint32_t lo, hi, t;
      m1.CumWBEx(sym, excl, lo, hi, t);
      dec.Decode(lo, hi, t);

      if (sym != 256) {
        decoded = true;
      } else {
        m1.FillExclusion(excl);
      }
    }

    // Order-0
    if (!decoded) {
      uint32_t f = dec.GetFreq(order0.total);
      sym = order0.FindByFreq(f);
      uint32_t lo, hi;
      order0.Cum(sym, lo, hi);
      dec.Decode(lo, hi, order0.total);

      if (sym == 256)
        break;
    }

    uint8_t b = (uint8_t)sym;
    out.push_back(b);

    ctx5[h5].Bump(b);
    ctx4[h & 0xFFFFFFFF].Bump(b);
    ctx3[h & 0xFFFFFF].Bump(b);
    ctx2[h & 0xFFFF].Bump(b);
    ctx1[h & 0xFF].Bump(b);
    order0.Bump(b);

    h = (h << 8) | b;
  }

  return out;
}

// PPM6: Order-6 with sparse contexts and Witten-Bell exclusion
std::vector<uint8_t> CompressPPM6(const std::vector<uint8_t> &in) {
  std::unordered_map<uint64_t, Model257> ctx6;
  std::unordered_map<uint64_t, Model257> ctx5;
  std::unordered_map<uint32_t, Model257> ctx4;
  std::unordered_map<uint32_t, Model257> ctx3;
  std::vector<Model257> ctx2(256 * 256);
  std::vector<Model257> ctx1(256);

  for (auto &m : ctx2)
    m.InitEscOnly();
  for (auto &m : ctx1)
    m.InitEscOnly();

  Model257 order0;
  order0.InitUniform256();

  OutBuf out;
  RangeEnc enc;
  enc.Init(out);

  uint64_t h = 0;

  for (uint8_t b : in) {
    std::bitset<256> excl;
    bool encoded = false;

    // Order-6
    uint64_t h6 = h & 0xFFFFFFFFFFFFULL;
    auto it6 = ctx6.find(h6);
    if (it6 != ctx6.end() && it6->second.Get(b) != 0) {
      uint32_t lo, hi, tot;
      it6->second.CumWB(b, lo, hi, tot);
      enc.Encode(lo, hi, tot);
      encoded = true;
    } else if (it6 != ctx6.end()) {
      uint32_t lo, hi, tot;
      it6->second.CumWB(256, lo, hi, tot);
      enc.Encode(lo, hi, tot);
      it6->second.FillExclusion(excl);
    }

    // Order-5
    if (!encoded) {
      uint64_t h5 = h & 0xFFFFFFFFFFULL;
      auto it5 = ctx5.find(h5);
      if (it5 != ctx5.end() && it5->second.Get(b) != 0 && !excl[b]) {
        uint32_t lo, hi, tot;
        it5->second.CumWBEx(b, excl, lo, hi, tot);
        enc.Encode(lo, hi, tot);
        encoded = true;
      } else if (it5 != ctx5.end()) {
        uint32_t lo, hi, tot;
        it5->second.CumWBEx(256, excl, lo, hi, tot);
        enc.Encode(lo, hi, tot);
        it5->second.FillExclusion(excl);
      }
    }

    // Order-4
    if (!encoded) {
      uint32_t h4 = h & 0xFFFFFFFF;
      auto it4 = ctx4.find(h4);
      if (it4 != ctx4.end() && it4->second.Get(b) != 0 && !excl[b]) {
        uint32_t lo, hi, tot;
        it4->second.CumWBEx(b, excl, lo, hi, tot);
        enc.Encode(lo, hi, tot);
        encoded = true;
      } else if (it4 != ctx4.end()) {
        uint32_t lo, hi, tot;
        it4->second.CumWBEx(256, excl, lo, hi, tot);
        enc.Encode(lo, hi, tot);
        it4->second.FillExclusion(excl);
      }
    }

    // Order-3
    if (!encoded) {
      uint32_t h3 = h & 0xFFFFFF;
      auto it3 = ctx3.find(h3);
      if (it3 != ctx3.end() && it3->second.Get(b) != 0 && !excl[b]) {
        uint32_t lo, hi, tot;
        it3->second.CumWBEx(b, excl, lo, hi, tot);
        enc.Encode(lo, hi, tot);
        encoded = true;
      } else if (it3 != ctx3.end()) {
        uint32_t lo, hi, tot;
        it3->second.CumWBEx(256, excl, lo, hi, tot);
        enc.Encode(lo, hi, tot);
        it3->second.FillExclusion(excl);
      }
    }

    // Order-2
    if (!encoded) {
      uint16_t h2 = h & 0xFFFF;
      Model257 &m2 = ctx2[h2];
      if (m2.Get(b) != 0 && !excl[b]) {
        uint32_t lo, hi, tot;
        m2.CumWBEx(b, excl, lo, hi, tot);
        enc.Encode(lo, hi, tot);
        encoded = true;
      } else {
        uint32_t lo, hi, tot;
        m2.CumWBEx(256, excl, lo, hi, tot);
        enc.Encode(lo, hi, tot);
        m2.FillExclusion(excl);
      }
    }

    // Order-1
    if (!encoded) {
      uint8_t h1 = h & 0xFF;
      Model257 &m1 = ctx1[h1];
      if (m1.Get(b) != 0 && !excl[b]) {
        uint32_t lo, hi, tot;
        m1.CumWBEx(b, excl, lo, hi, tot);
        enc.Encode(lo, hi, tot);
        encoded = true;
      } else {
        uint32_t lo, hi, tot;
        m1.CumWBEx(256, excl, lo, hi, tot);
        enc.Encode(lo, hi, tot);
        m1.FillExclusion(excl);
      }
    }

    // Order-0
    if (!encoded) {
      uint32_t lo, hi;
      order0.Cum(b, lo, hi);
      enc.Encode(lo, hi, order0.total);
    }

    // Update contexts
    ctx6[h6].Bump(b);
    ctx5[h & 0xFFFFFFFFFFULL].Bump(b);
    ctx4[h & 0xFFFFFFFF].Bump(b);
    ctx3[h & 0xFFFFFF].Bump(b);
    ctx2[h & 0xFFFF].Bump(b);
    ctx1[h & 0xFF].Bump(b);
    order0.Bump(b);

    h = (h << 8) | b;
  }

  // Encode EOF
  {
    std::bitset<256> excl;

    uint64_t h6 = h & 0xFFFFFFFFFFFFULL;
    auto it6 = ctx6.find(h6);
    if (it6 != ctx6.end()) {
      uint32_t lo, hi, tot;
      it6->second.CumWB(256, lo, hi, tot);
      enc.Encode(lo, hi, tot);
      it6->second.FillExclusion(excl);
    }

    uint64_t h5 = h & 0xFFFFFFFFFFULL;
    auto it5 = ctx5.find(h5);
    if (it5 != ctx5.end()) {
      uint32_t lo, hi, tot;
      it5->second.CumWBEx(256, excl, lo, hi, tot);
      enc.Encode(lo, hi, tot);
      it5->second.FillExclusion(excl);
    }

    uint32_t h4 = h & 0xFFFFFFFF;
    auto it4 = ctx4.find(h4);
    if (it4 != ctx4.end()) {
      uint32_t lo, hi, tot;
      it4->second.CumWBEx(256, excl, lo, hi, tot);
      enc.Encode(lo, hi, tot);
      it4->second.FillExclusion(excl);
    }

    uint32_t h3 = h & 0xFFFFFF;
    auto it3 = ctx3.find(h3);
    if (it3 != ctx3.end()) {
      uint32_t lo, hi, tot;
      it3->second.CumWBEx(256, excl, lo, hi, tot);
      enc.Encode(lo, hi, tot);
      it3->second.FillExclusion(excl);
    }

    uint16_t h2 = h & 0xFFFF;
    Model257 &m2 = ctx2[h2];
    uint32_t lo2, hi2, tot2;
    m2.CumWBEx(256, excl, lo2, hi2, tot2);
    enc.Encode(lo2, hi2, tot2);
    m2.FillExclusion(excl);

    uint8_t h1 = h & 0xFF;
    Model257 &m1 = ctx1[h1];
    uint32_t lo1, hi1, tot1;
    m1.CumWBEx(256, excl, lo1, hi1, tot1);
    enc.Encode(lo1, hi1, tot1);

    uint32_t lo0, hi0;
    order0.Cum(256, lo0, hi0);
    enc.Encode(lo0, hi0, order0.total);
  }

  enc.Finish();
  return out.data;
}

std::vector<uint8_t> DecompressPPM6(const std::vector<uint8_t> &in) {
  std::unordered_map<uint64_t, Model257> ctx6;
  std::unordered_map<uint64_t, Model257> ctx5;
  std::unordered_map<uint32_t, Model257> ctx4;
  std::unordered_map<uint32_t, Model257> ctx3;
  std::vector<Model257> ctx2(256 * 256);
  std::vector<Model257> ctx1(256);

  for (auto &m : ctx2)
    m.InitEscOnly();
  for (auto &m : ctx1)
    m.InitEscOnly();

  Model257 order0;
  order0.InitUniform256();

  InBuf ib{in.data(), in.data() + in.size()};
  RangeDec dec;
  dec.Init(ib);

  std::vector<uint8_t> out;
  out.reserve(in.size() * 3);

  uint64_t h = 0;

  while (true) {
    std::bitset<256> excl;
    int sym = 256;
    bool decoded = false;

    // Order-6
    uint64_t h6 = h & 0xFFFFFFFFFFFFULL;
    auto it6 = ctx6.find(h6);
    if (it6 != ctx6.end()) {
      uint32_t tot = it6->second.GetWBTotal();
      uint32_t f = dec.GetFreq(tot);
      sym = it6->second.FindByFreqWB(f);
      uint32_t lo, hi, t;
      it6->second.CumWB(sym, lo, hi, t);
      dec.Decode(lo, hi, t);

      if (sym != 256) {
        decoded = true;
      } else {
        it6->second.FillExclusion(excl);
      }
    }

    // Order-5
    if (!decoded) {
      uint64_t h5 = h & 0xFFFFFFFFFFULL;
      auto it5 = ctx5.find(h5);
      if (it5 != ctx5.end()) {
        uint32_t tot = it5->second.GetWBTotalEx(excl);
        uint32_t f = dec.GetFreq(tot);
        sym = it5->second.FindByFreqWBEx(f, excl);
        uint32_t lo, hi, t;
        it5->second.CumWBEx(sym, excl, lo, hi, t);
        dec.Decode(lo, hi, t);

        if (sym != 256) {
          decoded = true;
        } else {
          it5->second.FillExclusion(excl);
        }
      }
    }

    // Order-4
    if (!decoded) {
      uint32_t h4 = h & 0xFFFFFFFF;
      auto it4 = ctx4.find(h4);
      if (it4 != ctx4.end()) {
        uint32_t tot = it4->second.GetWBTotalEx(excl);
        uint32_t f = dec.GetFreq(tot);
        sym = it4->second.FindByFreqWBEx(f, excl);
        uint32_t lo, hi, t;
        it4->second.CumWBEx(sym, excl, lo, hi, t);
        dec.Decode(lo, hi, t);

        if (sym != 256) {
          decoded = true;
        } else {
          it4->second.FillExclusion(excl);
        }
      }
    }

    // Order-3
    if (!decoded) {
      uint32_t h3 = h & 0xFFFFFF;
      auto it3 = ctx3.find(h3);
      if (it3 != ctx3.end()) {
        uint32_t tot = it3->second.GetWBTotalEx(excl);
        uint32_t f = dec.GetFreq(tot);
        sym = it3->second.FindByFreqWBEx(f, excl);
        uint32_t lo, hi, t;
        it3->second.CumWBEx(sym, excl, lo, hi, t);
        dec.Decode(lo, hi, t);

        if (sym != 256) {
          decoded = true;
        } else {
          it3->second.FillExclusion(excl);
        }
      }
    }

    // Order-2
    if (!decoded) {
      uint16_t h2 = h & 0xFFFF;
      Model257 &m2 = ctx2[h2];
      uint32_t tot = m2.GetWBTotalEx(excl);
      uint32_t f = dec.GetFreq(tot);
      sym = m2.FindByFreqWBEx(f, excl);
      uint32_t lo, hi, t;
      m2.CumWBEx(sym, excl, lo, hi, t);
      dec.Decode(lo, hi, t);

      if (sym != 256) {
        decoded = true;
      } else {
        m2.FillExclusion(excl);
      }
    }

    // Order-1
    if (!decoded) {
      uint8_t h1 = h & 0xFF;
      Model257 &m1 = ctx1[h1];
      uint32_t tot = m1.GetWBTotalEx(excl);
      uint32_t f = dec.GetFreq(tot);
      sym = m1.FindByFreqWBEx(f, excl);
      uint32_t lo, hi, t;
      m1.CumWBEx(sym, excl, lo, hi, t);
      dec.Decode(lo, hi, t);

      if (sym != 256) {
        decoded = true;
      } else {
        m1.FillExclusion(excl);
      }
    }

    // Order-0
    if (!decoded) {
      uint32_t f = dec.GetFreq(order0.total);
      sym = order0.FindByFreq(f);
      uint32_t lo, hi;
      order0.Cum(sym, lo, hi);
      dec.Decode(lo, hi, order0.total);

      if (sym == 256)
        break;
    }

    uint8_t b = (uint8_t)sym;
    out.push_back(b);

    ctx6[h6].Bump(b);
    ctx5[h & 0xFFFFFFFFFFULL].Bump(b);
    ctx4[h & 0xFFFFFFFF].Bump(b);
    ctx3[h & 0xFFFFFF].Bump(b);
    ctx2[h & 0xFFFF].Bump(b);
    ctx1[h & 0xFF].Bump(b);
    order0.Bump(b);

    h = (h << 8) | b;
  }

  return out;
}

// Memory-efficient helper: try compression and keep if better
static void TryCompress(std::vector<uint8_t>& best, int& best_mode,
                        std::vector<uint8_t>&& candidate, int mode) {
  if (best.empty() || candidate.size() < best.size()) {
    best = std::move(candidate);
    best_mode = mode;
  }
}

// Hybrid compressor: tries multiple strategies, picks best
// Memory-efficient: only keeps the best result, discards others immediately
// Format: first byte is mode:
//   0 = PPM5, 1 = LZ77+PPM3, 2 = LZ77+PPM5, 3 = PPM6, 4 = LZ77+PPM6
//   5 = LZOpt+PPM3, 6 = LZOpt+PPM5, 7 = LZOpt+PPM6
//   8 = BWT+MTF+PPM3, 9 = BWT+MTF+PPM5, 10 = LZX+PPM5, 11 = LZX+PPM6
//   12 = CM (Context Mixing), 13 = BWT+MTF+PPM6
//   14 = RLE+PPM5, 15 = RLE+PPM6, 16 = LZ77+BWT+MTF+PPM5
//   17 = Delta+PPM5, 18 = Delta+RLE+PPM5, 19 = Pattern repeat
//   20 = Word+PPM5, 21 = Word+PPM6, 22 = Delta+BWT+MTF+PPM5
//   23 = RLE+LZ77+PPM5, 24 = LZ77+RLE+PPM5, 25 = RLE+BWT+MTF+PPM5
//   26 = LZOpt+RLE+PPM5, 27 = RLE+LZOpt+PPM5
//   28 = RecordInterleave(512)+PPM5, 29 = RecordInterleave(512)+RLE+PPM5
//   30 = Word+RLE+PPM5, 31 = Word+RLE+PPM6
//   32 = Dict+PPM5, 33 = Dict+PPM6, 34 = Word+Dict+PPM6, 255 = Store raw
std::vector<uint8_t> CompressHybrid(const std::vector<uint8_t> &in) {
  std::vector<uint8_t> best;
  int best_mode = 0;

  // Limits for expensive algorithms
  constexpr size_t MAX_BWT_SIZE = 1 << 20;   // 1MB limit for BWT (O(n^2) sort)
  constexpr size_t MAX_LZX_SIZE = 1 << 18;   // 256KB limit for LZX (suffix array)
  constexpr size_t MAX_CM_SIZE = 512 * 1024; // 512KB limit for CM (slow but best ratio)

  // Try PPM5 alone (best for unique text)
  TryCompress(best, best_mode, CompressPPM5(in), 0);

  // Try PPM6 (higher order context)
  TryCompress(best, best_mode, CompressPPM6(in), 3);

  // Try LZ77 preprocessing (64KB window) - fast, always try
  {
    auto lz77_data = LZ77Compress(in);
    TryCompress(best, best_mode, CompressPPM3(lz77_data), 1);
    TryCompress(best, best_mode, CompressPPM5(lz77_data), 2);
    TryCompress(best, best_mode, CompressPPM6(lz77_data), 4);
  }

  // Try LZOpt preprocessing (1MB window) - only for smaller files
  if (in.size() <= 512 * 1024) {
    auto lzopt_data = LZOptCompress(in);
    TryCompress(best, best_mode, CompressPPM3(lzopt_data), 5);
    TryCompress(best, best_mode, CompressPPM5(lzopt_data), 6);
    TryCompress(best, best_mode, CompressPPM6(lzopt_data), 7);
  }

  if (in.size() <= MAX_BWT_SIZE) {
    uint32_t bwt_idx = 0;
    auto bwt_data = BWTEncode(in, bwt_idx);
    auto mtf_data = MTFEncode(bwt_data);
    bwt_data.clear();
    std::vector<uint8_t> prefix = {
      (uint8_t)((bwt_idx >> 24) & 0xFF),
      (uint8_t)((bwt_idx >> 16) & 0xFF),
      (uint8_t)((bwt_idx >> 8) & 0xFF),
      (uint8_t)(bwt_idx & 0xFF)
    };
    auto ppm3 = CompressPPM3(mtf_data);
    auto ppm5 = CompressPPM5(mtf_data);
    auto ppm6 = CompressPPM6(mtf_data);
    std::vector<uint8_t> full3, full5, full6;
    full3.reserve(4 + ppm3.size());
    full3.insert(full3.end(), prefix.begin(), prefix.end());
    full3.insert(full3.end(), ppm3.begin(), ppm3.end());
    full5.reserve(4 + ppm5.size());
    full5.insert(full5.end(), prefix.begin(), prefix.end());
    full5.insert(full5.end(), ppm5.begin(), ppm5.end());
    full6.reserve(4 + ppm6.size());
    full6.insert(full6.end(), prefix.begin(), prefix.end());
    full6.insert(full6.end(), ppm6.begin(), ppm6.end());
    TryCompress(best, best_mode, std::move(full3), 8);
    TryCompress(best, best_mode, std::move(full5), 9);
    TryCompress(best, best_mode, std::move(full6), 13);
  }

  // Try LZX preprocessing (64MB window) - only for smaller files (suffix array is expensive)
  if (in.size() <= MAX_LZX_SIZE) {
    auto lzx_data = LZXCompress(in);
    TryCompress(best, best_mode, CompressPPM5(lzx_data), 10);
    TryCompress(best, best_mode, CompressPPM6(lzx_data), 11);
  }

  // Try CM (Context Mixing) - PAQ-style, best ratio but slow
  if (in.size() <= MAX_CM_SIZE) {
    TryCompress(best, best_mode, CompressCM(in), 12);
  }

  // Try RLE preprocessing - good for files with many repeated bytes (TAR, binary)
  {
    auto rle_data = RLECompress(in);
    TryCompress(best, best_mode, CompressPPM5(rle_data), 14);
    TryCompress(best, best_mode, CompressPPM6(rle_data), 15);
  }

  if (in.size() <= MAX_BWT_SIZE) {
    auto lz_data = LZ77Compress(in);
    uint32_t bwt_idx = 0;
    auto bwt_data = BWTEncode(lz_data, bwt_idx);
    auto mtf_data = MTFEncode(bwt_data);
    bwt_data.clear();
    std::vector<uint8_t> prefix = {
      (uint8_t)((bwt_idx >> 24) & 0xFF),
      (uint8_t)((bwt_idx >> 16) & 0xFF),
      (uint8_t)((bwt_idx >> 8) & 0xFF),
      (uint8_t)(bwt_idx & 0xFF)
    };
    auto ppm5 = CompressPPM5(mtf_data);
    std::vector<uint8_t> full;
    full.reserve(4 + ppm5.size());
    full.insert(full.end(), prefix.begin(), prefix.end());
    full.insert(full.end(), ppm5.begin(), ppm5.end());
    TryCompress(best, best_mode, std::move(full), 16);
  }

  // Try Delta encoding - good for binary files with gradual value changes
  {
    auto delta_data = DeltaEncode(in);
    TryCompress(best, best_mode, CompressPPM5(delta_data), 17);

    // Delta + RLE for binary with both patterns
    auto delta_rle = RLECompress(delta_data);
    TryCompress(best, best_mode, CompressPPM5(delta_rle), 18);
  }

  // Pattern encoding disabled - decompression bug
  // {
  //   auto pattern_data = PatternEncode(in);
  //   if (!pattern_data.empty()) {
  //     TryCompress(best, best_mode, std::move(pattern_data), 19);
  //   }
  // }

  // Try Word tokenization - good for HTML/text with common patterns
  {
    auto word_data = WordEncode(in);
    if (word_data.size() < in.size()) {  // Only if tokenization helps
      TryCompress(best, best_mode, CompressPPM5(word_data), 20);
      TryCompress(best, best_mode, CompressPPM6(word_data), 21);

      // Word+RLE is especially good for TAR and similar archives
      auto word_rle = RLECompress(word_data);
      TryCompress(best, best_mode, CompressPPM5(word_rle), 30);
      TryCompress(best, best_mode, CompressPPM6(word_rle), 31);

      // Word+LZ77+PPM - good for XML/Wiki content with repeated structures
      auto word_lz = LZ77Compress(word_data);
      TryCompress(best, best_mode, CompressPPM5(word_lz), 35);
      TryCompress(best, best_mode, CompressPPM6(word_lz), 36);
    }
  }

  // Try LZ77+Word+PPM - find matches first, then tokenize
  {
    auto lz_data = LZ77Compress(in);
    auto lz_word = WordEncode(lz_data);
    if (lz_word.size() < lz_data.size()) {
      TryCompress(best, best_mode, CompressPPM5(lz_word), 37);
      TryCompress(best, best_mode, CompressPPM6(lz_word), 38);
    }
  }

  if (in.size() <= MAX_BWT_SIZE) {
    auto delta_data = DeltaEncode(in);
    uint32_t bwt_idx = 0;
    auto bwt_data = BWTEncode(delta_data, bwt_idx);
    auto mtf_data = MTFEncode(bwt_data);
    bwt_data.clear();
    std::vector<uint8_t> prefix = {
      (uint8_t)((bwt_idx >> 24) & 0xFF),
      (uint8_t)((bwt_idx >> 16) & 0xFF),
      (uint8_t)((bwt_idx >> 8) & 0xFF),
      (uint8_t)(bwt_idx & 0xFF)
    };
    auto ppm5 = CompressPPM5(mtf_data);
    std::vector<uint8_t> full;
    full.reserve(4 + ppm5.size());
    full.insert(full.end(), prefix.begin(), prefix.end());
    full.insert(full.end(), ppm5.begin(), ppm5.end());
    TryCompress(best, best_mode, std::move(full), 22);
  }

  // Try RLE+LZ77 - RLE first removes runs, then LZ77 finds patterns
  {
    auto rle_data = RLECompress(in);
    auto lz_data = LZ77Compress(rle_data);
    TryCompress(best, best_mode, CompressPPM5(lz_data), 23);
  }

  // Try LZ77+RLE - LZ77 first finds patterns, then RLE handles remaining runs
  {
    auto lz_data = LZ77Compress(in);
    auto rle_data = RLECompress(lz_data);
    TryCompress(best, best_mode, CompressPPM5(rle_data), 24);
  }

  if (in.size() <= MAX_BWT_SIZE) {
    auto rle_data = RLECompress(in);
    uint32_t bwt_idx = 0;
    auto bwt_data = BWTEncode(rle_data, bwt_idx);
    auto mtf_data = MTFEncode(bwt_data);
    bwt_data.clear();
    std::vector<uint8_t> prefix = {
      (uint8_t)((bwt_idx >> 24) & 0xFF),
      (uint8_t)((bwt_idx >> 16) & 0xFF),
      (uint8_t)((bwt_idx >> 8) & 0xFF),
      (uint8_t)(bwt_idx & 0xFF)
    };
    auto ppm5 = CompressPPM5(mtf_data);
    std::vector<uint8_t> full;
    full.reserve(4 + ppm5.size());
    full.insert(full.end(), prefix.begin(), prefix.end());
    full.insert(full.end(), ppm5.begin(), ppm5.end());
    TryCompress(best, best_mode, std::move(full), 25);
  }

  // Try LZOpt+RLE - optimal parsing + run encoding for archives
  if (in.size() <= 512 * 1024) {
    auto lzopt_data = LZOptCompress(in);
    auto rle_data = RLECompress(lzopt_data);
    TryCompress(best, best_mode, CompressPPM5(rle_data), 26);

    // Also try RLE first, then LZOpt
    auto rle_first = RLECompress(in);
    auto lzopt_then = LZOptCompress(rle_first);
    TryCompress(best, best_mode, CompressPPM5(lzopt_then), 27);
  }

  // Try Record Interleave with 512-byte blocks (for TAR and similar formats)
  // This groups same positions across records together, improving PPM context
  if (in.size() >= 1024 && in.size() <= 1024 * 1024) {  // 1KB to 1MB
    auto rec512 = RecordInterleave(in, 512);
    TryCompress(best, best_mode, CompressPPM5(rec512), 28);

    // Also try with RLE after interleaving (groups runs of same byte)
    auto rec512_rle = RLECompress(rec512);
    TryCompress(best, best_mode, CompressPPM5(rec512_rle), 29);
  }

  // Try Dictionary-based compression for small files
  // Prepends a static dictionary to bootstrap PPM with common patterns
  // This helps small files (like brotli's dictionary approach)
  if (in.size() <= 65535) {  // Only for files up to 64KB
    auto dict_data = DictEncode(in);
    // Compress dict+data together, PPM learns from dict first
    TryCompress(best, best_mode, CompressPPM5(dict_data), 32);
    TryCompress(best, best_mode, CompressPPM6(dict_data), 33);

    // Also try Word+Dict for HTML/text
    auto word_data = WordEncode(in);
    if (word_data.size() < in.size()) {
      auto word_dict = DictEncode(word_data);
      TryCompress(best, best_mode, CompressPPM6(word_dict), 34);
    }
  }

  // Try Sparse encoding - optimized for files with many zeros (TAR, binary)
  {
    auto sparse_data = SparseEncode(in);
    if (sparse_data.size() < in.size()) {
      TryCompress(best, best_mode, CompressPPM5(sparse_data), 39);
      TryCompress(best, best_mode, CompressPPM6(sparse_data), 40);

      // Sparse+Word - for TAR with source code
      auto sparse_word = WordEncode(sparse_data);
      if (sparse_word.size() < sparse_data.size()) {
        TryCompress(best, best_mode, CompressPPM6(sparse_word), 41);
      }
    }
  }

  // Try LZMA-style optimal parsing (1MB window) - best for text/code
  {
    auto lzma_data = LZMACompress(in);
    TryCompress(best, best_mode, CompressPPM5(lzma_data), 42);
    TryCompress(best, best_mode, CompressPPM6(lzma_data), 43);

    if (lzma_data.size() <= MAX_BWT_SIZE) {
      uint32_t bwt_idx = 0;
      auto bwt_data = BWTEncode(lzma_data, bwt_idx);
      auto mtf_data = MTFEncode(bwt_data);
      bwt_data.clear();
      std::vector<uint8_t> prefix = {
        (uint8_t)((bwt_idx >> 24) & 0xFF),
        (uint8_t)((bwt_idx >> 16) & 0xFF),
        (uint8_t)((bwt_idx >> 8) & 0xFF),
        (uint8_t)(bwt_idx & 0xFF)
      };
      auto ppm5 = CompressPPM5(mtf_data);
      std::vector<uint8_t> full;
      full.reserve(4 + ppm5.size());
      full.insert(full.end(), prefix.begin(), prefix.end());
      full.insert(full.end(), ppm5.begin(), ppm5.end());
      TryCompress(best, best_mode, std::move(full), 44);
    }
  }

  // Try Word+LZMA - tokenize first for better long-range matching
  {
    auto word_data = WordEncode(in);
    if (word_data.size() < in.size()) {
      auto lzma_data = LZMACompress(word_data);
      TryCompress(best, best_mode, CompressPPM5(lzma_data), 45);
      TryCompress(best, best_mode, CompressPPM6(lzma_data), 46);
    }
  }

  // Try Dict+LZMA for small files
  if (in.size() <= 65535) {
    auto dict_data = DictEncode(in);
    auto lzma_data = LZMACompress(dict_data);
    TryCompress(best, best_mode, CompressPPM5(lzma_data), 47);
    TryCompress(best, best_mode, CompressPPM6(lzma_data), 48);
  }

  // Try RLE+LZMA for TAR/archives
  {
    auto rle_data = RLECompress(in);
    if (rle_data.size() < in.size()) {
      auto lzma_data = LZMACompress(rle_data);
      TryCompress(best, best_mode, CompressPPM5(lzma_data), 49);
      TryCompress(best, best_mode, CompressPPM6(lzma_data), 50);
    }
  }

  // If nothing compresses well, store raw (mode 255)
  // Only store raw if compressed size >= original size
  if (best.size() >= in.size()) {
    std::vector<uint8_t> result;
    result.reserve(1 + in.size());
    result.push_back(255);  // Store raw mode
    result.insert(result.end(), in.begin(), in.end());
    return result;
  }

  // Build final result
  std::vector<uint8_t> result;
  result.reserve(1 + best.size());
  result.push_back((uint8_t)best_mode);
  result.insert(result.end(), best.begin(), best.end());

  return result;
}

std::vector<uint8_t> DecompressHybrid(const std::vector<uint8_t> &in) {
  if (in.empty()) return {};

  uint8_t mode = in[0];
  std::vector<uint8_t> payload(in.begin() + 1, in.end());

  switch (mode) {
    case 0: // PPM5
      return DecompressPPM5(payload);
    case 1: { // LZ77+PPM3
      auto lz77_data = DecompressPPM3(payload);
      return LZ77Decompress(lz77_data);
    }
    case 2: { // LZ77+PPM5
      auto lz77_data = DecompressPPM5(payload);
      return LZ77Decompress(lz77_data);
    }
    case 3: // PPM6
      return DecompressPPM6(payload);
    case 4: { // LZ77+PPM6
      auto lz77_data = DecompressPPM6(payload);
      return LZ77Decompress(lz77_data);
    }
    case 5: { // LZOpt+PPM3
      auto lzopt_data = DecompressPPM3(payload);
      return LZOptDecompress(lzopt_data);
    }
    case 6: { // LZOpt+PPM5
      auto lzopt_data = DecompressPPM5(payload);
      return LZOptDecompress(lzopt_data);
    }
    case 7: { // LZOpt+PPM6
      auto lzopt_data = DecompressPPM6(payload);
      return LZOptDecompress(lzopt_data);
    }
    case 8: { // BWT+MTF+PPM3
      if (payload.size() < 4) return {};
      uint32_t bwt_idx = ((uint32_t)payload[0] << 24) | ((uint32_t)payload[1] << 16) |
                         ((uint32_t)payload[2] << 8) | payload[3];
      std::vector<uint8_t> ppm_payload(payload.begin() + 4, payload.end());
      auto mtf_data = DecompressPPM3(ppm_payload);
      auto bwt_data = MTFDecode(mtf_data);
      return BWTDecode(bwt_data, bwt_idx);
    }
    case 9: { // BWT+MTF+PPM5
      if (payload.size() < 4) return {};
      uint32_t bwt_idx = ((uint32_t)payload[0] << 24) | ((uint32_t)payload[1] << 16) |
                         ((uint32_t)payload[2] << 8) | payload[3];
      std::vector<uint8_t> ppm_payload(payload.begin() + 4, payload.end());
      auto mtf_data = DecompressPPM5(ppm_payload);
      auto bwt_data = MTFDecode(mtf_data);
      return BWTDecode(bwt_data, bwt_idx);
    }
    case 10: { // LZX+PPM5
      auto lzx_data = DecompressPPM5(payload);
      return LZXDecompress(lzx_data);
    }
    case 11: { // LZX+PPM6
      auto lzx_data = DecompressPPM6(payload);
      return LZXDecompress(lzx_data);
    }
    case 12: // CM (Context Mixing)
      return DecompressCM(payload);
    case 13: { // BWT+MTF+PPM6
      if (payload.size() < 4) return {};
      uint32_t bwt_idx = ((uint32_t)payload[0] << 24) | ((uint32_t)payload[1] << 16) |
                         ((uint32_t)payload[2] << 8) | payload[3];
      std::vector<uint8_t> ppm_payload(payload.begin() + 4, payload.end());
      auto mtf_data = DecompressPPM6(ppm_payload);
      auto bwt_data = MTFDecode(mtf_data);
      return BWTDecode(bwt_data, bwt_idx);
    }
    case 14: { // RLE+PPM5
      auto rle_data = DecompressPPM5(payload);
      return RLEDecompress(rle_data);
    }
    case 15: { // RLE+PPM6
      auto rle_data = DecompressPPM6(payload);
      return RLEDecompress(rle_data);
    }
    case 16: { // LZ77+BWT+MTF+PPM5
      if (payload.size() < 4) return {};
      uint32_t bwt_idx = ((uint32_t)payload[0] << 24) | ((uint32_t)payload[1] << 16) |
                         ((uint32_t)payload[2] << 8) | payload[3];
      std::vector<uint8_t> ppm_payload(payload.begin() + 4, payload.end());
      auto mtf_data = DecompressPPM5(ppm_payload);
      auto bwt_data = MTFDecode(mtf_data);
      auto lz_data = BWTDecode(bwt_data, bwt_idx);
      return LZ77Decompress(lz_data);
    }
    case 17: { // Delta+PPM5
      auto delta_data = DecompressPPM5(payload);
      return DeltaDecode(delta_data);
    }
    case 18: { // Delta+RLE+PPM5
      auto rle_data = DecompressPPM5(payload);
      auto delta_data = RLEDecompress(rle_data);
      return DeltaDecode(delta_data);
    }
    case 19: // Pattern repeat
      return PatternDecode(payload);
    case 20: { // Word+PPM5
      auto word_data = DecompressPPM5(payload);
      return WordDecode(word_data);
    }
    case 21: { // Word+PPM6
      auto word_data = DecompressPPM6(payload);
      return WordDecode(word_data);
    }
    case 22: { // Delta+BWT+MTF+PPM5
      if (payload.size() < 4) return {};
      uint32_t bwt_idx = ((uint32_t)payload[0] << 24) | ((uint32_t)payload[1] << 16) |
                         ((uint32_t)payload[2] << 8) | payload[3];
      std::vector<uint8_t> ppm_payload(payload.begin() + 4, payload.end());
      auto mtf_data = DecompressPPM5(ppm_payload);
      auto bwt_data = MTFDecode(mtf_data);
      auto delta_data = BWTDecode(bwt_data, bwt_idx);
      return DeltaDecode(delta_data);
    }
    case 23: { // RLE+LZ77+PPM5
      auto lz_data = DecompressPPM5(payload);
      auto rle_data = LZ77Decompress(lz_data);
      return RLEDecompress(rle_data);
    }
    case 24: { // LZ77+RLE+PPM5
      auto rle_data = DecompressPPM5(payload);
      auto lz_data = RLEDecompress(rle_data);
      return LZ77Decompress(lz_data);
    }
    case 25: { // RLE+BWT+MTF+PPM5
      if (payload.size() < 4) return {};
      uint32_t bwt_idx = ((uint32_t)payload[0] << 24) | ((uint32_t)payload[1] << 16) |
                         ((uint32_t)payload[2] << 8) | payload[3];
      std::vector<uint8_t> ppm_payload(payload.begin() + 4, payload.end());
      auto mtf_data = DecompressPPM5(ppm_payload);
      auto bwt_data = MTFDecode(mtf_data);
      auto rle_data = BWTDecode(bwt_data, bwt_idx);
      return RLEDecompress(rle_data);
    }
    case 26: { // LZOpt+RLE+PPM5
      auto rle_data = DecompressPPM5(payload);
      auto lzopt_data = RLEDecompress(rle_data);
      return LZOptDecompress(lzopt_data);
    }
    case 27: { // RLE+LZOpt+PPM5
      auto lzopt_data = DecompressPPM5(payload);
      auto rle_data = LZOptDecompress(lzopt_data);
      return RLEDecompress(rle_data);
    }
    case 28: { // RecordInterleave(512)+PPM5
      auto rec_data = DecompressPPM5(payload);
      return RecordDeinterleave(rec_data);
    }
    case 29: { // RecordInterleave(512)+RLE+PPM5
      auto rle_data = DecompressPPM5(payload);
      auto rec_data = RLEDecompress(rle_data);
      return RecordDeinterleave(rec_data);
    }
    case 30: { // Word+RLE+PPM5
      auto rle_data = DecompressPPM5(payload);
      auto word_data = RLEDecompress(rle_data);
      return WordDecode(word_data);
    }
    case 31: { // Word+RLE+PPM6
      auto rle_data = DecompressPPM6(payload);
      auto word_data = RLEDecompress(rle_data);
      return WordDecode(word_data);
    }
    case 32: { // Dict+PPM5
      auto dict_data = DecompressPPM5(payload);
      return DictDecode(dict_data);
    }
    case 33: { // Dict+PPM6
      auto dict_data = DecompressPPM6(payload);
      return DictDecode(dict_data);
    }
    case 34: { // Word+Dict+PPM6
      auto dict_data = DecompressPPM6(payload);
      auto word_data = DictDecode(dict_data);
      return WordDecode(word_data);
    }
    case 35: { // Word+LZ77+PPM5
      auto word_lz = DecompressPPM5(payload);
      auto word_data = LZ77Decompress(word_lz);
      return WordDecode(word_data);
    }
    case 36: { // Word+LZ77+PPM6
      auto word_lz = DecompressPPM6(payload);
      auto word_data = LZ77Decompress(word_lz);
      return WordDecode(word_data);
    }
    case 37: { // LZ77+Word+PPM5
      auto lz_word = DecompressPPM5(payload);
      auto lz_data = WordDecode(lz_word);
      return LZ77Decompress(lz_data);
    }
    case 38: { // LZ77+Word+PPM6
      auto lz_word = DecompressPPM6(payload);
      auto lz_data = WordDecode(lz_word);
      return LZ77Decompress(lz_data);
    }
    case 39: { // Sparse+PPM5
      auto sparse_data = DecompressPPM5(payload);
      return SparseDecode(sparse_data);
    }
    case 40: { // Sparse+PPM6
      auto sparse_data = DecompressPPM6(payload);
      return SparseDecode(sparse_data);
    }
    case 41: { // Sparse+Word+PPM6
      auto sparse_word = DecompressPPM6(payload);
      auto sparse_data = WordDecode(sparse_word);
      return SparseDecode(sparse_data);
    }
    case 42: { // LZMA+PPM5
      auto lzma_data = DecompressPPM5(payload);
      return LZMADecompress(lzma_data);
    }
    case 43: { // LZMA+PPM6
      auto lzma_data = DecompressPPM6(payload);
      return LZMADecompress(lzma_data);
    }
    case 44: { // LZMA+BWT+MTF+PPM5
      if (payload.size() < 4) return {};
      uint32_t bwt_idx = ((uint32_t)payload[0] << 24) | ((uint32_t)payload[1] << 16) |
                         ((uint32_t)payload[2] << 8) | payload[3];
      std::vector<uint8_t> ppm_payload(payload.begin() + 4, payload.end());
      auto mtf_data = DecompressPPM5(ppm_payload);
      auto bwt_data = MTFDecode(mtf_data);
      auto lzma_data = BWTDecode(bwt_data, bwt_idx);
      return LZMADecompress(lzma_data);
    }
    case 45: { // Word+LZMA+PPM5
      auto lzma_data = DecompressPPM5(payload);
      auto word_data = LZMADecompress(lzma_data);
      return WordDecode(word_data);
    }
    case 46: { // Word+LZMA+PPM6
      auto lzma_data = DecompressPPM6(payload);
      auto word_data = LZMADecompress(lzma_data);
      return WordDecode(word_data);
    }
    case 47: { // Dict+LZMA+PPM5
      auto lzma_data = DecompressPPM5(payload);
      auto dict_data = LZMADecompress(lzma_data);
      return DictDecode(dict_data);
    }
    case 48: { // Dict+LZMA+PPM6
      auto lzma_data = DecompressPPM6(payload);
      auto dict_data = LZMADecompress(lzma_data);
      return DictDecode(dict_data);
    }
    case 49: { // RLE+LZMA+PPM5
      auto lzma_data = DecompressPPM5(payload);
      auto rle_data = LZMADecompress(lzma_data);
      return RLEDecompress(rle_data);
    }
    case 50: { // RLE+LZMA+PPM6
      auto lzma_data = DecompressPPM6(payload);
      auto rle_data = LZMADecompress(lzma_data);
      return RLEDecompress(rle_data);
    }
    case 255: // Store raw (incompressible data)
      return payload;
    default:
      return DecompressPPM5(payload);
  }
}
