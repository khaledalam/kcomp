.PHONY: all build release test test-unit test-all benchmark clean install uninstall help

PREFIX ?= /usr/local

all: build

build:
	@cmake -B build
	@cmake --build build

release:
	@cmake -B build -DCMAKE_BUILD_TYPE=Release
	@cmake --build build --config Release

test: build
	@./test.sh

test-unit: build
	@echo "Building C++ unit tests..."
	@g++ -std=c++17 -O2 -I. \
		tests/test_roundtrip.cpp \
		src/models/ppm.cpp \
		src/models/bwt.cpp \
		src/models/lz77.cpp \
		src/models/lzopt.cpp \
		src/models/lzx.cpp \
		src/models/cm.cpp \
		src/models/dict.cpp \
		src/models/lzma.cpp \
		src/models/mixer.cpp \
		src/models/model257.cpp \
		src/models/rle.cpp \
		src/core/range_coder.cpp \
		src/io/file_io.cpp \
		-o build/test_roundtrip
	@echo "Running unit tests..."
	@./build/test_roundtrip

test-all: test test-unit
	@echo ""
	@echo "All tests passed!"

benchmark: build
	@./benchmark.sh

install: release
	@echo "Installing kcomp to $(PREFIX)/bin..."
	@sudo mkdir -p $(PREFIX)/bin
	@sudo cp build/kcomp $(PREFIX)/bin/kcomp
	@sudo chmod +x $(PREFIX)/bin/kcomp
	@echo "Done! Run 'kcomp' from anywhere."

uninstall:
	@echo "Removing kcomp from $(PREFIX)/bin..."
	@sudo rm -f $(PREFIX)/bin/kcomp
	@echo "Done!"

clean:
	@rm -rf build outputs test_data
	@echo "Cleaned build artifacts"

help:
	@echo "kcomp - High-performance compression utility"
	@echo ""
	@echo "Available targets:"
	@echo "  make build      - Build the project (debug)"
	@echo "  make release    - Build optimized release"
	@echo "  make test       - Run shell test suite"
	@echo "  make test-unit  - Build and run C++ unit tests"
	@echo "  make test-all   - Run all tests (shell + unit)"
	@echo "  make benchmark  - Run compression benchmarks"
	@echo "  make install    - Install to $(PREFIX)/bin (requires sudo)"
	@echo "  make uninstall  - Remove from $(PREFIX)/bin"
	@echo "  make clean      - Remove build artifacts"
	@echo "  make help       - Show this help message"
	@echo ""
	@echo "Usage after install:"
	@echo "  kcomp c <input> <output>   - Compress file"
	@echo "  kcomp d <input> <output>   - Decompress file"
	@echo "  kcomp b <input>            - Benchmark file"
