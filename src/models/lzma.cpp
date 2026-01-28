#include "lzma.hpp"
#include <algorithm>
#include <cstring>
#include <unordered_map>
#include <limits>

// Match encoding format:
// Literal: 0x00 byte (if byte < 0x80)
// Literal escape: 0x80 byte (if byte >= 0x80)
// Short match (len 3-18, off 1-256): 0x81 + (len-3) | (off-1)<<4
// Medium match (len 3-34, off 1-65536): 0x92 len off_hi off_lo
// Long match (len 3-273, off 1-1M): 0xB3 len_lo len_hi off_0 off_1 off_2

namespace {

constexpr uint8_t LIT_DIRECT = 0x00;    // Literal byte < 0x80
constexpr uint8_t LIT_ESCAPE = 0x80;    // Literal byte >= 0x80
constexpr uint8_t MATCH_SHORT = 0x81;   // 0x81-0x91: short match
constexpr uint8_t MATCH_MEDIUM = 0x92;  // Medium match
constexpr uint8_t MATCH_LONG = 0xB3;    // Long match

constexpr int HASH_BITS = 20;  // 1M hash entries
constexpr int HASH_SIZE = 1 << HASH_BITS;
constexpr int MAX_CHAIN = 256;  // Maximum chain length for match finding

// Calculate hash from 4 bytes
inline uint32_t Hash4(const uint8_t* p) {
  return (((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
          ((uint32_t)p[2] << 8) | p[3]) * 2654435761u >> (32 - HASH_BITS);
}

// Cost in bits for encoding a literal
inline int LiteralCost(uint8_t byte) {
  return byte < 0x80 ? 16 : 24;  // 2 bytes or 3 bytes
}

// Cost in bits for encoding a match
inline int MatchCost(int len, size_t offset) {
  if (len >= 3 && len <= 18 && offset >= 1 && offset <= 256) {
    return 16;  // Short match: 2 bytes
  } else if (len >= 3 && len <= 34 && offset <= 65536) {
    return 32;  // Medium match: 4 bytes
  } else {
    return 48;  // Long match: 6 bytes
  }
}

// Find best match at position i using hash chains
struct Match {
  int len;
  size_t off;
};

Match FindBestMatch(const std::vector<uint8_t>& in, size_t i,
                    const std::vector<int>& head,
                    const std::vector<int>& prev) {
  Match best = {0, 0};
  if (i + LZMA_MIN_MATCH > in.size()) return best;

  uint32_t h = Hash4(&in[i]);
  int pos = head[h];
  int chain_len = 0;

  while (pos >= 0 && chain_len < MAX_CHAIN) {
    // Skip positions >= current (can only match earlier positions)
    if ((size_t)pos >= i) {
      pos = prev[pos];
      continue;
    }

    size_t off = i - pos;
    if (off > LZMA_DICT_SIZE) break;

    // Find match length
    int len = 0;
    size_t max_len = std::min((size_t)LZMA_MAX_MATCH, in.size() - i);
    while (len < (int)max_len && in[pos + len] == in[i + len]) {
      len++;
    }

    // Keep if better than current best
    // Prefer shorter offsets for same length (better encoding)
    if (len >= LZMA_MIN_MATCH) {
      int new_cost = MatchCost(len, off);
      int old_cost = best.len > 0 ? MatchCost(best.len, best.off) : 0;

      // Better if: longer match, or same length but cheaper encoding
      if (len > best.len || (len == best.len && new_cost < old_cost)) {
        best.len = len;
        best.off = off;
      }
    }

    pos = prev[pos];
    chain_len++;
  }

  return best;
}

// Optimal parsing using dynamic programming
// cost[i] = minimum cost to encode bytes 0..i-1
// choice[i] = {len, off} to get to position i with minimum cost
struct Choice {
  int len;  // 0 = literal, >0 = match length
  size_t off;
};

} // namespace

std::vector<uint8_t> LZMACompress(const std::vector<uint8_t> &in) {
  if (in.empty()) return {};

  std::vector<uint8_t> out;
  out.reserve(in.size());

  // Hash chain tables
  std::vector<int> head(HASH_SIZE, -1);
  std::vector<int> prev(in.size(), -1);

  // Build hash chains
  for (size_t i = 0; i + 3 < in.size(); i++) {
    uint32_t h = Hash4(&in[i]);
    prev[i] = head[h];
    head[h] = (int)i;
  }

  // Optimal parsing: find minimum cost path
  size_t n = in.size();
  std::vector<int> cost(n + 1, std::numeric_limits<int>::max());
  std::vector<Choice> choice(n + 1);
  cost[0] = 0;

  for (size_t i = 0; i < n; i++) {
    if (cost[i] == std::numeric_limits<int>::max()) continue;

    // Option 1: Emit literal
    int lit_cost = cost[i] + LiteralCost(in[i]);
    if (lit_cost < cost[i + 1]) {
      cost[i + 1] = lit_cost;
      choice[i + 1] = {0, 0};
    }

    // Option 2: Emit match
    Match m = FindBestMatch(in, i, head, prev);
    if (m.len >= LZMA_MIN_MATCH) {
      // Try all lengths from min to best
      for (int len = LZMA_MIN_MATCH; len <= m.len; len++) {
        int match_cost = cost[i] + MatchCost(len, m.off);
        if (match_cost < cost[i + len]) {
          cost[i + len] = match_cost;
          choice[i + len] = {len, m.off};
        }
      }
    }
  }

  // Backtrack to find optimal sequence
  std::vector<Choice> seq;
  size_t pos = n;
  while (pos > 0) {
    seq.push_back(choice[pos]);
    if (choice[pos].len == 0) {
      pos--;
    } else {
      pos -= choice[pos].len;
    }
  }
  std::reverse(seq.begin(), seq.end());

  // Encode the sequence
  pos = 0;
  for (const auto& c : seq) {
    if (c.len == 0) {
      // Literal
      if (in[pos] < 0x80) {
        out.push_back(in[pos]);
      } else {
        out.push_back(LIT_ESCAPE);
        out.push_back(in[pos]);
      }
      pos++;
    } else {
      // Match
      int len = c.len;
      size_t off = c.off;

      if (len >= 3 && len <= 18 && off >= 1 && off <= 256) {
        // Short match: 2 bytes
        out.push_back(MATCH_SHORT + (len - 3));
        out.push_back((uint8_t)(off - 1));
      } else if (len >= 3 && len <= 34 && off <= 65536) {
        // Medium match: 4 bytes
        out.push_back(MATCH_MEDIUM + (len - 3));
        out.push_back((uint8_t)((off - 1) >> 8));
        out.push_back((uint8_t)((off - 1) & 0xFF));
      } else {
        // Long match: 6 bytes
        out.push_back(MATCH_LONG);
        out.push_back((uint8_t)((len - 3) & 0xFF));
        out.push_back((uint8_t)((len - 3) >> 8));
        out.push_back((uint8_t)((off - 1) & 0xFF));
        out.push_back((uint8_t)(((off - 1) >> 8) & 0xFF));
        out.push_back((uint8_t)(((off - 1) >> 16) & 0xFF));
      }
      pos += len;
    }
  }

  return out;
}

std::vector<uint8_t> LZMADecompress(const std::vector<uint8_t> &in) {
  std::vector<uint8_t> out;
  out.reserve(in.size() * 4);

  size_t i = 0;
  while (i < in.size()) {
    uint8_t b = in[i];

    if (b < 0x80) {
      // Direct literal
      out.push_back(b);
      i++;
    } else if (b == LIT_ESCAPE) {
      // Escaped literal
      if (i + 1 >= in.size()) break;
      out.push_back(in[i + 1]);
      i += 2;
    } else if (b >= MATCH_SHORT && b < MATCH_MEDIUM) {
      // Short match
      if (i + 1 >= in.size()) break;
      int len = (b - MATCH_SHORT) + 3;
      size_t off = in[i + 1] + 1;
      if (off > out.size()) break;
      size_t pos = out.size() - off;
      for (int j = 0; j < len; j++) {
        out.push_back(out[pos + j]);
      }
      i += 2;
    } else if (b >= MATCH_MEDIUM && b < MATCH_LONG) {
      // Medium match
      if (i + 2 >= in.size()) break;
      int len = (b - MATCH_MEDIUM) + 3;
      size_t off = ((size_t)in[i + 1] << 8) | in[i + 2];
      off += 1;
      if (off > out.size()) break;
      size_t pos = out.size() - off;
      for (int j = 0; j < len; j++) {
        out.push_back(out[pos + j]);
      }
      i += 3;
    } else if (b == MATCH_LONG) {
      // Long match
      if (i + 5 >= in.size()) break;
      int len = ((int)in[i + 1] | ((int)in[i + 2] << 8)) + 3;
      size_t off = (size_t)in[i + 3] | ((size_t)in[i + 4] << 8) | ((size_t)in[i + 5] << 16);
      off += 1;
      if (off > out.size()) break;
      size_t pos = out.size() - off;
      for (int j = 0; j < len; j++) {
        out.push_back(out[pos + j]);
      }
      i += 6;
    } else {
      // Unknown, treat as literal
      out.push_back(b);
      i++;
    }
  }

  return out;
}

// FSE (Finite State Entropy) implementation
// tANS-style entropy coding that's faster and sometimes better than range coding

namespace {

constexpr int FSE_TABLE_LOG = 11;  // 2048 states
constexpr int FSE_TABLE_SIZE = 1 << FSE_TABLE_LOG;
constexpr int FSE_MAX_SYMBOL = 256;

struct FSETable {
  uint16_t newState[FSE_TABLE_SIZE];  // New state after output
  uint8_t symbol[FSE_TABLE_SIZE];      // Symbol for this state
  uint8_t nbBits[FSE_TABLE_SIZE];      // Bits to output
};

// Build FSE encoding table from symbol counts
bool BuildFSETable(FSETable& table, const int* counts, int maxSymbol) {
  // Calculate normalized frequencies (sum to FSE_TABLE_SIZE)
  int total = 0;
  for (int i = 0; i <= maxSymbol; i++) {
    total += counts[i];
  }
  if (total == 0) return false;

  std::vector<int> norm(maxSymbol + 1);
  int remaining = FSE_TABLE_SIZE;
  for (int i = 0; i <= maxSymbol; i++) {
    if (counts[i] > 0) {
      norm[i] = (counts[i] * FSE_TABLE_SIZE + total / 2) / total;
      if (norm[i] < 1) norm[i] = 1;
      remaining -= norm[i];
    }
  }

  // Distribute remaining among highest count symbols
  while (remaining > 0) {
    int best = -1;
    for (int i = 0; i <= maxSymbol; i++) {
      if (counts[i] > 0 && (best < 0 || counts[i] > counts[best])) {
        best = i;
      }
    }
    if (best >= 0) {
      norm[best]++;
      remaining--;
    } else break;
  }
  while (remaining < 0) {
    int best = -1;
    for (int i = 0; i <= maxSymbol; i++) {
      if (norm[i] > 1 && (best < 0 || norm[i] > norm[best])) {
        best = i;
      }
    }
    if (best >= 0) {
      norm[best]--;
      remaining++;
    } else break;
  }

  // Build decoding table
  int pos = 0;
  for (int s = 0; s <= maxSymbol; s++) {
    for (int i = 0; i < norm[s]; i++) {
      table.symbol[pos] = s;
      // Calculate bits needed for this position
      int log2 = 0;
      int n = norm[s];
      while ((1 << log2) < n) log2++;
      table.nbBits[pos] = FSE_TABLE_LOG - log2;
      table.newState[pos] = (pos >> table.nbBits[pos]) + FSE_TABLE_SIZE - norm[s];
      pos++;
    }
  }

  return true;
}

} // namespace

std::vector<uint8_t> FSECompress(const std::vector<uint8_t> &in) {
  if (in.empty()) return {};

  // Count symbol frequencies
  int counts[FSE_MAX_SYMBOL] = {0};
  int maxSymbol = 0;
  for (uint8_t b : in) {
    counts[b]++;
    if (b > maxSymbol) maxSymbol = b;
  }

  // Output format:
  // maxSymbol (1 byte)
  // normalized counts (var bytes, delta encoded)
  // encoded data

  std::vector<uint8_t> out;
  out.reserve(in.size());

  // Write header: maxSymbol + counts
  out.push_back(maxSymbol);

  // Write counts (simple encoding: 0 = zero count, else 1-255)
  for (int i = 0; i <= maxSymbol; i++) {
    if (counts[i] == 0) {
      out.push_back(0);
    } else if (counts[i] < 256) {
      out.push_back(counts[i]);
    } else {
      // For high counts, cap at 255 (still builds valid table)
      out.push_back(255);
    }
  }

  // Build FSE table
  FSETable table;
  if (!BuildFSETable(table, counts, maxSymbol)) {
    // Fallback: store raw
    out.clear();
    out.push_back(255);  // Signal raw
    out.insert(out.end(), in.begin(), in.end());
    return out;
  }

  // Encode symbols (simplified: just store raw for now, FSE is complex)
  // In a full implementation, we'd use tANS encoding here
  // For now, this is a placeholder that just passes through
  uint32_t state = FSE_TABLE_SIZE;
  std::vector<uint8_t> bits;

  // Encode backwards for proper decoding
  for (auto it = in.rbegin(); it != in.rend(); ++it) {
    uint8_t sym = *it;
    // Find state range for this symbol
    // (simplified - full tANS would be more complex)
    bits.push_back(sym);
  }

  // Reverse to get forward order
  std::reverse(bits.begin(), bits.end());
  out.insert(out.end(), bits.begin(), bits.end());

  return out;
}

std::vector<uint8_t> FSEDecompress(const std::vector<uint8_t> &in) {
  if (in.empty()) return {};

  // Check for raw storage
  if (in[0] == 255) {
    return std::vector<uint8_t>(in.begin() + 1, in.end());
  }

  int maxSymbol = in[0];
  if (in.size() < (size_t)(maxSymbol + 2)) return {};

  // Read counts
  int counts[FSE_MAX_SYMBOL] = {0};
  for (int i = 0; i <= maxSymbol; i++) {
    counts[i] = in[1 + i];
  }

  // Decode (simplified - just pass through)
  size_t dataStart = 2 + maxSymbol;
  return std::vector<uint8_t>(in.begin() + dataStart, in.end());
}

std::vector<uint8_t> LZMAFSECompress(const std::vector<uint8_t> &in) {
  auto lzma = LZMACompress(in);
  return FSECompress(lzma);
}

std::vector<uint8_t> LZMAFSEDecompress(const std::vector<uint8_t> &in) {
  auto fse = FSEDecompress(in);
  return LZMADecompress(fse);
}
