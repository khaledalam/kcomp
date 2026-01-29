# Changelog

All notable changes to kcomp will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [1.0.2] - 2025-01-29

### Added
- Progress bar during compression and decompression
- `--silent` (`-s`) flag to disable progress output
- Original filename stored in compressed files (auto-restore on decompress)
- Shorthand compression: `kcomp file.txt` creates `file.txt.kc`
- Optional output argument: `kcomp c file.txt` auto-generates output name
- `--version` and `--help` flags
- Human-readable file size display
- Compression ratio and timing statistics

### Changed
- Decompression now restores original filename when no output specified
- Updated help message with new usage examples

## [1.0.1] - 2025-01-29

### Added
- Professional CLI help output with version number
- Separate `--version` flag showing author credits
- Version number from CMake build system

### Changed
- Improved usage message formatting

## [1.0.0] - 2025-01-28

### Added
- Initial release
- Hybrid compression with 50+ algorithm pipelines
- PPM (Order 1-6) compression
- LZ77, LZOpt, LZX variants
- BWT + MTF transform
- Context Mixing (PAQ-style)
- RLE and Delta encoding
- Automatic algorithm selection for best compression
- Homebrew formula for easy installation
- Comprehensive test suite
- Benchmark system comparing against gzip, brotli, xz, zstd

### Performance
- Wins against gzip/brotli/xz/zstd on 69% of test files
- Up to 5.6x better compression than gzip on BMP images
- Excellent results on structured data (JSON, XML, CSV)
