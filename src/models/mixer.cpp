#include "mixer.hpp"
#include <cmath>
#include <algorithm>

// Lookup tables for stretch and squash
namespace {

std::array<int, 4096> stretch_table;
std::array<int, 8192> squash_table;
bool tables_initialized = false;

void InitTables() {
  if (tables_initialized) return;

  // Initialize stretch table: p -> log(p/(1-p))
  for (int i = 0; i < 4096; i++) {
    double p = (i + 0.5) / 4096.0;
    stretch_table[i] = (int)(512.0 * std::log(p / (1.0 - p)));
  }

  // Initialize squash table: x -> 1/(1+exp(-x/512))
  for (int i = 0; i < 8192; i++) {
    int x = i - 4096;
    double p = 1.0 / (1.0 + std::exp(-x / 512.0));
    squash_table[i] = std::clamp((int)(p * 4096), 1, 4095);
  }

  tables_initialized = true;
}

}  // namespace

ContextMixer::ContextMixer() {
  InitTables();
  Reset();
}

void ContextMixer::Reset() {
  num_inputs = 0;
  last_prediction = 2048;
  weights.fill(256);  // Initial weight
}

void ContextMixer::Add(int p) {
  if (num_inputs < MAX_INPUTS) {
    inputs[num_inputs++] = Stretch(std::clamp(p, 1, 4095));
  }
}

int ContextMixer::Stretch(int p) {
  return stretch_table[std::clamp(p, 0, 4095)];
}

int ContextMixer::Squash(int x) {
  return squash_table[std::clamp(x + 4096, 0, 8191)];
}

int ContextMixer::Mix() {
  if (num_inputs == 0) {
    last_prediction = 2048;
    return last_prediction;
  }

  // Weighted sum of stretched probabilities
  int64_t sum = 0;
  int64_t weight_sum = 0;
  for (int i = 0; i < num_inputs; i++) {
    sum += (int64_t)inputs[i] * weights[i];
    weight_sum += weights[i];
  }

  int x = (weight_sum > 0) ? (int)(sum / weight_sum) : 0;
  last_prediction = Squash(x);

  num_inputs = 0;  // Reset for next symbol
  return last_prediction;
}

void ContextMixer::Update(int bit) {
  // Update weights using gradient descent
  // Error = actual - predicted
  int error = (bit ? 4095 : 0) - last_prediction;

  // Learning rate
  constexpr int RATE = 8;

  for (int i = 0; i < MAX_INPUTS; i++) {
    // Adjust weight based on prediction contribution and error
    int delta = (error * inputs[i]) >> 16;
    weights[i] = std::clamp(weights[i] + delta * RATE, 1, 65535);
  }
}

// SSE Implementation
SSE::SSE() {
  // Initialize with neutral probabilities
  for (auto& row : table) {
    for (int i = 0; i < NUM_BUCKETS; i++) {
      // Linear interpolation from 0 to 4095
      row[i] = (uint16_t)((i * 4095 + NUM_BUCKETS / 2) / (NUM_BUCKETS - 1));
    }
  }
}

int SSE::Refine(int p, int context) {
  // Map prediction to bucket
  int bucket = (p * (NUM_BUCKETS - 1) + 2048) / 4096;
  bucket = std::clamp(bucket, 0, NUM_BUCKETS - 1);

  // Interpolate between adjacent buckets for smoother predictions
  int ctx = context & (NUM_CONTEXTS - 1);

  if (bucket == 0) {
    return table[ctx][0];
  } else if (bucket >= NUM_BUCKETS - 1) {
    return table[ctx][NUM_BUCKETS - 1];
  } else {
    // Linear interpolation
    int frac = (p * (NUM_BUCKETS - 1)) % 4096;
    int p1 = table[ctx][bucket];
    int p2 = table[ctx][bucket + 1];
    return p1 + ((p2 - p1) * frac) / 4096;
  }
}

void SSE::Update(int bit, int context) {
  int ctx = context & (NUM_CONTEXTS - 1);
  int target = bit ? 4095 : 0;

  // Update all buckets slightly, with more weight to the one that was used
  constexpr int RATE = 7;

  for (int i = 0; i < NUM_BUCKETS; i++) {
    int current = table[ctx][i];
    int error = target - current;
    table[ctx][i] = std::clamp(current + (error >> RATE), 0, 4095);
  }
}
