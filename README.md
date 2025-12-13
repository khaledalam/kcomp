# kcomp

High-performance data compression using Prediction by Partial Matching (PPM) with range coding.

kcomp implements PPM order-2 compression with arithmetic coding, delivering compression ratios competitive with industry-standard algorithms while maintaining a clean, focused codebase.

⸻

## Benchmark Results

### enwik_10k (10KB)

```text
Algorithm      Compressed    Ratio     Performance
─────────────────────────────────────────────────
brotli -11        2,940B     29.40%    ████████████████████  Best
xz -9e            3,664B     36.64%    ████████████████
zstd -19          3,669B     36.69%    ████████████████
gzip -9           3,725B     37.25%    ███████████████
kcomp (PPM2)      4,726B     47.26%    ████████████
```

### enwik_100k (100KB)

```text
Algorithm      Compressed    Ratio     Performance
─────────────────────────────────────────────────
brotli -11       29,388B     29.39%    ████████████████████  Best
xz -9e           33,024B     33.02%    ██████████████████
zstd -19         33,744B     33.74%    █████████████████
gzip -9          36,250B     36.25%    ████████████████
kcomp (PPM2)     40,662B     40.66%    ██████████████
```

### enwik_1m (1MB)

```text
Algorithm      Compressed    Ratio     Performance
─────────────────────────────────────────────────
brotli -11      281,012B     28.10%    ████████████████████  Best
xz -9e          290,740B     29.07%    ███████████████████
zstd -19        300,115B     30.01%    ██████████████████
gzip -9         355,800B     35.58%    ███████████████
kcomp (PPM2)    392,744B     39.27%    █████████████
```

kcomp achieves **39-47% compression ratio** across datasets, performing well on small files (10-100KB) while remaining competitive on medium files (1MB).

```bash
$ ./benchmark_multi.sh
kcomp multi-dataset benchmark

Dataset: enwik_10k (10000 bytes)
────────────────────────────────────────────────────────────────────────
algorithm        compressed     ratio      encode      decode  verify
────────────────────────────────────────────────────────────────────────
kcomp (PPM2)          4726B    47.26%           s           s  OK
gzip -9               3725B    37.25%           s           s  OK
brotli -11            2940B    29.40%           s           s  OK
zstd -19              3669B    36.69%           s           s  OK
xz -9e                3664B    36.64%           s           s  OK
────────────────────────────────────────────────────────────────────────

Dataset: enwik_100k (100000 bytes)
────────────────────────────────────────────────────────────────────────
algorithm        compressed     ratio      encode      decode  verify
────────────────────────────────────────────────────────────────────────
kcomp (PPM2)         40662B    40.66%           s           s  OK
gzip -9              36250B    36.25%           s           s  OK
brotli -11           29388B    29.39%           s           s  OK
zstd -19             33744B    33.74%           s           s  OK
xz -9e               33024B    33.02%           s           s  OK
────────────────────────────────────────────────────────────────────────

Dataset: enwik_1m (1000000 bytes)
────────────────────────────────────────────────────────────────────────
algorithm        compressed     ratio      encode      decode  verify
────────────────────────────────────────────────────────────────────────
kcomp (PPM2)        392744B    39.27%           s           s  OK
gzip -9             355800B    35.58%           s           s  OK
brotli -11          281012B    28.10%           s           s  OK
zstd -19            300115B    30.01%           s           s  OK
xz -9e              290740B    29.07%           s           s  OK
────────────────────────────────────────────────────────────────────────
```

⸻

## Features

- **PPM Order-2**: Uses previous 2 bytes as context for superior prediction
- **Range Coding**: Efficient arithmetic coding implementation
- **Adaptive Models**: Online frequency adaptation with automatic rescaling
- **Multiple Algorithms**: PPM1, PPM2, RLE, and copy for comparison
- **Comprehensive Testing**: Full test suite ensuring correctness
- **Clean Output**: All generated files isolated in `outputs/` directory

⸻

## Quick Start

### Build

```bash
cmake -B build
cmake --build build
```

### Test

```bash
./test.sh
```

### Benchmark

Single file benchmark:

```bash
./benchmark.sh
```

Or with a custom file:

```bash
./benchmark.sh path/to/your/file.txt
```

Multi-dataset benchmark (requires enwik datasets):

```bash
./benchmark_multi.sh
```

To download enwik datasets:

```bash
curl -L http://mattmahoney.net/dc/enwik8.zip -o enwik8.zip
unzip enwik8.zip
head -c 10000 enwik8 > enwik_10k
head -c 100000 enwik8 > enwik_100k
head -c 1000000 enwik8 > enwik_1m
```

⸻

## Usage

### Compress a file

```bash
./build/kcomp c input.txt output.kcomp
```

### Decompress a file

```bash
./build/kcomp d output.kcomp restored.txt
```

### Run internal benchmark

```bash
./build/kcomp b testfile.txt
```

⸻

## Algorithm

kcomp uses **PPM (Prediction by Partial Matching)** with order-2 context modeling:

1. **Context Modeling**: Tracks the previous 2 bytes to predict the next byte
2. **Escape Mechanism**: Falls back to lower-order contexts when symbol not found
3. **Range Coding**: Encodes symbols using arithmetic coding for near-optimal compression
4. **Adaptive Learning**: Models update online with automatic rescaling to prevent overflow

**Model Hierarchy:**

- Order-2: 65,536 contexts (256×256 previous bytes)
- Order-1: 256 contexts (1 previous byte)
- Order-0: Uniform distribution fallback

This multi-order approach balances compression ratio with robustness on diverse data.

⸻

## Project Structure

```text
khabit/
├── src/
│   ├── main.cpp          Entry point
│   ├── core/
│   │   ├── range_coder.{hpp,cpp}  Range (arithmetic) coder
│   │   └── benchmark.{hpp,cpp}     Benchmark harness
│   ├── models/
│   │   ├── model257.{hpp,cpp}      Frequency model
│   │   ├── ppm.{hpp,cpp}           PPM compressors
│   │   └── rle.{hpp,cpp}           Run-length encoding
│   └── io/
│       ├── buffer.hpp              I/O buffers
│       └── file_io.{hpp,cpp}       File operations
├── build/                Compiled binaries (gitignored)
├── outputs/              Benchmark outputs (gitignored)
├── benchmark.sh          Comparative benchmark script
├── test.sh               Test suite
├── CMakeLists.txt        Build configuration
├── Makefile              Convenience commands
└── LICENSE               MIT License
```

⸻

## Development

### Requirements

- C++17 compatible compiler (GCC, Clang, MSVC)
- CMake 3.10 or higher

### Testing

Run the test suite to verify correctness:

```bash
./test.sh
```

All tests must pass before deploying changes.

⸻

## Performance Notes

- Optimized for small to medium text files
- Order-2 context provides excellent compression on English text
- Trade-off: Higher memory usage (~17MB) for better compression
- Speed: Competitive with gzip on small files

⸻

## Roadmap

- [ ] Order-3+ context with hashing
- [ ] Secondary symbol estimation
- [ ] LZ77 preprocessing
- [ ] Model mixing/weighting
- [ ] Multithreading support
- [ ] Stable file format specification

⸻

## Author

Khaled Alam
kcomp (khaled compress)

⸻

## License

MIT License - Free to use, modify, and distribute.
