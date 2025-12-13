.PHONY: all build test benchmark clean help

all: build

build:
	@cmake -B build
	@cmake --build build

test: build
	@./test.sh

benchmark: build
	@./benchmark.sh

clean:
	@rm -rf build outputs test_data
	@echo "Cleaned build artifacts"

help:
	@echo "kcomp - PPM compression utility"
	@echo ""
	@echo "Available targets:"
	@echo "  make build      - Build the project"
	@echo "  make test       - Run test suite"
	@echo "  make benchmark  - Run compression benchmarks"
	@echo "  make clean      - Remove build artifacts"
	@echo "  make help       - Show this help message"
