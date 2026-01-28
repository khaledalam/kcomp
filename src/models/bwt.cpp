#include "bwt.hpp"
#include <algorithm>
#include <numeric>

namespace {

void ClassifySuffixes(const std::vector<uint8_t>& text, std::vector<bool>& types) {
  size_t n = text.size();
  types.resize(n + 1);
  types[n] = true;

  if (n == 0) return;
  types[n - 1] = false;

  for (size_t i = n - 1; i > 0; i--) {
    if (text[i - 1] < text[i]) {
      types[i - 1] = true;
    } else if (text[i - 1] > text[i]) {
      types[i - 1] = false;
    } else {
      types[i - 1] = types[i];
    }
  }
}

inline bool IsLMS(const std::vector<bool>& types, size_t i) {
  return i > 0 && types[i] && !types[i - 1];
}

void BuildSuffixArraySimple(const std::vector<uint8_t>& text, std::vector<int>& sa) {
  size_t n = text.size();
  sa.resize(n);
  std::iota(sa.begin(), sa.end(), 0);

  std::sort(sa.begin(), sa.end(), [&](int a, int b) {
    while (a < (int)n && b < (int)n) {
      if (text[a] != text[b]) return text[a] < text[b];
      a++; b++;
    }
    return a > b;
  });
}

}

std::vector<uint8_t> BWTEncode(const std::vector<uint8_t>& in, uint32_t& primary_index) {
  if (in.empty()) {
    primary_index = 0;
    return {};
  }

  size_t n = in.size();

  std::vector<int> sa;
  BuildSuffixArraySimple(in, sa);

  std::vector<uint8_t> out(n);
  primary_index = 0;

  for (size_t i = 0; i < n; i++) {
    if (sa[i] == 0) {
      primary_index = i;
      out[i] = in[n - 1];
    } else {
      out[i] = in[sa[i] - 1];
    }
  }

  return out;
}

std::vector<uint8_t> BWTDecode(const std::vector<uint8_t>& in, uint32_t primary_index) {
  if (in.empty()) return {};

  size_t n = in.size();

  std::array<int, 256> count{};
  for (uint8_t c : in) {
    count[c]++;
  }

  std::array<int, 256> first{};
  int sum = 0;
  for (int i = 0; i < 256; i++) {
    first[i] = sum;
    sum += count[i];
  }

  std::vector<int> T(n);
  std::array<int, 256> next_pos{};
  std::copy(first.begin(), first.end(), next_pos.begin());

  for (size_t i = 0; i < n; i++) {
    T[i] = next_pos[in[i]]++;
  }

  std::vector<uint8_t> out(n);
  int j = primary_index;
  for (size_t i = n; i > 0; i--) {
    out[i - 1] = in[j];
    j = T[j];
  }

  return out;
}

std::vector<uint8_t> MTFEncode(const std::vector<uint8_t>& in) {
  std::vector<uint8_t> out;
  out.reserve(in.size());

  std::array<uint8_t, 256> list;
  std::iota(list.begin(), list.end(), 0);

  for (uint8_t c : in) {
    int pos = 0;
    while (list[pos] != c) pos++;

    out.push_back(pos);

    for (int i = pos; i > 0; i--) {
      list[i] = list[i - 1];
    }
    list[0] = c;
  }

  return out;
}

std::vector<uint8_t> MTFDecode(const std::vector<uint8_t>& in) {
  std::vector<uint8_t> out;
  out.reserve(in.size());

  std::array<uint8_t, 256> list;
  std::iota(list.begin(), list.end(), 0);

  for (uint8_t pos : in) {
    uint8_t c = list[pos];
    out.push_back(c);

    for (int i = pos; i > 0; i--) {
      list[i] = list[i - 1];
    }
    list[0] = c;
  }

  return out;
}
