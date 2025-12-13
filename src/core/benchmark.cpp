#include "benchmark.hpp"
#include "../io/file_io.hpp"
#include "../models/ppm.hpp"
#include "../models/rle.hpp"
#include <chrono>
#include <cstdio>

static uint64_t NowNs() {
  return (uint64_t)std::chrono::duration_cast<std::chrono::nanoseconds>(
             std::chrono::high_resolution_clock::now().time_since_epoch())
      .count();
}

static void PrintBench(const char *name, size_t in_sz, size_t out_sz,
                       double sec_c, double sec_d) {
  double ratio = in_sz ? (100.0 * (double)out_sz / (double)in_sz) : 0.0;
  std::printf("%-10s  out=%10zu  ratio=%7.2f%%  c=%8.4fs  d=%8.4fs\n", name,
              out_sz, ratio, sec_c, sec_d);
}

int Bench(const std::string &path) {
  auto input = ReadAll(path);

  {
    uint64_t t0 = NowNs();
    auto out = input;
    uint64_t t1 = NowNs();
    uint64_t t2 = NowNs();
    auto back = out;
    uint64_t t3 = NowNs();
    if (back != input)
      return 2;
    PrintBench("copy", input.size(), out.size(), (t1 - t0) / 1e9,
               (t3 - t2) / 1e9);
  }

  {
    uint64_t t0 = NowNs();
    auto out = CompressRLE(input);
    uint64_t t1 = NowNs();
    uint64_t t2 = NowNs();
    auto back = DecompressRLE(out);
    uint64_t t3 = NowNs();
    if (back != input)
      return 2;
    PrintBench("rle", input.size(), out.size(), (t1 - t0) / 1e9,
               (t3 - t2) / 1e9);
  }

  {
    uint64_t t0 = NowNs();
    auto out = CompressPPM1(input);
    uint64_t t1 = NowNs();
    uint64_t t2 = NowNs();
    auto back = DecompressPPM1(out);
    uint64_t t3 = NowNs();
    if (back != input)
      return 2;
    PrintBench("ppm1", input.size(), out.size(), (t1 - t0) / 1e9,
               (t3 - t2) / 1e9);
  }

  {
    uint64_t t0 = NowNs();
    auto out = CompressPPM2(input);
    uint64_t t1 = NowNs();
    uint64_t t2 = NowNs();
    auto back = DecompressPPM2(out);
    uint64_t t3 = NowNs();
    if (back != input)
      return 2;
    PrintBench("ppm2", input.size(), out.size(), (t1 - t0) / 1e9,
               (t3 - t2) / 1e9);
  }

  return 0;
}
