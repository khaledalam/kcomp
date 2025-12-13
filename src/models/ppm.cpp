#include "ppm.hpp"
#include "../core/range_coder.hpp"
#include "../io/buffer.hpp"
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
    bool encoded = false;

    uint32_t idx2 = ((uint32_t)prev2 << 8) | prev1;
    Model257 &m2 = ctx2[idx2];
    if (m2.Get(b) != 0) {
      uint32_t lo, hi;
      m2.Cum(b, lo, hi);
      enc.Encode(lo, hi, m2.total);
      encoded = true;
    } else {
      uint32_t lo, hi;
      m2.Cum(256, lo, hi);
      enc.Encode(lo, hi, m2.total);
    }

    if (!encoded) {
      Model257 &m1 = ctx1[prev1];
      if (m1.Get(b) != 0) {
        uint32_t lo, hi;
        m1.Cum(b, lo, hi);
        enc.Encode(lo, hi, m1.total);
        encoded = true;
      } else {
        uint32_t lo, hi;
        m1.Cum(256, lo, hi);
        enc.Encode(lo, hi, m1.total);
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
    uint32_t idx2 = ((uint32_t)prev2 << 8) | prev1;
    Model257 &m2 = ctx2[idx2];
    uint32_t lo2, hi2;
    m2.Cum(256, lo2, hi2);
    enc.Encode(lo2, hi2, m2.total);

    Model257 &m1 = ctx1[prev1];
    uint32_t lo1, hi1;
    m1.Cum(256, lo1, hi1);
    enc.Encode(lo1, hi1, m1.total);

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
    int sym = 256;
    bool decoded = false;

    uint32_t idx2 = ((uint32_t)prev2 << 8) | prev1;
    Model257 &m2 = ctx2[idx2];
    {
      uint32_t f = dec.GetFreq(m2.total);
      sym = m2.FindByFreq(f);
      uint32_t lo, hi;
      m2.Cum(sym, lo, hi);
      dec.Decode(lo, hi, m2.total);

      if (sym != 256) {
        decoded = true;
      }
    }

    if (!decoded) {
      Model257 &m1 = ctx1[prev1];
      uint32_t f = dec.GetFreq(m1.total);
      sym = m1.FindByFreq(f);
      uint32_t lo, hi;
      m1.Cum(sym, lo, hi);
      dec.Decode(lo, hi, m1.total);

      if (sym != 256) {
        decoded = true;
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
