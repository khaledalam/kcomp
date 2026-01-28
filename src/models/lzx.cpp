#include "lzx.hpp"
#include <algorithm>
#include <numeric>
#include <unordered_map>
#include <limits>

namespace {

constexpr uint8_t ESC_TINY = 0xFC;
constexpr uint8_t ESC_SHORT = 0xFD;
constexpr uint8_t ESC_MED = 0xFE;
constexpr uint8_t ESC_LONG = 0xFF;
constexpr int MIN_MATCH = 3;
constexpr int MAX_MATCH = 251 + MIN_MATCH;
constexpr size_t WINDOW_SIZE = 64ULL << 20;

inline int MatchCost(size_t offset, int len) {
  if (len < MIN_MATCH) return 999999;
  if (offset < 256) return 3;
  if (offset < 65536) return 4;
  if (offset < (1 << 24)) return 5;
  return 6;
}

inline int LiteralCost(uint8_t b) {
  return (b >= ESC_TINY) ? 2 : 1;
}

class SuffixArray {
public:
  std::vector<int> sa;
  std::vector<int> rank;
  std::vector<int> lcp;
  std::vector<int> inv;

  void Build(const std::vector<uint8_t>& text) {
    size_t n = text.size();
    if (n == 0) return;

    sa.resize(n);
    rank.resize(n);
    inv.resize(n);

    std::iota(sa.begin(), sa.end(), 0);
    for (size_t i = 0; i < n; i++) {
      rank[i] = text[i];
    }

    for (size_t k = 1; k < n; k *= 2) {
      std::vector<std::pair<std::pair<int, int>, int>> pairs(n);
      for (size_t i = 0; i < n; i++) {
        int r1 = rank[i];
        int r2 = (i + k < n) ? rank[i + k] : -1;
        pairs[i] = {{r1, r2}, (int)i};
      }
      std::sort(pairs.begin(), pairs.end());

      for (size_t i = 0; i < n; i++) {
        sa[i] = pairs[i].second;
      }

      rank[sa[0]] = 0;
      for (size_t i = 1; i < n; i++) {
        if (pairs[i].first == pairs[i - 1].first) {
          rank[sa[i]] = rank[sa[i - 1]];
        } else {
          rank[sa[i]] = rank[sa[i - 1]] + 1;
        }
      }

      if (rank[sa[n - 1]] == (int)n - 1) break;
    }

    for (size_t i = 0; i < n; i++) {
      inv[sa[i]] = i;
    }

    lcp.resize(n);
    int h = 0;
    for (size_t i = 0; i < n; i++) {
      if (inv[i] > 0) {
        int j = sa[inv[i] - 1];
        while (i + h < n && j + h < n && text[i + h] == text[j + h]) {
          h++;
        }
        lcp[inv[i]] = h;
        if (h > 0) h--;
      }
    }
  }
};

struct Match {
  int len;
  size_t offset;
};

std::vector<Match> FindMatchesSA(
    const std::vector<uint8_t>& text,
    size_t pos,
    const SuffixArray& sa_obj) {

  std::vector<Match> matches;
  size_t n = text.size();
  if (pos >= n || sa_obj.sa.empty()) return matches;

  int rank_pos = sa_obj.inv[pos];

  auto try_match = [&](int sa_idx) {
    if (sa_idx < 0 || sa_idx >= (int)n) return;
    size_t match_pos = sa_obj.sa[sa_idx];
    if (match_pos >= pos) return;

    size_t dist = pos - match_pos;
    if (dist > WINDOW_SIZE || dist == 0) return;

    int len = 0;
    size_t max_len = std::min((size_t)MAX_MATCH, n - pos);
    while (len < (int)max_len && text[match_pos + len] == text[pos + len]) {
      len++;
    }

    if (len >= MIN_MATCH) {
      matches.push_back({len, dist});
    }
  };

  int search_range = std::min(128, (int)n);
  for (int delta = 1; delta < search_range; delta++) {
    try_match(rank_pos - delta);
    try_match(rank_pos + delta);

    if (matches.size() >= 8) break;
  }

  return matches;
}

static inline uint32_t Hash4LZX(const uint8_t* p) {
  return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
         ((uint32_t)p[2] << 8) | p[3];
}

std::vector<Match> FindMatchesHash(
    const std::vector<uint8_t>& text, size_t pos,
    std::unordered_map<uint32_t, std::vector<size_t>>& hash_table) {

  std::vector<Match> matches;
  if (pos + 3 >= text.size()) return matches;

  uint32_t h = Hash4LZX(&text[pos]);
  auto it = hash_table.find(h);
  if (it == hash_table.end()) return matches;

  constexpr int CHAIN_LEN = 128;
  int checked = 0;

  for (auto pit = it->second.rbegin();
       pit != it->second.rend() && checked < CHAIN_LEN;
       ++pit, ++checked) {
    size_t match_pos = *pit;
    if (match_pos >= pos) continue;
    size_t dist = pos - match_pos;
    if (dist > WINDOW_SIZE || dist == 0) break;

    int len = 0;
    size_t max_len = std::min((size_t)MAX_MATCH, text.size() - pos);
    while (len < (int)max_len && text[match_pos + len] == text[pos + len]) {
      len++;
    }

    if (len >= MIN_MATCH) {
      matches.push_back({len, dist});
      if (len >= 64) break;
    }
  }

  return matches;
}

}

std::vector<uint8_t> LZXCompress(const std::vector<uint8_t>& in) {
  if (in.empty()) return {};

  size_t n = in.size();
  std::vector<uint8_t> out;
  out.reserve(n);

  constexpr size_t MAX_SA_SIZE = 256 * 1024;
  bool use_suffix_array = (n <= MAX_SA_SIZE);

  SuffixArray sa_obj;
  std::unordered_map<uint32_t, std::vector<size_t>> hash_table;

  if (use_suffix_array) {
    sa_obj.Build(in);
  } else {
    hash_table.reserve(std::min(n, (size_t)65536));
  }

  for (size_t i = 0; i < n;) {
    std::vector<Match> matches;

    if (use_suffix_array) {
      matches = FindMatchesSA(in, i, sa_obj);
    } else {
      matches = FindMatchesHash(in, i, hash_table);
      if (i + 3 < n) {
        uint32_t h = Hash4LZX(&in[i]);
        hash_table[h].push_back(i);
        if (hash_table[h].size() > 256) {
          hash_table[h].erase(hash_table[h].begin());
        }
      }
    }

    int best_savings = 0;
    Match best_match = {0, 0};

    for (const auto& m : matches) {
      int savings = m.len - MatchCost(m.offset, m.len);
      if (savings > best_savings) {
        best_savings = savings;
        best_match = m;
      }
    }

    if (best_savings > 0) {
      if (best_match.offset < 256) {
        out.push_back(ESC_TINY);
        out.push_back((uint8_t)(best_match.len - MIN_MATCH));
        out.push_back((uint8_t)best_match.offset);
      } else if (best_match.offset < 65536) {
        out.push_back(ESC_SHORT);
        out.push_back((uint8_t)(best_match.len - MIN_MATCH));
        out.push_back((uint8_t)(best_match.offset >> 8));
        out.push_back((uint8_t)(best_match.offset & 0xFF));
      } else if (best_match.offset < (1 << 24)) {
        out.push_back(ESC_MED);
        out.push_back((uint8_t)(best_match.len - MIN_MATCH));
        out.push_back((uint8_t)(best_match.offset >> 16));
        out.push_back((uint8_t)((best_match.offset >> 8) & 0xFF));
        out.push_back((uint8_t)(best_match.offset & 0xFF));
      } else {
        out.push_back(ESC_LONG);
        out.push_back((uint8_t)(best_match.len - MIN_MATCH));
        out.push_back((uint8_t)(best_match.offset >> 24));
        out.push_back((uint8_t)((best_match.offset >> 16) & 0xFF));
        out.push_back((uint8_t)((best_match.offset >> 8) & 0xFF));
        out.push_back((uint8_t)(best_match.offset & 0xFF));
      }
      i += best_match.len;
    } else {
      uint8_t b = in[i];
      if (b >= ESC_TINY) {
        out.push_back(b);
        out.push_back(b);
      } else {
        out.push_back(b);
      }
      i++;
    }
  }

  return out;
}

std::vector<uint8_t> LZXDecompress(const std::vector<uint8_t>& in) {
  std::vector<uint8_t> out;
  out.reserve(in.size() * 3);

  for (size_t i = 0; i < in.size();) {
    if (in[i] == ESC_TINY) {
      if (i + 1 >= in.size()) break;
      if (in[i + 1] == ESC_TINY) {
        out.push_back(ESC_TINY);
        i += 2;
      } else {
        if (i + 2 >= in.size()) break;
        int len = in[i + 1] + MIN_MATCH;
        size_t offset = in[i + 2];
        if (offset > out.size() || offset == 0) break;
        size_t pos = out.size() - offset;
        for (int j = 0; j < len; j++) out.push_back(out[pos + j]);
        i += 3;
      }
    } else if (in[i] == ESC_SHORT) {
      if (i + 1 >= in.size()) break;
      if (in[i + 1] == ESC_SHORT) {
        out.push_back(ESC_SHORT);
        i += 2;
      } else {
        if (i + 3 >= in.size()) break;
        int len = in[i + 1] + MIN_MATCH;
        size_t offset = ((size_t)in[i + 2] << 8) | in[i + 3];
        if (offset > out.size() || offset == 0) break;
        size_t pos = out.size() - offset;
        for (int j = 0; j < len; j++) out.push_back(out[pos + j]);
        i += 4;
      }
    } else if (in[i] == ESC_MED) {
      if (i + 1 >= in.size()) break;
      if (in[i + 1] == ESC_MED) {
        out.push_back(ESC_MED);
        i += 2;
      } else {
        if (i + 4 >= in.size()) break;
        int len = in[i + 1] + MIN_MATCH;
        size_t offset = ((size_t)in[i + 2] << 16) |
                       ((size_t)in[i + 3] << 8) | in[i + 4];
        if (offset > out.size() || offset == 0) break;
        size_t pos = out.size() - offset;
        for (int j = 0; j < len; j++) out.push_back(out[pos + j]);
        i += 5;
      }
    } else if (in[i] == ESC_LONG) {
      if (i + 1 >= in.size()) break;
      if (in[i + 1] == ESC_LONG) {
        out.push_back(ESC_LONG);
        i += 2;
      } else {
        if (i + 5 >= in.size()) break;
        int len = in[i + 1] + MIN_MATCH;
        size_t offset = ((size_t)in[i + 2] << 24) |
                       ((size_t)in[i + 3] << 16) |
                       ((size_t)in[i + 4] << 8) | in[i + 5];
        if (offset > out.size() || offset == 0) break;
        size_t pos = out.size() - offset;
        for (int j = 0; j < len; j++) out.push_back(out[pos + j]);
        i += 6;
      }
    } else {
      out.push_back(in[i]);
      i++;
    }
  }

  return out;
}
