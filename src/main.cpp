#include "core/benchmark.hpp"
#include "io/file_io.hpp"
#include "models/ppm.hpp"
#include <cstdio>
#include <cstring>
#include <exception>

#ifndef KCOMP_VERSION
#define KCOMP_VERSION "1.0.1"
#endif

static void print_usage() {
  std::fprintf(stderr,
    "kcomp %s - High-performance compression utility\n"
    "\n"
    "Usage:\n"
    "  kcomp c <input> <output>   Compress a file\n"
    "  kcomp d <input> <output>   Decompress a file\n"
    "  kcomp b <input>            Benchmark compression on a file\n"
    "  kcomp -v, --version        Show version and credits\n"
    "  kcomp -h, --help           Show this help message\n"
    "\n"
    "Examples:\n"
    "  kcomp c document.txt document.kc    # Compress\n"
    "  kcomp d document.kc document.txt    # Decompress\n"
    "\n"
    "Algorithms: PPM, LZ77, BWT, Context Mixing with adaptive selection.\n",
    KCOMP_VERSION
  );
}

static void print_version() {
  std::printf(
    "kcomp %s\n"
    "\n"
    "High-performance compression utility with adaptive algorithm selection.\n"
    "Combines PPM, LZ77, BWT, and Context Mixing for optimal compression.\n"
    "\n"
    "Author:  Khaled Alam\n"
    "Website: https://khaledalam.net\n"
    "GitHub:  https://github.com/khaledalam/kcomp\n"
    "License: MIT\n",
    KCOMP_VERSION
  );
}

int main(int argc, char **argv) {
  try {
    if (argc < 2) {
      print_usage();
      return 1;
    }

    std::string cmd = argv[1];

    if (cmd == "-v" || cmd == "--version") {
      print_version();
      return 0;
    }

    if (cmd == "-h" || cmd == "--help") {
      print_usage();
      return 0;
    }

    if (cmd == "c") {
      if (argc != 4)
        return 1;
      auto input = ReadAll(argv[2]);
      auto out = CompressHybrid(input);
      WriteAll(argv[3], out);
      return 0;
    }

    if (cmd == "d") {
      if (argc != 4)
        return 1;
      auto input = ReadAll(argv[2]);
      auto out = DecompressHybrid(input);
      WriteAll(argv[3], out);
      return 0;
    }

    if (cmd == "b") {
      if (argc != 3)
        return 1;
      return Bench(argv[2]);
    }

    return 1;
  } catch (const std::exception &e) {
    std::fprintf(stderr, "error: %s\n", e.what());
    return 2;
  }
}
