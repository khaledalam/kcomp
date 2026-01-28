#pragma once

#include <cstdint>
#include <vector>

// Match model: tracks recent byte occurrences and predicts next byte
// based on exact context matches
class MatchModel {
public:
  static constexpr int CONTEXT_SIZE = 8;  // Look back 8 bytes for context
  static constexpr int HASH_SIZE = 65536; // 64K hash entries

  MatchModel() : hash_table(HASH_SIZE, 0), match_length(0), match_pos(0) {}

  // Update model with current position
  void Update(const uint8_t* data, size_t pos, size_t size) {
    if (pos < CONTEXT_SIZE) return;

    uint32_t h = Hash(data + pos - CONTEXT_SIZE, CONTEXT_SIZE);
    size_t prev_pos = hash_table[h & (HASH_SIZE - 1)];

    // Check if there's a match at this position
    if (prev_pos > CONTEXT_SIZE && prev_pos < pos) {
      // Verify context actually matches
      bool matches = true;
      for (int i = 0; i < CONTEXT_SIZE && matches; i++) {
        if (data[prev_pos - CONTEXT_SIZE + i] != data[pos - CONTEXT_SIZE + i]) {
          matches = false;
        }
      }

      if (matches) {
        // Count how long the match extends
        match_pos = prev_pos;
        match_length = 0;
        while (pos + match_length < size &&
               prev_pos + match_length < pos &&
               data[prev_pos + match_length] == data[pos + match_length]) {
          match_length++;
        }
      } else {
        match_length = 0;
      }
    } else {
      match_length = 0;
    }

    // Store current position in hash table
    hash_table[h & (HASH_SIZE - 1)] = pos;
  }

  // Get predicted byte (if in match) and match probability
  bool GetPrediction(const uint8_t* data, size_t pos,
                     uint8_t& predicted, int& confidence) const {
    if (match_length > 0 && match_pos + match_length <= pos) {
      // We're currently in a match - predict next byte
      predicted = data[match_pos + match_length];
      confidence = std::min(match_length * 10, 200); // Higher confidence for longer matches
      return true;
    }
    return false;
  }

  // Reset state
  void Reset() {
    std::fill(hash_table.begin(), hash_table.end(), 0);
    match_length = 0;
    match_pos = 0;
  }

private:
  std::vector<size_t> hash_table;
  int match_length;
  size_t match_pos;

  static uint32_t Hash(const uint8_t* p, int len) {
    uint32_t h = 0x811c9dc5; // FNV-1a
    for (int i = 0; i < len; i++) {
      h ^= p[i];
      h *= 0x01000193;
    }
    return h;
  }
};
