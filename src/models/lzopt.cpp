#include "lzopt.hpp"
#include <unordered_map>
#include <algorithm>
#include <limits>

constexpr uint8_t ESC_XLONG = 0xFD;
constexpr uint8_t ESC_SHORT = 0xFE;
constexpr uint8_t ESC_LONG = 0xFF;
constexpr int MIN_MATCH = 3;
constexpr int MAX_MATCH = 252 + MIN_MATCH;
constexpr size_t WINDOW_SIZE = 1 << 20;
constexpr int HASH_CHAIN_LEN = 256;
constexpr int NICE_MATCH = 64;

static inline uint32_t Hash4(const uint8_t* p) {
  return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
         ((uint32_t)p[2] << 8) | p[3];
}

static inline int MatchCost(size_t offset, int len) {
  if (len < MIN_MATCH) return std::numeric_limits<int>::max();
  if (offset < 256) return 3;
  if (offset < 65536) return 4;
  return 5;
}

static inline int LiteralCost(uint8_t b) {
  return (b >= ESC_XLONG) ? 2 : 1;
}

struct Match {
  int len;
  size_t offset;
};

static std::vector<Match> FindMatches(
    const std::vector<uint8_t>& in, size_t pos,
    const std::unordered_map<uint32_t, std::vector<size_t>>& hash_table) {

  std::vector<Match> matches;
  if (pos + 3 >= in.size()) return matches;

  uint32_t h = Hash4(&in[pos]);
  auto it = hash_table.find(h);
  if (it == hash_table.end()) return matches;

  int best_len_at_cost[6] = {0};

  int checked = 0;
  for (auto pit = it->second.rbegin();
       pit != it->second.rend() && checked < HASH_CHAIN_LEN;
       ++pit, ++checked) {
    size_t match_pos = *pit;
    if (match_pos >= pos) continue;
    size_t dist = pos - match_pos;
    if (dist > WINDOW_SIZE || dist == 0) break;

    int len = 0;
    size_t max_len = std::min((size_t)MAX_MATCH, in.size() - pos);
    while (len < (int)max_len && in[match_pos + len] == in[pos + len]) {
      len++;
    }

    if (len >= MIN_MATCH) {
      int cost = MatchCost(dist, len);
      if (len > best_len_at_cost[cost]) {
        best_len_at_cost[cost] = len;
        matches.push_back({len, dist});
      }
      if (len >= NICE_MATCH) break;
    }
  }

  return matches;
}

static std::vector<uint8_t> LZOptGreedy(const std::vector<uint8_t> &in) {
  std::vector<uint8_t> out;
  out.reserve(in.size());
  size_t n = in.size();

  std::unordered_map<uint32_t, std::vector<size_t>> hash_table;
  hash_table.reserve(std::min(n, (size_t)65536));

  for (size_t i = 0; i < n;) {
    auto matches = FindMatches(in, i, hash_table);

    if (i + 3 < n) {
      uint32_t h = Hash4(&in[i]);
      hash_table[h].push_back(i);
      if (hash_table[h].size() > HASH_CHAIN_LEN) {
        hash_table[h].erase(hash_table[h].begin());
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
        out.push_back(ESC_SHORT);
        out.push_back((uint8_t)(best_match.len - MIN_MATCH));
        out.push_back((uint8_t)best_match.offset);
      } else if (best_match.offset < 65536) {
        out.push_back(ESC_LONG);
        out.push_back((uint8_t)(best_match.len - MIN_MATCH));
        out.push_back((uint8_t)(best_match.offset >> 8));
        out.push_back((uint8_t)(best_match.offset & 0xFF));
      } else {
        out.push_back(ESC_XLONG);
        out.push_back((uint8_t)(best_match.len - MIN_MATCH));
        out.push_back((uint8_t)(best_match.offset >> 16));
        out.push_back((uint8_t)((best_match.offset >> 8) & 0xFF));
        out.push_back((uint8_t)(best_match.offset & 0xFF));
      }

      for (int j = 1; j < best_match.len && i + j + 3 < n; j++) {
        uint32_t h = Hash4(&in[i + j]);
        hash_table[h].push_back(i + j);
        if (hash_table[h].size() > HASH_CHAIN_LEN) {
          hash_table[h].erase(hash_table[h].begin());
        }
      }
      i += best_match.len;
    } else {
      uint8_t b = in[i];
      if (b >= ESC_XLONG) {
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

std::vector<uint8_t> LZOptCompress(const std::vector<uint8_t> &in) {
  if (in.empty()) return {};

  size_t n = in.size();

  if (n > 256 * 1024) {
    return LZOptGreedy(in);
  }

  std::unordered_map<uint32_t, std::vector<size_t>> hash_table;
  hash_table.reserve(std::min(n, (size_t)65536));
  for (size_t i = 0; i + 3 < n; i++) {
    uint32_t h = Hash4(&in[i]);
    hash_table[h].push_back(i);
    if (hash_table[h].size() > HASH_CHAIN_LEN * 2) {
      hash_table[h].erase(hash_table[h].begin(),
                          hash_table[h].begin() + HASH_CHAIN_LEN);
    }
  }

  std::vector<int> cost(n + 1, std::numeric_limits<int>::max());
  std::vector<int> choice_len(n + 1, 0);
  std::vector<size_t> choice_off(n + 1, 0);
  cost[0] = 0;

  for (size_t i = 0; i < n; i++) {
    if (cost[i] == std::numeric_limits<int>::max()) continue;

    int lit_cost = LiteralCost(in[i]);
    if (cost[i] + lit_cost < cost[i + 1]) {
      cost[i + 1] = cost[i] + lit_cost;
      choice_len[i + 1] = 0;
    }

    auto matches = FindMatches(in, i, hash_table);
    for (const auto& m : matches) {
      int match_cost = MatchCost(m.offset, m.len);
      size_t end = i + m.len;
      if (end <= n && cost[i] + match_cost < cost[end]) {
        cost[end] = cost[i] + match_cost;
        choice_len[end] = m.len;
        choice_off[end] = m.offset;
      }
    }
  }

  std::vector<uint8_t> out;
  out.reserve(cost[n]);

  size_t i = n;
  std::vector<std::pair<size_t, std::pair<int, size_t>>> ops;

  while (i > 0) {
    if (choice_len[i] == 0) {
      ops.push_back({i - 1, {0, 0}});
      i--;
    } else {
      int len = choice_len[i];
      size_t off = choice_off[i];
      ops.push_back({i - len, {len, off}});
      i -= len;
    }
  }

  std::reverse(ops.begin(), ops.end());
  for (const auto& op : ops) {
    size_t pos = op.first;
    int len = op.second.first;
    size_t off = op.second.second;

    if (len == 0) {
      uint8_t b = in[pos];
      if (b >= ESC_XLONG) {
        out.push_back(b);
        out.push_back(b);
      } else {
        out.push_back(b);
      }
    } else {
      if (off < 256) {
        out.push_back(ESC_SHORT);
        out.push_back((uint8_t)(len - MIN_MATCH));
        out.push_back((uint8_t)off);
      } else if (off < 65536) {
        out.push_back(ESC_LONG);
        out.push_back((uint8_t)(len - MIN_MATCH));
        out.push_back((uint8_t)(off >> 8));
        out.push_back((uint8_t)(off & 0xFF));
      } else {
        out.push_back(ESC_XLONG);
        out.push_back((uint8_t)(len - MIN_MATCH));
        out.push_back((uint8_t)(off >> 16));
        out.push_back((uint8_t)((off >> 8) & 0xFF));
        out.push_back((uint8_t)(off & 0xFF));
      }
    }
  }

  return out;
}

std::vector<uint8_t> LZOptDecompress(const std::vector<uint8_t> &in) {
  std::vector<uint8_t> out;
  out.reserve(in.size() * 3);

  for (size_t i = 0; i < in.size();) {
    if (in[i] == ESC_SHORT) {
      if (i + 1 >= in.size()) break;

      if (in[i + 1] == ESC_SHORT) {
        out.push_back(ESC_SHORT);
        i += 2;
      } else {
        if (i + 2 >= in.size()) break;
        int len = in[i + 1] + MIN_MATCH;
        size_t offset = in[i + 2];
        if (offset > out.size() || offset == 0) break;
        size_t pos = out.size() - offset;
        for (int j = 0; j < len; j++) {
          out.push_back(out[pos + j]);
        }
        i += 3;
      }
    } else if (in[i] == ESC_LONG) {
      if (i + 1 >= in.size()) break;

      if (in[i + 1] == ESC_LONG) {
        out.push_back(ESC_LONG);
        i += 2;
      } else {
        if (i + 3 >= in.size()) break;
        int len = in[i + 1] + MIN_MATCH;
        size_t offset = ((size_t)in[i + 2] << 8) | in[i + 3];
        if (offset > out.size() || offset == 0) break;
        size_t pos = out.size() - offset;
        for (int j = 0; j < len; j++) {
          out.push_back(out[pos + j]);
        }
        i += 4;
      }
    } else if (in[i] == ESC_XLONG) {
      if (i + 1 >= in.size()) break;

      if (in[i + 1] == ESC_XLONG) {
        out.push_back(ESC_XLONG);
        i += 2;
      } else {
        if (i + 4 >= in.size()) break;
        int len = in[i + 1] + MIN_MATCH;
        size_t offset = ((size_t)in[i + 2] << 16) |
                       ((size_t)in[i + 3] << 8) | in[i + 4];
        if (offset > out.size() || offset == 0) break;
        size_t pos = out.size() - offset;
        for (int j = 0; j < len; j++) {
          out.push_back(out[pos + j]);
        }
        i += 5;
      }
    } else {
      out.push_back(in[i]);
      i++;
    }
  }

  return out;
}
