#include "core/benchmark.hpp"
#include "core/progress.hpp"
#include "io/file_io.hpp"
#include "models/ppm.hpp"
#include <cstdio>
#include <cstring>
#include <exception>
#include <chrono>

#ifndef KCOMP_VERSION
#define KCOMP_VERSION "1.0.2"
#endif

// File format magic bytes
static const uint8_t MAGIC[2] = {'K', 'C'};
static const uint8_t FORMAT_VERSION = 2;

static void print_usage() {
  std::fprintf(stderr,
    "kcomp %s - High-performance compression utility\n"
    "\n"
    "Usage:\n"
    "  kcomp <input>              Compress (output: <input>.kc)\n"
    "  kcomp c <input> [output]   Compress a file\n"
    "  kcomp d <input> [output]   Decompress a file\n"
    "  kcomp b <input>            Benchmark compression\n"
    "  kcomp -v, --version        Show version and credits\n"
    "  kcomp -h, --help           Show this help message\n"
    "\n"
    "Options:\n"
    "  -s, --silent               Disable progress bar\n"
    "\n"
    "Examples:\n"
    "  kcomp video.mp4                        # -> video.mp4.kc\n"
    "  kcomp c document.txt                   # -> document.txt.kc\n"
    "  kcomp c document.txt archive.kc        # Explicit output\n"
    "  kcomp d archive.kc                     # -> original filename\n"
    "  kcomp d archive.kc document.txt        # Explicit output\n"
    "  kcomp c -s file.txt                    # Silent mode\n"
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

// Extract basename from path (without directory)
static std::string get_basename(const std::string& path) {
  size_t pos = path.find_last_of("/\\");
  if (pos == std::string::npos) return path;
  return path.substr(pos + 1);
}

// Generate output path for compression: input.ext -> input.ext.kc
static std::string make_compress_output(const std::string& input) {
  return input + ".kc";
}

// Generate output path for decompression: input.kc -> input, or input.ext.kc -> input.ext
static std::string make_decompress_output(const std::string& input) {
  if (input.size() > 3 && input.substr(input.size() - 3) == ".kc") {
    return input.substr(0, input.size() - 3);
  }
  return input + ".out";
}

// Check if argument looks like a file (not a flag or command)
static bool is_file_arg(const std::string& arg) {
  if (arg.empty()) return false;
  if (arg[0] == '-') return false;
  if (arg == "c" || arg == "d" || arg == "b") return false;
  return true;
}

// Add file header with original filename
static std::vector<uint8_t> add_header(const std::vector<uint8_t>& compressed, const std::string& original_name) {
  std::string basename = get_basename(original_name);
  if (basename.size() > 65535) basename = basename.substr(0, 65535);

  std::vector<uint8_t> result;
  result.reserve(5 + basename.size() + compressed.size());

  // Magic bytes
  result.push_back(MAGIC[0]);
  result.push_back(MAGIC[1]);

  // Version
  result.push_back(FORMAT_VERSION);

  // Filename length (2 bytes, little-endian)
  uint16_t name_len = static_cast<uint16_t>(basename.size());
  result.push_back(name_len & 0xFF);
  result.push_back((name_len >> 8) & 0xFF);

  // Filename
  result.insert(result.end(), basename.begin(), basename.end());

  // Compressed data
  result.insert(result.end(), compressed.begin(), compressed.end());

  return result;
}

// Parse file header and extract original filename
// Returns empty string if no header (legacy format)
static std::string parse_header(const std::vector<uint8_t>& data, size_t& data_offset) {
  data_offset = 0;

  // Check for magic bytes
  if (data.size() < 5 || data[0] != MAGIC[0] || data[1] != MAGIC[1]) {
    // Legacy format (no header)
    return "";
  }

  uint8_t version = data[2];
  if (version != FORMAT_VERSION) {
    // Unknown version, treat as legacy
    return "";
  }

  // Read filename length
  uint16_t name_len = data[3] | (data[4] << 8);

  if (data.size() < 5 + name_len) {
    // Corrupted header, treat as legacy
    return "";
  }

  // Extract filename
  std::string filename(data.begin() + 5, data.begin() + 5 + name_len);
  data_offset = 5 + name_len;

  return filename;
}

static int do_compress(const char* input_path, const char* output_path, bool silent) {
  size_t file_size = GetFileSize(input_path);
  bool show_progress = !silent && file_size > 0;

  std::vector<uint8_t> input;
  auto start = std::chrono::high_resolution_clock::now();

  // Read with progress
  if (show_progress) {
    ProgressBar read_bar(file_size, "Reading", true);
    input = ReadAllWithProgress(input_path, [&](size_t current, size_t) {
      read_bar.update(current);
    });
    read_bar.finish();
  } else {
    input = ReadAll(input_path);
  }

  // Compress with spinner
  std::vector<uint8_t> compressed;
  if (show_progress) {
    Spinner compress_spinner("Compressing", true);
    compressed = CompressHybrid(input);
    compress_spinner.finish("done");
  } else {
    compressed = CompressHybrid(input);
  }

  // Add header with original filename
  std::vector<uint8_t> out = add_header(compressed, input_path);

  // Write with progress
  if (show_progress) {
    ProgressBar write_bar(out.size(), "Writing", true);
    WriteAllWithProgress(output_path, out, [&](size_t current, size_t) {
      write_bar.update(current);
    });
    write_bar.finish();
  } else {
    WriteAll(output_path, out);
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

  if (!silent) {
    double ratio = input.size() > 0 ? (100.0 * out.size() / input.size()) : 0;
    std::fprintf(stderr, "\n%s -> %s\n", format_size(input.size()).c_str(), format_size(out.size()).c_str());
    std::fprintf(stderr, "Ratio: %.1f%% | Time: %.2fs\n", ratio, duration / 1000.0);
    std::fprintf(stderr, "Output: %s\n", output_path);
  }

  return 0;
}

static int do_decompress(const char* input_path, const std::string& explicit_output, bool silent) {
  size_t file_size = GetFileSize(input_path);
  bool show_progress = !silent && file_size > 0;

  std::vector<uint8_t> input;
  auto start = std::chrono::high_resolution_clock::now();

  // Read with progress
  if (show_progress) {
    ProgressBar read_bar(file_size, "Reading", true);
    input = ReadAllWithProgress(input_path, [&](size_t current, size_t) {
      read_bar.update(current);
    });
    read_bar.finish();
  } else {
    input = ReadAll(input_path);
  }

  // Parse header to get original filename
  size_t data_offset = 0;
  std::string original_name = parse_header(input, data_offset);

  // Determine output path
  std::string output_path;
  if (!explicit_output.empty()) {
    output_path = explicit_output;
  } else if (!original_name.empty()) {
    // Use original filename from header
    output_path = original_name;
  } else {
    // Fallback: remove .kc extension
    output_path = make_decompress_output(input_path);
  }

  // Extract compressed data (skip header if present)
  std::vector<uint8_t> compressed_data;
  if (data_offset > 0) {
    compressed_data.assign(input.begin() + data_offset, input.end());
  } else {
    compressed_data = std::move(input);
  }

  // Decompress with spinner
  std::vector<uint8_t> out;
  if (show_progress) {
    Spinner decompress_spinner("Decompressing", true);
    out = DecompressHybrid(compressed_data);
    decompress_spinner.finish("done");
  } else {
    out = DecompressHybrid(compressed_data);
  }

  // Write with progress
  if (show_progress) {
    ProgressBar write_bar(out.size(), "Writing", true);
    WriteAllWithProgress(output_path.c_str(), out, [&](size_t current, size_t) {
      write_bar.update(current);
    });
    write_bar.finish();
  } else {
    WriteAll(output_path, out);
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

  if (!silent) {
    std::fprintf(stderr, "\n%s -> %s\n", format_size(file_size).c_str(), format_size(out.size()).c_str());
    std::fprintf(stderr, "Time: %.2fs\n", duration / 1000.0);
    std::fprintf(stderr, "Output: %s\n", output_path.c_str());
  }

  return 0;
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

    // Handle: kcomp -s file.txt (silent shorthand compression)
    if (cmd == "-s" || cmd == "--silent") {
      if (argc < 3) {
        std::fprintf(stderr, "Usage: kcomp -s <input>\n");
        return 1;
      }
      std::string input_path = argv[2];
      std::string output_path = make_compress_output(input_path);
      return do_compress(input_path.c_str(), output_path.c_str(), true);
    }

    // Shorthand: kcomp file.txt -> compress to file.txt.kc
    if (is_file_arg(cmd)) {
      bool silent = false;

      // Check for -s flag after file: kcomp file.txt -s
      if (argc > 2) {
        std::string arg2 = argv[2];
        if (arg2 == "-s" || arg2 == "--silent") {
          silent = true;
        }
      }

      std::string output = make_compress_output(cmd);
      return do_compress(argv[1], output.c_str(), silent);
    }

    if (cmd == "c") {
      // Parse optional flags
      bool silent = false;
      std::vector<std::string> args;

      for (int i = 2; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-s" || arg == "--silent") {
          silent = true;
        } else {
          args.push_back(arg);
        }
      }

      if (args.empty()) {
        std::fprintf(stderr, "Usage: kcomp c [-s|--silent] <input> [output]\n");
        return 1;
      }

      std::string input_path = args[0];
      std::string output_path = args.size() > 1 ? args[1] : make_compress_output(input_path);

      return do_compress(input_path.c_str(), output_path.c_str(), silent);
    }

    if (cmd == "d") {
      // Parse optional flags
      bool silent = false;
      std::vector<std::string> args;

      for (int i = 2; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-s" || arg == "--silent") {
          silent = true;
        } else {
          args.push_back(arg);
        }
      }

      if (args.empty()) {
        std::fprintf(stderr, "Usage: kcomp d [-s|--silent] <input> [output]\n");
        return 1;
      }

      std::string input_path = args[0];
      std::string explicit_output = args.size() > 1 ? args[1] : "";

      return do_decompress(input_path.c_str(), explicit_output, silent);
    }

    if (cmd == "b") {
      if (argc != 3)
        return 1;
      return Bench(argv[2]);
    }

    print_usage();
    return 1;
  } catch (const std::exception &e) {
    std::fprintf(stderr, "\nerror: %s\n", e.what());
    return 2;
  }
}
