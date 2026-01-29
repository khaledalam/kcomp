.PHONY: all build release test test-unit test-all benchmark clean install uninstall help tag brew-update changelog

PREFIX ?= /usr/local
VERSION ?= $(shell git describe --tags --abbrev=0 2>/dev/null || echo "v1.0.0")
BREW_TAP_REPO ?= $(HOME)/homebrew-kcomp

# Get current version from CMakeLists.txt
CURRENT_VERSION := $(shell grep -o 'VERSION [0-9]*\.[0-9]*\.[0-9]*' CMakeLists.txt | cut -d' ' -f2)
MAJOR := $(shell echo $(CURRENT_VERSION) | cut -d. -f1)
MINOR := $(shell echo $(CURRENT_VERSION) | cut -d. -f2)
PATCH := $(shell echo $(CURRENT_VERSION) | cut -d. -f3)

# Calculate next versions
NEXT_PATCH := $(shell echo $$(($(PATCH) + 1)))
NEXT_MINOR := $(shell echo $$(($(MINOR) + 1)))
NEXT_MAJOR := $(shell echo $$(($(MAJOR) + 1)))

# Default bump type is patch
type ?= patch

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

tag:
	@if [ -z "$(v)" ]; then \
		echo "Usage: make tag v=1.0.1"; \
		echo "Current version: $(VERSION)"; \
		exit 1; \
	fi
	@echo "Creating release tag v$(v)..."
	@git tag -a "v$(v)" -m "Release v$(v)"
	@git push origin "v$(v)"
	@echo "Tag v$(v) created and pushed!"
	@echo ""
	@echo "Next steps:"
	@echo "  1. Wait for GitHub to create the release tarball"
	@echo "  2. Run: make brew-update v=$(v)"

brew-update:
	@if [ -z "$(v)" ]; then \
		echo "Usage: make brew-update v=1.0.1"; \
		exit 1; \
	fi
	@echo "Updating Homebrew formula for v$(v)..."
	@SHA=$$(curl -sL https://github.com/khaledalam/kcomp/archive/refs/tags/v$(v).tar.gz | shasum -a 256 | cut -d' ' -f1); \
	echo "SHA256: $$SHA"; \
	if [ -d "$(BREW_TAP_REPO)" ]; then \
		cd $(BREW_TAP_REPO) && git pull; \
	else \
		git clone https://github.com/khaledalam/homebrew-kcomp.git $(BREW_TAP_REPO); \
	fi; \
	sed -i '' 's|url ".*"|url "https://github.com/khaledalam/kcomp/archive/refs/tags/v$(v).tar.gz"|' $(BREW_TAP_REPO)/Formula/kcomp.rb; \
	sed -i '' 's|sha256 ".*"|sha256 "'$$SHA'"|' $(BREW_TAP_REPO)/Formula/kcomp.rb; \
	cd $(BREW_TAP_REPO) && git add . && git commit -m "Update kcomp to v$(v)" && git push; \
	echo ""; \
	echo "Homebrew formula updated to v$(v)!"; \
	echo "Users can now run: brew upgrade kcomp"

changelog:
	@if [ -z "$(v)" ]; then \
		echo "Usage: make changelog v=1.0.1"; \
		exit 1; \
	fi
	@echo "Updating CHANGELOG.md for v$(v)..."
	@DATE=$$(date +%Y-%m-%d); \
	sed -i '' "s/## \[Unreleased\]/## [Unreleased]\n\n## [$(v)] - $$DATE/" CHANGELOG.md; \
	echo "CHANGELOG.md updated with version $(v)"

version-bump:
	@if [ -z "$(v)" ]; then \
		echo "Usage: make version-bump v=1.0.1"; \
		exit 1; \
	fi
	@echo "Updating version to $(v)..."
	@sed -i '' 's/project(kcomp VERSION [0-9]*\.[0-9]*\.[0-9]*/project(kcomp VERSION $(v)/' CMakeLists.txt
	@sed -i '' 's/#define KCOMP_VERSION "[0-9]*\.[0-9]*\.[0-9]*"/#define KCOMP_VERSION "$(v)"/' src/main.cpp
	@echo "Version updated to $(v) in CMakeLists.txt and main.cpp"

release-new:
	@if [ -n "$(v)" ]; then \
		NEW_VERSION="$(v)"; \
	elif [ "$(type)" = "major" ]; then \
		NEW_VERSION="$(NEXT_MAJOR).0.0"; \
	elif [ "$(type)" = "minor" ]; then \
		NEW_VERSION="$(MAJOR).$(NEXT_MINOR).0"; \
	else \
		NEW_VERSION="$(MAJOR).$(MINOR).$(NEXT_PATCH)"; \
	fi; \
	echo "=== Creating release v$$NEW_VERSION ==="; \
	echo ""; \
	echo "Current version: $(CURRENT_VERSION)"; \
	echo "New version:     $$NEW_VERSION"; \
	echo ""; \
	echo "Step 1: Updating changelog..."; \
	$(MAKE) changelog v=$$NEW_VERSION; \
	echo ""; \
	echo "Step 2: Bumping version..."; \
	$(MAKE) version-bump v=$$NEW_VERSION; \
	echo ""; \
	echo "Step 3: Committing changes..."; \
	git add CHANGELOG.md CMakeLists.txt src/main.cpp; \
	git commit -m "Release v$$NEW_VERSION"; \
	git push; \
	echo ""; \
	echo "Step 4: Running tests..."; \
	./test.sh; \
	echo ""; \
	echo "Step 5: Creating tag..."; \
	git tag -a "v$$NEW_VERSION" -m "Release v$$NEW_VERSION"; \
	git push origin "v$$NEW_VERSION"; \
	echo ""; \
	echo "Step 6: Waiting for GitHub..."; \
	sleep 5; \
	echo ""; \
	echo "Step 7: Updating Homebrew formula..."; \
	$(MAKE) brew-update v=$$NEW_VERSION; \
	echo ""; \
	echo "=== Release v$$NEW_VERSION complete! ==="; \
	echo ""; \
	echo "Changelog: https://github.com/khaledalam/kcomp/blob/master/CHANGELOG.md"

help:
	@echo "kcomp - High-performance compression utility"
	@echo ""
	@echo "Build & Test:"
	@echo "  make build        - Build the project (debug)"
	@echo "  make release      - Build optimized release"
	@echo "  make test         - Run shell test suite"
	@echo "  make test-unit    - Build and run C++ unit tests"
	@echo "  make test-all     - Run all tests (shell + unit)"
	@echo "  make benchmark    - Run compression benchmarks"
	@echo ""
	@echo "Installation:"
	@echo "  make install      - Install to $(PREFIX)/bin (requires sudo)"
	@echo "  make uninstall    - Remove from $(PREFIX)/bin"
	@echo "  make clean        - Remove build artifacts"
	@echo ""
	@echo "Release Management:"
	@echo "  make release-new           - Auto-increment patch ($(CURRENT_VERSION) -> $(MAJOR).$(MINOR).$(NEXT_PATCH))"
	@echo "  make release-new type=minor - Bump minor version (-> $(MAJOR).$(NEXT_MINOR).0)"
	@echo "  make release-new type=major - Bump major version (-> $(NEXT_MAJOR).0.0)"
	@echo "  make release-new v=X.Y.Z   - Explicit version"
	@echo "  make changelog v=X.Y.Z     - Update CHANGELOG.md"
	@echo "  make version-bump v=X.Y.Z  - Bump version in source files"
	@echo "  make tag v=X.Y.Z           - Create and push git tag"
	@echo "  make brew-update v=X.Y.Z   - Update Homebrew formula"
	@echo ""
	@echo "Usage after install:"
	@echo "  kcomp <input>              - Compress (auto output: <input>.kc)"
	@echo "  kcomp c <input> [output]   - Compress file"
	@echo "  kcomp d <input> [output]   - Decompress file (restores original name)"
	@echo "  kcomp b <input>            - Benchmark file"
	@echo "  kcomp -s <input>           - Compress silently"
	@echo ""
	@echo "Current version: $(VERSION)"
