# kcomp

A high-performance compression utility that combines multiple compression techniques with adaptive algorithm selection to achieve optimal compression ratios across diverse data types.

## Overview

kcomp implements an ensemble of compression algorithms including PPM (Prediction by Partial Matching), LZ77 variants, BWT (Burrows-Wheeler Transform), and Context Mixing. The hybrid compressor automatically evaluates 50+ compression pipelines and selects the one producing the smallest output.

## Benchmark Results

> Tested on 16 diverse file types against gzip, brotli, xz, and zstd

### Scoreboard

```text
                        WINS
  kcomp    ███████████████████████████       11  (69%)
  brotli   ███████                            3  (19%)
  xz       ██                                 1  (6%)
  zstd     ██                                 1  (6%)
  gzip     ░                                  0  (0%)
```

---

### Compression Ratio Comparison

*Shorter bar = better compression*

#### Text & Structured Data

```text
English Dictionary (50 KB)
  kcomp   ███                          12.8%  ★ BEST
  gzip    ████████                     31.6%
  brotli  ████████                     31.6%
  xz      ███████                      27.9%
  zstd    ███████                      28.2%

JSON Data (51 KB)
  kcomp   █                             2.0%  ★ BEST
  gzip    ██                            8.0%
  brotli  █                             3.1%
  xz      █                             3.2%
  zstd    █                             3.5%

XML Data (23 KB)
  kcomp   █                             2.8%  ★ BEST
  gzip    ███                          10.3%
  brotli  █                             4.4%
  xz      █                             3.9%
  zstd    █                             4.7%

CSV Data (38 KB)
  kcomp   █                             5.3%  ★ BEST
  gzip    ████                         17.1%
  brotli  █                             5.9%
  xz      ██                            6.4%
  zstd    ██                            6.8%

Log File (42 KB)
  kcomp   █                             5.8%  ★ BEST
  gzip    ███                          10.2%
  brotli  ██                            7.6%
  xz      ██                            6.7%
  zstd    ██                            7.4%
```

#### Media & Documents

```text
BMP Image (3 KB)
  kcomp   ███                          13.1%  ★ BEST
  gzip    ██████████████████           73.7%
  brotli  ██████████████████           72.4%
  xz      ██████████████████████       88.9%
  zstd    ██████████████████           72.2%

PNG Image (309 B)
  kcomp   ████████████████████         79.3%  ★ BEST
  gzip    ███████████████████████████ 108.4%
  brotli  ████████████████████████     94.5%
  xz      ██████████████████████████  106.1%
  zstd    ██████████████████████████  104.5%

WAV Audio (8 KB)
  kcomp   █                             5.0%  ★ BEST
  gzip    ██                            7.0%
  brotli  █                             5.8%
  xz      ██                            6.5%
  zstd    █                             5.8%

PDF Document (454 B)
  kcomp   █████████████                51.5%  ★ BEST
  gzip    ████████████████             63.4%
  brotli  █████████████                53.7%
  xz      ██████████████████           74.0%
  zstd    ██████████████               57.7%
```

#### Binary & Executables

```text
ELF Binary (10 KB)
  kcomp   ░                             0.4%  ★ BEST
  gzip    ░                             0.8%
  brotli  ░                             0.5%
  xz      ░                             1.4%
  zstd    ░                             0.5%

C++ Source (47 KB)
  kcomp   ████                         15.8%
  gzip    ████                         15.4%
  brotli  ████                         14.7%
  xz      ████                         14.3%  ★ BEST
  zstd    ████                         14.4%
```

#### Edge Cases

```text
Repeated Pattern (70 KB)
  kcomp   ░                            0.05%
  gzip    ░                             0.3%
  brotli  ░                            0.04%  ★ BEST
  xz      ░                             0.2%
  zstd    ░                            0.06%

Random Binary (10 KB)
  kcomp   █████████████████████████  100.0%  ★ BEST
  gzip    █████████████████████████  100.4%
  brotli  █████████████████████████  100.1%
  xz      █████████████████████████  100.7%
  zstd    █████████████████████████  100.1%
```

---

### Key Improvements vs gzip

```text
                              kcomp     gzip      Δ
BMP Image                     13.1%    73.7%    5.6x better
JSON Data                      2.0%     8.1%    4.0x better
XML Data                       2.8%    10.3%    3.7x better
CSV Data                       5.3%    17.1%    3.2x better
English Dictionary            12.8%    31.6%    2.5x better
PNG Image                     79.3%   108.4%    1.4x better
WAV Audio                      5.0%     7.0%    1.4x better
ELF Binary                     0.4%     0.9%    2.0x better
```

## Installation

### Homebrew (macOS/Linux)

```bash
brew install khaledalam/kcomp/kcomp
```

### From Source

```bash
git clone https://github.com/khaledalam/kcomp
cd kcomp
make install
```

This installs `kcomp` to `/usr/local/bin`. You can then use it from anywhere:

```bash
kcomp c file.txt file.kc    # Compress
kcomp d file.kc file.txt    # Decompress
```

To uninstall:

```bash
# If installed via Homebrew
brew uninstall kcomp

# If installed from source
make uninstall
```

### Build Only (without installing)

```bash
make build
./build/kcomp c input.txt output.kc
```

### Custom Install Location

```bash
make install PREFIX=/opt/kcomp
```

## Usage

```bash
# Compress a file
kcomp c input.txt output.kc

# Decompress a file
kcomp d output.kc restored.txt

# Benchmark compression on a file
kcomp b testfile.txt

# Run full benchmark suite
./benchmark_all.sh
```

## Compression Algorithms

### PPM (Order 1-6)

Statistical compression using context modeling. Higher orders provide better predictions for structured data at the cost of memory.

### LZ77 Variants

- **LZ77**: 64KB sliding window with lazy matching
- **LZOpt**: 1MB window with optimal parsing via dynamic programming
- **LZX**: 64MB window with suffix array matching
- **LZMA-style**: 1MB window with optimal parsing

### Transform Pipelines

- **BWT+MTF**: Burrows-Wheeler Transform followed by Move-to-Front encoding
- **Delta**: Difference encoding for gradual value changes
- **RLE**: Run-length encoding for repeated bytes
- **Word Tokenization**: Common pattern substitution

### Context Mixing

PAQ-style neural network mixer combining multiple prediction models for maximum compression.

## Algorithm Selection

The hybrid compressor evaluates these combinations:

- PPM5, PPM6 standalone
- LZ77/LZOpt/LZX + PPM3/5/6
- BWT+MTF + PPM3/5/6
- RLE + PPM5/6
- Delta + PPM5
- Word + PPM5/6
- LZMA + PPM5/6
- Various multi-stage pipelines

The smallest result is selected automatically.

## Technical Details

### Frequency Model

- Fenwick tree for O(log n) cumulative frequency queries
- Adaptive rescaling at 16K total count
- Witten-Bell escape probability estimation

### Range Coder

- 32-bit precision arithmetic coding
- Byte-aligned output

### Memory Usage

- PPM5: ~20MB for sparse contexts
- BWT: Limited to 1MB inputs
- Context Mixing: 512KB limit

## Project Structure

```text
kcomp/
├── src/
│   ├── main.cpp
│   ├── core/
│   │   ├── range_coder.cpp    Arithmetic coding
│   │   └── benchmark.cpp      Performance testing
│   ├── models/
│   │   ├── model257.cpp       Frequency model
│   │   ├── ppm.cpp            PPM1-6 + Hybrid
│   │   ├── lz77.cpp           LZ77, RLE, Delta
│   │   ├── lzopt.cpp          Optimal parsing LZ
│   │   ├── lzx.cpp            Suffix array LZ
│   │   ├── lzma.cpp           LZMA-style compression
│   │   ├── bwt.cpp            BWT + MTF
│   │   ├── cm.cpp             Context mixing
│   │   └── dict.cpp           Dictionary preprocessing
│   └── io/
│       └── file_io.cpp
├── testdata/                   Test corpus
├── benchmark_all.sh           Full benchmark suite
└── test.sh                    Verification tests
```

## Performance Characteristics

**Strengths:**

- Natural language text
- Structured data (JSON, XML, CSV)
- Log files with repetitive patterns
- Binary files with structure
- Media files (uncompressed BMP, WAV)

**Competitive:**

- Source code
- Archives

**Limitations:**

- Already-compressed data (PNG, JPEG, MP3)
- Very small files (<1KB)

## Building from Source

Requirements:

- C++17 compatible compiler
- CMake 3.10+

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

## Running Tests

```bash
./test.sh
```

Verifies compression/decompression roundtrip for all algorithms.

## License

MIT License

## Author

**Khaled Alam**

- Website: [khaledalam.net](https://khaledalam.net)
- LinkedIn: [linkedin.com/in/khaledalam](https://linkedin.com/in/khaledalam)
- GitHub: [github.com/khaledalam](https://github.com/khaledalam)
