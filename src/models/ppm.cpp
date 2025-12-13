#include "ppm.hpp"
#include "../core/range_coder.hpp"
#include "../io/buffer.hpp"
#include "model257.hpp"
#include <array>

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
