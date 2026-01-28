#include "cm.hpp"
#include <array>
#include <vector>
#include <cmath>
#include <algorithm>

namespace {

std::array<int, 4096> stretch_tbl;
std::array<int, 8192> squash_tbl;
bool tables_init = false;

void InitTables() {
  if (tables_init) return;
  for (int i = 0; i < 4096; i++) {
    double p = (i + 0.5) / 4096.0;
    stretch_tbl[i] = int(512.0 * std::log(p / (1.0 - p)));
  }
  for (int i = 0; i < 8192; i++) {
    double x = (i - 4096) / 512.0;
    squash_tbl[i] = int(4096.0 / (1.0 + std::exp(-x)));
    squash_tbl[i] = std::clamp(squash_tbl[i], 1, 4095);
  }
  tables_init = true;
}

inline int Stretch(int p) { return stretch_tbl[std::clamp(p, 0, 4095)]; }
inline int Squash(int x) { return squash_tbl[std::clamp(x + 4096, 0, 8191)]; }

struct StateTable {
  std::array<uint8_t, 512> next_state;
  std::array<uint8_t, 256> state_map;

  StateTable() {
    for (int i = 0; i < 256; i++) {
      int n0 = (i >> 4) & 15;
      int n1 = i & 15;

      state_map[i] = (n1 * 255) / std::max(n0 + n1, 1);

      int new_n0 = n0, new_n1 = n1;

      new_n0 = std::min(n0 + 1, 15);
      if (new_n0 + n1 > 15) new_n1 = n1 * 14 / 15;
      next_state[i * 2] = (new_n0 << 4) | new_n1;

      new_n0 = n0;
      new_n1 = std::min(n1 + 1, 15);
      if (n0 + new_n1 > 15) new_n0 = n0 * 14 / 15;
      next_state[i * 2 + 1] = (new_n0 << 4) | new_n1;
    }
  }
};

static StateTable st;

class ContextModel {
  std::vector<uint8_t> states;
  size_t mask;

public:
  ContextModel(int bits) : states(1 << bits, 0), mask((1 << bits) - 1) {}

  int Predict(uint32_t ctx) const {
    return st.state_map[states[ctx & mask]] * 16;
  }

  void Update(uint32_t ctx, int bit) {
    uint8_t& s = states[ctx & mask];
    s = st.next_state[s * 2 + bit];
  }
};

class MatchModel {
  std::vector<uint32_t> hash_table;
  std::vector<uint8_t> history;
  size_t hist_pos = 0;
  int match_len = 0;
  size_t match_pos = 0;
  int predicted_bit = 0;
  int confidence = 0;

public:
  MatchModel(size_t hist_size = 1 << 20, size_t hash_size = 1 << 18)
      : hash_table(hash_size, 0), history(hist_size, 0) {}

  void Update(uint32_t ctx, int bit, uint8_t byte_ctx) {
    if ((ctx & 0xFF) == 1) {
      history[hist_pos % history.size()] = byte_ctx;
      hist_pos++;
    }

    if (match_len > 0) {
      if (bit == predicted_bit) {
        confidence = std::min(confidence + 1, 7);
      } else {
        match_len = 0;
        confidence = 0;
      }
    }

    if ((ctx & 0xFF) == 1 && hist_pos > 8) {
      uint32_t h = 0;
      for (int i = 0; i < 8; i++) {
        h = h * 257 + history[(hist_pos - 8 + i) % history.size()];
      }
      h &= (hash_table.size() - 1);

      if (match_len == 0) {
        size_t prev = hash_table[h];
        if (prev > 0 && prev < hist_pos - 8) {
          bool valid = true;
          for (int i = 0; i < 8 && valid; i++) {
            if (history[(prev + i) % history.size()] !=
                history[(hist_pos - 8 + i) % history.size()]) {
              valid = false;
            }
          }
          if (valid) {
            match_pos = prev + 8;
            match_len = 1;
            confidence = 1;
          }
        }
      }

      hash_table[h] = hist_pos - 8;
    }
  }

  int Predict(uint32_t bit_ctx) {
    if (match_len == 0) return 2048;

    uint8_t pred_byte = history[match_pos % history.size()];
    int bit_pos = 7 - ((bit_ctx & 0xFF) - 1);
    if (bit_pos < 0 || bit_pos > 7) return 2048;

    predicted_bit = (pred_byte >> bit_pos) & 1;

    int p = predicted_bit ? 4095 - (512 >> confidence) : (512 >> confidence);
    return p;
  }

  void ByteDone() {
    if (match_len > 0) {
      match_pos++;
      match_len++;
    }
  }
};

class Mixer {
  static constexpr int N = 8;
  std::array<int, N> inputs;
  std::array<int, N> weights;
  int n_inputs = 0;
  int pr = 2048;

public:
  Mixer() { Reset(); }

  void Reset() {
    weights.fill(256);
    n_inputs = 0;
  }

  void Add(int p) {
    if (n_inputs < N) {
      inputs[n_inputs++] = Stretch(std::clamp(p, 1, 4095));
    }
  }

  int Mix() {
    if (n_inputs == 0) return 2048;

    int64_t sum = 0;
    int64_t w_sum = 0;
    for (int i = 0; i < n_inputs; i++) {
      sum += (int64_t)inputs[i] * weights[i];
      w_sum += weights[i];
    }

    pr = Squash(w_sum > 0 ? (int)(sum / w_sum) : 0);
    n_inputs = 0;
    return pr;
  }

  void Update(int bit) {
    int err = ((bit << 12) - pr) * 7;
    for (int i = 0; i < N; i++) {
      weights[i] += (inputs[i] * err) >> 16;
      weights[i] = std::clamp(weights[i], 1, 65535);
    }
  }
};

class BitEncoder {
  uint32_t low = 0;
  uint32_t high = 0xFFFFFFFF;
  std::vector<uint8_t>& out;

public:
  BitEncoder(std::vector<uint8_t>& o) : out(o) {}

  void Encode(int bit, int p) {
    uint32_t mid = low + ((uint64_t)(high - low) * p >> 12);
    if (bit) {
      low = mid + 1;
    } else {
      high = mid;
    }

    while ((low ^ high) < 0x1000000) {
      out.push_back(low >> 24);
      low <<= 8;
      high = (high << 8) | 0xFF;
    }
  }

  void Flush() {
    out.push_back((low >> 24) & 0xFF);
    out.push_back((low >> 16) & 0xFF);
    out.push_back((low >> 8) & 0xFF);
    out.push_back(low & 0xFF);
  }
};

class BitDecoder {
  uint32_t low = 0;
  uint32_t high = 0xFFFFFFFF;
  uint32_t code = 0;
  const uint8_t* ptr;
  const uint8_t* end;

public:
  BitDecoder(const uint8_t* data, size_t size)
      : ptr(data), end(data + size) {
    for (int i = 0; i < 4 && ptr < end; i++) {
      code = (code << 8) | *ptr++;
    }
  }

  int Decode(int p) {
    uint32_t mid = low + ((uint64_t)(high - low) * p >> 12);
    int bit = (code > mid);

    if (bit) {
      low = mid + 1;
    } else {
      high = mid;
    }

    while ((low ^ high) < 0x1000000) {
      low <<= 8;
      high = (high << 8) | 0xFF;
      code = (code << 8) | (ptr < end ? *ptr++ : 0);
    }

    return bit;
  }
};

}

std::vector<uint8_t> CompressCM(const std::vector<uint8_t>& in) {
  if (in.empty()) return {};

  InitTables();

  std::vector<uint8_t> out;
  out.reserve(in.size());

  uint32_t size = in.size();
  out.push_back((size >> 24) & 0xFF);
  out.push_back((size >> 16) & 0xFF);
  out.push_back((size >> 8) & 0xFF);
  out.push_back(size & 0xFF);

  BitEncoder enc(out);

  ContextModel cm0(8);
  ContextModel cm1(16);
  ContextModel cm2(20);
  ContextModel cm3(22);
  ContextModel cm4(24);

  MatchModel mm;
  Mixer mixer;

  uint32_t ctx1 = 0;
  uint32_t ctx2 = 0;
  uint32_t ctx3 = 0;
  uint32_t ctx4 = 0;

  for (uint8_t byte : in) {
    uint32_t bit_ctx = 1;

    for (int i = 7; i >= 0; i--) {
      int bit = (byte >> i) & 1;

      int p0 = cm0.Predict(bit_ctx);
      int p1 = cm1.Predict((ctx1 << 8) | bit_ctx);
      int p2 = cm2.Predict(((ctx2 & 0xFFF) << 8) | bit_ctx);
      int p3 = cm3.Predict(((ctx3 & 0x3FFF) << 8) | bit_ctx);
      int p4 = cm4.Predict(((ctx4 & 0xFFFF) << 8) | bit_ctx);
      int pm = mm.Predict(bit_ctx);

      mixer.Add(p0);
      mixer.Add(p1);
      mixer.Add(p2);
      mixer.Add(p3);
      mixer.Add(p4);
      mixer.Add(pm);
      mixer.Add(2048);
      mixer.Add(2048);

      int p = mixer.Mix();

      enc.Encode(bit, p);

      cm0.Update(bit_ctx, bit);
      cm1.Update((ctx1 << 8) | bit_ctx, bit);
      cm2.Update(((ctx2 & 0xFFF) << 8) | bit_ctx, bit);
      cm3.Update(((ctx3 & 0x3FFF) << 8) | bit_ctx, bit);
      cm4.Update(((ctx4 & 0xFFFF) << 8) | bit_ctx, bit);
      mm.Update(bit_ctx, bit, ctx1 & 0xFF);
      mixer.Update(bit);

      bit_ctx = (bit_ctx << 1) | bit;
    }

    mm.ByteDone();

    ctx4 = (ctx4 << 8) | ctx3 >> 24;
    ctx3 = (ctx3 << 8) | ctx2 >> 16;
    ctx2 = (ctx2 << 8) | ctx1 >> 8;
    ctx1 = (ctx1 << 8) | byte;
  }

  enc.Flush();
  return out;
}

std::vector<uint8_t> DecompressCM(const std::vector<uint8_t>& in) {
  if (in.size() < 4) return {};

  InitTables();

  uint32_t size = ((uint32_t)in[0] << 24) | ((uint32_t)in[1] << 16) |
                  ((uint32_t)in[2] << 8) | in[3];

  if (size > 100 * 1024 * 1024) return {};

  std::vector<uint8_t> out;
  out.reserve(size);

  BitDecoder dec(in.data() + 4, in.size() - 4);

  ContextModel cm0(8);
  ContextModel cm1(16);
  ContextModel cm2(20);
  ContextModel cm3(22);
  ContextModel cm4(24);

  MatchModel mm;
  Mixer mixer;

  uint32_t ctx1 = 0;
  uint32_t ctx2 = 0;
  uint32_t ctx3 = 0;
  uint32_t ctx4 = 0;

  for (uint32_t n = 0; n < size; n++) {
    uint32_t bit_ctx = 1;
    uint8_t byte = 0;

    for (int i = 7; i >= 0; i--) {
      int p0 = cm0.Predict(bit_ctx);
      int p1 = cm1.Predict((ctx1 << 8) | bit_ctx);
      int p2 = cm2.Predict(((ctx2 & 0xFFF) << 8) | bit_ctx);
      int p3 = cm3.Predict(((ctx3 & 0x3FFF) << 8) | bit_ctx);
      int p4 = cm4.Predict(((ctx4 & 0xFFFF) << 8) | bit_ctx);
      int pm = mm.Predict(bit_ctx);

      mixer.Add(p0);
      mixer.Add(p1);
      mixer.Add(p2);
      mixer.Add(p3);
      mixer.Add(p4);
      mixer.Add(pm);
      mixer.Add(2048);
      mixer.Add(2048);

      int p = mixer.Mix();

      int bit = dec.Decode(p);

      cm0.Update(bit_ctx, bit);
      cm1.Update((ctx1 << 8) | bit_ctx, bit);
      cm2.Update(((ctx2 & 0xFFF) << 8) | bit_ctx, bit);
      cm3.Update(((ctx3 & 0x3FFF) << 8) | bit_ctx, bit);
      cm4.Update(((ctx4 & 0xFFFF) << 8) | bit_ctx, bit);
      mm.Update(bit_ctx, bit, ctx1 & 0xFF);
      mixer.Update(bit);

      byte = (byte << 1) | bit;
      bit_ctx = (bit_ctx << 1) | bit;
    }

    mm.ByteDone();
    out.push_back(byte);

    ctx4 = (ctx4 << 8) | ctx3 >> 24;
    ctx3 = (ctx3 << 8) | ctx2 >> 16;
    ctx2 = (ctx2 << 8) | ctx1 >> 8;
    ctx1 = (ctx1 << 8) | byte;
  }

  return out;
}
