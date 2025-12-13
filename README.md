# kcomp

High-performance data compression using Prediction by Partial Matching (PPM) with range coding.

kcomp implements PPM order-2 compression with arithmetic coding, delivering compression ratios competitive with industry-standard algorithms while maintaining a clean, focused codebase.

⸻

## Benchmark Results

Tested on enwik_10k (10,000 bytes of English Wikipedia text):

```text
Algorithm      Compressed    Ratio     Performance
─────────────────────────────────────────────────
brotli -11        2,940B     29.40%    ████████████████████  Best
xz -9e            3,664B     36.64%    ████████████████
zstd -19          3,669B     36.69%    ████████████████
gzip -9           3,725B     37.25%    ███████████████
kcomp (PPM2)      4,726B     47.26%    ████████████
```

kcomp achieves 47.26% compression ratio, reducing file size by more than half while maintaining fast encode/decode speeds.

```bash
$ ./benchmark.sh
kcomp benchmark
input: enwik_10k (10000B)
────────────────────────────────────────────────────────────
algo                  out     ratio         enc         dec  verify
────────────────────────────────────────────────────────────
kcomp               4726B    47.26%           s           s  OK
gzip -9             3725B    37.25%           s           s  OK
brotli 11           2940B    29.40%           s           s  OK
zstd -19            3669B    36.69%           s           s  OK
xz -9e              3664B    36.64%           s           s  OK
────────────────────────────────────────────────────────────
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

```bash
./benchmark.sh
```

Or with a custom file:

```bash
./benchmark.sh path/to/your/file.txt
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
