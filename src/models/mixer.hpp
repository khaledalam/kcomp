#pragma once

#include <cstdint>
#include <vector>
#include <array>

// PAQ-style context mixer
// Blends predictions from multiple models using a neural network

class ContextMixer {
public:
  static constexpr int MAX_INPUTS = 8;

  ContextMixer();

  // Add a prediction (probability in 0-4095 range, 12-bit)
  void Add(int p);

  // Get mixed prediction (12-bit probability)
  int Mix();

  // Update weights based on actual bit
  void Update(int bit);

  void Reset();

private:
  std::array<int, MAX_INPUTS> inputs{};
  std::array<int, MAX_INPUTS> weights{};
  int num_inputs = 0;
  int last_prediction = 2048;

  // Stretch function: maps probability to log-odds
  static int Stretch(int p);
  // Squash function: inverse of stretch
  static int Squash(int x);
};

// Secondary Symbol Estimation (SSE)
// Refines predictions based on context and prediction bucket
class SSE {
public:
  SSE();

  // Refine prediction p (12-bit) using context
  int Refine(int p, int context);

  // Update with actual bit
  void Update(int bit, int context);

private:
  // Table indexed by (context, prediction_bucket)
  // Each entry is a 12-bit probability
  static constexpr int NUM_CONTEXTS = 256;
  static constexpr int NUM_BUCKETS = 32;
  std::array<std::array<uint16_t, NUM_BUCKETS>, NUM_CONTEXTS> table{};
};
