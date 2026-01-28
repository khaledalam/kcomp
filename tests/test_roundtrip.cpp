#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdint>
#include <cassert>

// Include headers
#include "../src/models/ppm.hpp"
#include "../src/models/bwt.hpp"
#include "../src/models/lz77.hpp"

int tests_passed = 0;
int tests_failed = 0;

void test(const std::string& name, bool condition) {
    if (condition) {
        std::cout << "  [PASS] " << name << "\n";
        tests_passed++;
    } else {
        std::cout << "  [FAIL] " << name << "\n";
        tests_failed++;
    }
}

std::vector<uint8_t> read_file(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return {};
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(f)),
                                 std::istreambuf_iterator<char>());
}

// Test PPM compression/decompression
void test_ppm() {
    std::cout << "\n=== PPM Tests ===\n";

    // Test empty
    {
        std::vector<uint8_t> empty;
        auto compressed = CompressPPM5(empty);
        auto decompressed = DecompressPPM5(compressed);
        test("PPM5 empty", decompressed.empty());
    }

    // Test single byte
    {
        std::vector<uint8_t> single = {'A'};
        auto compressed = CompressPPM5(single);
        auto decompressed = DecompressPPM5(compressed);
        test("PPM5 single byte", single == decompressed);
    }

    // Test repeated bytes
    {
        std::vector<uint8_t> repeated(100, 'X');
        auto compressed = CompressPPM5(repeated);
        auto decompressed = DecompressPPM5(compressed);
        test("PPM5 repeated bytes", repeated == decompressed);
    }

    // Test simple text
    {
        std::string s = "Hello, World!";
        std::vector<uint8_t> text(s.begin(), s.end());
        auto compressed = CompressPPM5(text);
        auto decompressed = DecompressPPM5(compressed);
        test("PPM5 simple text", text == decompressed);
    }

    // Test PPM3
    {
        std::string s = "The quick brown fox jumps over the lazy dog.";
        std::vector<uint8_t> text(s.begin(), s.end());
        auto compressed = CompressPPM3(text);
        auto decompressed = DecompressPPM3(compressed);
        test("PPM3 pangram", text == decompressed);
    }

    // Test PPM6
    {
        std::string s = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        std::vector<uint8_t> text(s.begin(), s.end());
        auto compressed = CompressPPM6(text);
        auto decompressed = DecompressPPM6(compressed);
        test("PPM6 alphabet", text == decompressed);
    }
}

// Test BWT compression/decompression
void test_bwt() {
    std::cout << "\n=== BWT Tests ===\n";

    // Test empty
    {
        std::vector<uint8_t> empty;
        uint32_t idx;
        auto encoded = BWTEncode(empty, idx);
        auto decoded = BWTDecode(encoded, idx);
        test("BWT empty", decoded.empty());
    }

    // Test single byte
    {
        std::vector<uint8_t> single = {'A'};
        uint32_t idx;
        auto encoded = BWTEncode(single, idx);
        auto decoded = BWTDecode(encoded, idx);
        test("BWT single byte", single == decoded);
    }

    // Test simple string
    {
        std::string s = "banana";
        std::vector<uint8_t> text(s.begin(), s.end());
        uint32_t idx;
        auto encoded = BWTEncode(text, idx);
        auto decoded = BWTDecode(encoded, idx);
        test("BWT banana", text == decoded);
    }

    // Test MTF
    {
        std::string s = "abracadabra";
        std::vector<uint8_t> text(s.begin(), s.end());
        auto encoded = MTFEncode(text);
        auto decoded = MTFDecode(encoded);
        test("MTF roundtrip", text == decoded);
    }
}

// Test LZ77 compression/decompression
void test_lz77() {
    std::cout << "\n=== LZ77 Tests ===\n";

    // Test empty
    {
        std::vector<uint8_t> empty;
        auto compressed = LZ77Compress(empty);
        auto decompressed = LZ77Decompress(compressed);
        test("LZ77 empty", decompressed.empty());
    }

    // Test repeated pattern
    {
        std::string s = "ABCABCABCABCABC";
        std::vector<uint8_t> text(s.begin(), s.end());
        auto compressed = LZ77Compress(text);
        auto decompressed = LZ77Decompress(compressed);
        test("LZ77 repeated pattern", text == decompressed);
    }

    // Test RLE
    {
        std::vector<uint8_t> runs(100, 'A');
        auto compressed = RLECompress(runs);
        auto decompressed = RLEDecompress(compressed);
        test("RLE runs", runs == decompressed);
    }

    // Test Delta
    {
        std::vector<uint8_t> seq;
        for (int i = 0; i < 100; i++) seq.push_back(i % 256);
        auto encoded = DeltaEncode(seq);
        auto decoded = DeltaDecode(encoded);
        test("Delta sequence", seq == decoded);
    }
}

// Test Hybrid compression
void test_hybrid() {
    std::cout << "\n=== Hybrid Tests ===\n";

    // Test empty
    {
        std::vector<uint8_t> empty;
        auto compressed = CompressHybrid(empty);
        auto decompressed = DecompressHybrid(compressed);
        test("Hybrid empty", decompressed.empty());
    }

    // Test single byte
    {
        std::vector<uint8_t> single = {'Z'};
        auto compressed = CompressHybrid(single);
        auto decompressed = DecompressHybrid(compressed);
        test("Hybrid single byte", single == decompressed);
    }

    // Test small text
    {
        std::string s = "Hello World";
        std::vector<uint8_t> text(s.begin(), s.end());
        auto compressed = CompressHybrid(text);
        auto decompressed = DecompressHybrid(compressed);
        test("Hybrid small text", text == decompressed);
    }

    // Test larger text
    {
        std::string base = "The quick brown fox jumps over the lazy dog. ";
        std::string s;
        for (int i = 0; i < 100; i++) s += base;
        std::vector<uint8_t> text(s.begin(), s.end());
        auto compressed = CompressHybrid(text);
        auto decompressed = DecompressHybrid(compressed);
        test("Hybrid large text", text == decompressed);
    }

    // Test binary data
    {
        std::vector<uint8_t> binary;
        for (int i = 0; i < 1000; i++) binary.push_back(i % 256);
        auto compressed = CompressHybrid(binary);
        auto decompressed = DecompressHybrid(compressed);
        test("Hybrid binary data", binary == decompressed);
    }

    // Test random-like data
    {
        std::vector<uint8_t> random;
        uint32_t seed = 12345;
        for (int i = 0; i < 1000; i++) {
            seed = seed * 1103515245 + 12345;
            random.push_back((seed >> 16) & 0xFF);
        }
        auto compressed = CompressHybrid(random);
        auto decompressed = DecompressHybrid(compressed);
        test("Hybrid random data", random == decompressed);
    }
}

// Test with real files
void test_files() {
    std::cout << "\n=== File Tests ===\n";

    std::vector<std::string> files = {
        "testdata/wikipedia_10k.txt",
        "testdata/json_data.json",
        "testdata/xml_data.xml",
        "testdata/csv_data.csv",
        "testdata/log_file.log",
        "testdata/english_50k.txt"
    };

    for (const auto& path : files) {
        auto data = read_file(path);
        if (data.empty()) {
            std::cout << "  [SKIP] " << path << " (not found)\n";
            continue;
        }

        auto compressed = CompressHybrid(data);
        auto decompressed = DecompressHybrid(compressed);
        test(path + " roundtrip", data == decompressed);
    }
}

int main() {
    std::cout << "kcomp Test Suite\n";
    std::cout << "================\n";

    test_ppm();
    test_bwt();
    test_lz77();
    test_hybrid();
    test_files();

    std::cout << "\n================\n";
    std::cout << "Results: " << tests_passed << " passed, " << tests_failed << " failed\n";

    return tests_failed > 0 ? 1 : 0;
}
