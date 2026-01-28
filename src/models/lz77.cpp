#include "lz77.hpp"
#include <unordered_map>
#include <algorithm>

constexpr uint8_t ESC_SHORT = 0xFE;
constexpr uint8_t ESC_LONG = 0xFF;
constexpr int MIN_MATCH = 3;
constexpr int MAX_MATCH = 253 + MIN_MATCH;
constexpr int WINDOW_SIZE = 65536;
constexpr int HASH_CHAIN_LEN = 64;

static inline uint32_t Hash4(const uint8_t* p) {
  return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
         ((uint32_t)p[2] << 8) | p[3];
}

static int FindBestMatch(const std::vector<uint8_t>& in, size_t pos,
                         const std::unordered_map<uint32_t, std::vector<size_t>>& hash_table,
                         size_t& best_off) {
  if (pos + 3 >= in.size()) return 0;

  uint32_t h = Hash4(&in[pos]);
  auto it = hash_table.find(h);
  if (it == hash_table.end()) return 0;

  int best_len = 0;
  best_off = 0;

  int checked = 0;
  for (auto pit = it->second.rbegin(); pit != it->second.rend() && checked < HASH_CHAIN_LEN; ++pit, ++checked) {
    size_t match_pos = *pit;
    if (pos - match_pos > WINDOW_SIZE) break;

    int len = 0;
    size_t max_len = std::min((size_t)MAX_MATCH, in.size() - pos);
    while (len < (int)max_len && in[match_pos + len] == in[pos + len]) {
      len++;
    }

    if (len >= MIN_MATCH && len > best_len) {
      best_len = len;
      best_off = pos - match_pos;
      if (len >= 32) break;
    }
  }

  return best_len;
}

std::vector<uint8_t> LZ77Compress(const std::vector<uint8_t> &in) {
  std::vector<uint8_t> out;
  out.reserve(in.size());

  std::unordered_map<uint32_t, std::vector<size_t>> hash_table;

  for (size_t i = 0; i < in.size();) {
    size_t best_off = 0;
    int best_len = FindBestMatch(in, i, hash_table, best_off);

    if (best_len >= MIN_MATCH && best_len < MAX_MATCH && i + 1 < in.size()) {
      size_t next_off = 0;
      int next_len = FindBestMatch(in, i + 1, hash_table, next_off);

      if (next_len > best_len + 1) {
        if (in[i] == ESC_SHORT) {
          out.push_back(ESC_SHORT);
          out.push_back(ESC_SHORT);
        } else if (in[i] == ESC_LONG) {
          out.push_back(ESC_LONG);
          out.push_back(ESC_LONG);
        } else {
          out.push_back(in[i]);
        }

        if (i + 3 < in.size()) {
          uint32_t h = Hash4(&in[i]);
          hash_table[h].push_back(i);
          if (hash_table[h].size() > HASH_CHAIN_LEN) {
            hash_table[h].erase(hash_table[h].begin());
          }
        }
        i++;
        continue;
      }
    }

    if (best_len >= MIN_MATCH) {
      if (best_off < 256) {
        out.push_back(ESC_SHORT);
        out.push_back((uint8_t)(best_len - MIN_MATCH));
        out.push_back((uint8_t)best_off);
      } else {
        out.push_back(ESC_LONG);
        out.push_back((uint8_t)(best_len - MIN_MATCH));
        out.push_back((uint8_t)(best_off >> 8));
        out.push_back((uint8_t)(best_off & 0xFF));
      }

      for (int j = 0; j < best_len && i + j + 3 < in.size(); j++) {
        uint32_t h = Hash4(&in[i + j]);
        hash_table[h].push_back(i + j);
        if (hash_table[h].size() > HASH_CHAIN_LEN) {
          hash_table[h].erase(hash_table[h].begin());
        }
      }

      i += best_len;
    } else {
      if (in[i] == ESC_SHORT) {
        out.push_back(ESC_SHORT);
        out.push_back(ESC_SHORT);
      } else if (in[i] == ESC_LONG) {
        out.push_back(ESC_LONG);
        out.push_back(ESC_LONG);
      } else {
        out.push_back(in[i]);
      }

      if (i + 3 < in.size()) {
        uint32_t h = Hash4(&in[i]);
        hash_table[h].push_back(i);
        if (hash_table[h].size() > HASH_CHAIN_LEN) {
          hash_table[h].erase(hash_table[h].begin());
        }
      }
      i++;
    }
  }

  return out;
}

std::vector<uint8_t> LZ77Decompress(const std::vector<uint8_t> &in) {
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
    } else {
      out.push_back(in[i]);
      i++;
    }
  }

  return out;
}

constexpr uint8_t RLE_ESC = 0xFF;
constexpr int RLE_MIN_RUN = 4;
constexpr int RLE_MAX_RUN = 259;

std::vector<uint8_t> RLECompress(const std::vector<uint8_t> &in) {
  std::vector<uint8_t> out;
  out.reserve(in.size());

  for (size_t i = 0; i < in.size();) {
    uint8_t byte = in[i];
    size_t run = 1;
    while (i + run < in.size() && in[i + run] == byte && run < RLE_MAX_RUN) {
      run++;
    }

    if (run >= RLE_MIN_RUN) {
      out.push_back(RLE_ESC);
      out.push_back(byte);
      out.push_back((uint8_t)(run - RLE_MIN_RUN));
      i += run;
    } else {
      if (byte == RLE_ESC) {
        out.push_back(RLE_ESC);
        out.push_back(RLE_ESC);
      } else {
        out.push_back(byte);
      }
      i++;
    }
  }

  return out;
}

std::vector<uint8_t> RLEDecompress(const std::vector<uint8_t> &in) {
  std::vector<uint8_t> out;
  out.reserve(in.size() * 2);

  for (size_t i = 0; i < in.size();) {
    if (in[i] == RLE_ESC) {
      if (i + 1 >= in.size()) break;

      if (in[i + 1] == RLE_ESC) {
        out.push_back(RLE_ESC);
        i += 2;
      } else {
        if (i + 2 >= in.size()) break;
        uint8_t byte = in[i + 1];
        size_t len = in[i + 2] + RLE_MIN_RUN;
        for (size_t j = 0; j < len; j++) {
          out.push_back(byte);
        }
        i += 3;
      }
    } else {
      out.push_back(in[i]);
      i++;
    }
  }

  return out;
}

std::vector<uint8_t> DeltaEncode(const std::vector<uint8_t> &in) {
  if (in.empty()) return {};

  std::vector<uint8_t> out;
  out.reserve(in.size());

  out.push_back(in[0]);
  for (size_t i = 1; i < in.size(); i++) {
    out.push_back((uint8_t)(in[i] - in[i - 1]));
  }

  return out;
}

std::vector<uint8_t> DeltaDecode(const std::vector<uint8_t> &in) {
  if (in.empty()) return {};

  std::vector<uint8_t> out;
  out.reserve(in.size());

  out.push_back(in[0]);
  for (size_t i = 1; i < in.size(); i++) {
    out.push_back((uint8_t)(out[i - 1] + in[i]));
  }

  return out;
}

std::vector<uint8_t> PatternEncode(const std::vector<uint8_t> &in) {
  if (in.size() < 8) return {};

  for (size_t plen = 1; plen <= 255 && plen <= in.size() / 2; plen++) {
    size_t matching_bytes = 0;
    for (size_t i = plen; i < in.size(); i++) {
      if (in[i] == in[i % plen]) {
        matching_bytes++;
      } else {
        break;
      }
    }

    size_t total_matched = plen + matching_bytes;
    size_t full_repeats = total_matched / plen;
    size_t trailing = in.size() - (full_repeats * plen);

    size_t result_size = 1 + plen + 4 + 1 + trailing;
    if (full_repeats >= 4 && trailing <= 255 && result_size < in.size()) {
      std::vector<uint8_t> out;
      out.reserve(result_size);

      out.push_back((uint8_t)plen);
      for (size_t i = 0; i < plen; i++) {
        out.push_back(in[i]);
      }

      uint32_t rep_count = (uint32_t)full_repeats;
      out.push_back(rep_count & 0xFF);
      out.push_back((rep_count >> 8) & 0xFF);
      out.push_back((rep_count >> 16) & 0xFF);
      out.push_back((rep_count >> 24) & 0xFF);

      out.push_back((uint8_t)trailing);
      for (size_t i = full_repeats * plen; i < in.size(); i++) {
        out.push_back(in[i]);
      }

      return out;
    }
  }

  return {};
}

std::vector<uint8_t> PatternDecode(const std::vector<uint8_t> &in) {
  if (in.size() < 7) return {};

  size_t plen = in[0];
  if (plen == 0 || in.size() < 1 + plen + 5) return {};

  uint32_t rep_count = in[1 + plen] |
                       ((uint32_t)in[2 + plen] << 8) |
                       ((uint32_t)in[3 + plen] << 16) |
                       ((uint32_t)in[4 + plen] << 24);

  size_t trailing_len = in[5 + plen];
  if (in.size() < 1 + plen + 5 + trailing_len) return {};

  std::vector<uint8_t> out;
  out.reserve(rep_count * plen + trailing_len);

  for (uint32_t r = 0; r < rep_count; r++) {
    for (size_t i = 0; i < plen; i++) {
      out.push_back(in[1 + i]);
    }
  }

  for (size_t i = 0; i < trailing_len; i++) {
    out.push_back(in[6 + plen + i]);
  }

  return out;
}

namespace {
const char* WORD_DICT[] = {
  "the ", "The ", " the ", " and ", " of ", " to ", " in ", " is ",
  " a ", "this ", "for ", "with ", " or ", " be ", " as ", " on ",
  " at ", " by ", " an ", "that ", " it ", " are ", " was ", " not ",
  "  ", "   ", "    ", "\n  ", "\n    ", "\r\n", "\n",
  "</", "/>", "=\"", "\">", "'>", "\":", "\": ", "\",", "\"}", "\"]",
  "return ", "void ", "int ", "if (", "else ", "for (", "while (",
  "function", "class ", "const ", "static ", "public ", "private ",
  "true", "false", "null", "new ", "var ", "let ",
  "http://", "https://", ".com", ".org",
  "ing ", "tion", "ment", "ness",
  nullptr
};

int MatchWord(const uint8_t* data, size_t remaining) {
  for (int i = 0; WORD_DICT[i] != nullptr && i < 127; i++) {
    const char* word = WORD_DICT[i];
    size_t wlen = strlen(word);
    if (wlen <= remaining) {
      bool match = true;
      for (size_t j = 0; j < wlen && match; j++) {
        if (data[j] != (uint8_t)word[j]) match = false;
      }
      if (match) return i;
    }
  }
  return -1;
}
}

constexpr uint8_t WORD_ESC = 0x7F;

std::vector<uint8_t> WordEncode(const std::vector<uint8_t> &in) {
  std::vector<uint8_t> out;
  out.reserve(in.size());

  for (size_t i = 0; i < in.size();) {
    int word_idx = MatchWord(&in[i], in.size() - i);
    if (word_idx >= 0) {
      out.push_back(0x80 | word_idx);
      i += strlen(WORD_DICT[word_idx]);
    } else if (in[i] >= 0x80 || in[i] == WORD_ESC) {
      out.push_back(WORD_ESC);
      out.push_back(in[i]);
      i++;
    } else {
      out.push_back(in[i]);
      i++;
    }
  }

  return out;
}

std::vector<uint8_t> WordDecode(const std::vector<uint8_t> &in) {
  std::vector<uint8_t> out;
  out.reserve(in.size() * 2);

  for (size_t i = 0; i < in.size();) {
    if (in[i] == WORD_ESC && i + 1 < in.size()) {
      out.push_back(in[i + 1]);
      i += 2;
    } else if (in[i] >= 0x80) {
      int idx = in[i] & 0x7F;
      if (idx < 127 && WORD_DICT[idx] != nullptr) {
        const char* word = WORD_DICT[idx];
        while (*word) {
          out.push_back((uint8_t)*word++);
        }
      }
      i++;
    } else {
      out.push_back(in[i]);
      i++;
    }
  }

  return out;
}

std::vector<uint8_t> RecordInterleave(const std::vector<uint8_t> &in, uint16_t record_size) {
  if (in.empty() || record_size == 0) return in;

  std::vector<uint8_t> out;
  out.reserve(2 + in.size());

  out.push_back((uint8_t)(record_size >> 8));
  out.push_back((uint8_t)(record_size & 0xFF));

  size_t num_records = (in.size() + record_size - 1) / record_size;

  for (size_t pos = 0; pos < record_size; pos++) {
    for (size_t rec = 0; rec < num_records; rec++) {
      size_t idx = rec * record_size + pos;
      if (idx < in.size()) {
        out.push_back(in[idx]);
      }
    }
  }

  return out;
}

std::vector<uint8_t> RecordDeinterleave(const std::vector<uint8_t> &in) {
  if (in.size() < 2) return in;

  uint16_t record_size = ((uint16_t)in[0] << 8) | in[1];
  if (record_size == 0) return {};

  size_t data_size = in.size() - 2;
  size_t num_records = (data_size + record_size - 1) / record_size;

  std::vector<uint8_t> out;
  out.reserve(data_size);

  for (size_t rec = 0; rec < num_records; rec++) {
    for (size_t pos = 0; pos < record_size; pos++) {
      size_t idx = pos * num_records + rec + 2;
      if (idx < in.size() && out.size() < data_size) {
        out.push_back(in[idx]);
      }
    }
  }

  return out;
}

constexpr uint8_t SPARSE_ESC = 0xFF;
constexpr int SPARSE_MIN_ZEROS = 4;
constexpr int SPARSE_MAX_ZEROS = 65535 + SPARSE_MIN_ZEROS;

std::vector<uint8_t> SparseEncode(const std::vector<uint8_t> &in) {
  std::vector<uint8_t> out;
  out.reserve(in.size());

  for (size_t i = 0; i < in.size();) {
    if (in[i] == 0) {
      size_t run = 1;
      while (i + run < in.size() && in[i + run] == 0 && run < SPARSE_MAX_ZEROS) {
        run++;
      }

      if (run >= SPARSE_MIN_ZEROS) {
        out.push_back(SPARSE_ESC);
        out.push_back(0x00);
        uint16_t encoded_len = (uint16_t)(run - SPARSE_MIN_ZEROS);
        out.push_back((uint8_t)(encoded_len >> 8));
        out.push_back((uint8_t)(encoded_len & 0xFF));
        i += run;
      } else {
        for (size_t j = 0; j < run; j++) {
          out.push_back(0x00);
        }
        i += run;
      }
    } else if (in[i] == SPARSE_ESC) {
      out.push_back(SPARSE_ESC);
      out.push_back(SPARSE_ESC);
      i++;
    } else {
      out.push_back(in[i]);
      i++;
    }
  }

  return out;
}

std::vector<uint8_t> SparseDecode(const std::vector<uint8_t> &in) {
  std::vector<uint8_t> out;
  out.reserve(in.size() * 2);

  for (size_t i = 0; i < in.size();) {
    if (in[i] == SPARSE_ESC) {
      if (i + 1 >= in.size()) break;

      if (in[i + 1] == SPARSE_ESC) {
        out.push_back(SPARSE_ESC);
        i += 2;
      } else if (in[i + 1] == 0x00) {
        if (i + 3 >= in.size()) break;
        uint16_t encoded_len = ((uint16_t)in[i + 2] << 8) | in[i + 3];
        size_t run = encoded_len + SPARSE_MIN_ZEROS;
        for (size_t j = 0; j < run; j++) {
          out.push_back(0x00);
        }
        i += 4;
      } else {
        out.push_back(in[i]);
        i++;
      }
    } else {
      out.push_back(in[i]);
      i++;
    }
  }

  return out;
}
