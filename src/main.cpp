#include "core/benchmark.hpp"
#include "io/file_io.hpp"
#include "models/ppm.hpp"
#include <cstdio>
#include <exception>

int main(int argc, char **argv) {
  try {
    if (argc < 2) {
      std::fprintf(stderr, "usage:\n  kcomp c <in> <out>\n  kcomp d <in> "
                           "<out>\n  kcomp b <in>\n");
      return 1;
    }

    std::string cmd = argv[1];

    if (cmd == "c") {
      if (argc != 4)
        return 1;
      auto input = ReadAll(argv[2]);
      auto out = CompressPPM2(input);
      WriteAll(argv[3], out);
      return 0;
    }

    if (cmd == "d") {
      if (argc != 4)
        return 1;
      auto input = ReadAll(argv[2]);
      auto out = DecompressPPM2(input);
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
