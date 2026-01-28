#include <iostream>
#include <vector>
#include <cstdint>
#include <string>
#include <cstring>
#include "../src/models/ppm.hpp"
#include "../src/models/bwt.hpp"

int passed = 0, failed = 0;

void test(const std::string& name, bool condition) {
    if (condition) {
        std::cout << "  [PASS] " << name << "\n";
        passed++;
    } else {
        std::cout << "  [FAIL] " << name << "\n";
        failed++;
    }
}

std::vector<uint8_t> make_test_data(size_t size) {
    std::vector<uint8_t> data;
    data.reserve(size);
    for (size_t i = 0; i < size; i++) {
        data.push_back("The quick brown fox jumps over the lazy dog. "[i % 46]);
    }
    return data;
}

void test_data_integrity() {
    std::cout << "\n=== Data Integrity Tests ===\n";

    std::vector<size_t> sizes = {100, 500, 1000, 5000, 10000, 50000};

    for (size_t size : sizes) {
        auto original = make_test_data(size);
        auto compressed = CompressHybrid(original);
        auto decompressed = DecompressHybrid(compressed);

        bool size_match = (original.size() == decompressed.size());
        bool content_match = (original == decompressed);

        test("Size " + std::to_string(size) + " integrity", size_match && content_match);

        if (!content_match && size <= 1000) {
            int diff_count = 0;
            for (size_t i = 0; i < std::min(original.size(), decompressed.size()); i++) {
                if (original[i] != decompressed[i]) {
                    diff_count++;
                    if (diff_count <= 5) {
                        std::cout << "    Diff at " << i << ": expected "
                                  << (int)original[i] << " got " << (int)decompressed[i] << "\n";
                    }
                }
            }
            if (diff_count > 5) {
                std::cout << "    ... and " << (diff_count - 5) << " more differences\n";
            }
        }
    }
}

void test_byte_values_preserved() {
    std::cout << "\n=== Byte Value Preservation Tests ===\n";

    // Test all possible byte values
    {
        std::vector<uint8_t> all_bytes;
        for (int i = 0; i < 256; i++) {
            all_bytes.push_back(i);
        }
        // Repeat to make it larger
        for (int r = 0; r < 10; r++) {
            for (int i = 0; i < 256; i++) {
                all_bytes.push_back(i);
            }
        }

        auto c = CompressHybrid(all_bytes);
        auto d = DecompressHybrid(c);

        // Check all bytes are present
        std::vector<bool> found(256, false);
        for (uint8_t b : d) found[b] = true;

        bool all_found = true;
        for (int i = 0; i < 256; i++) {
            if (!found[i]) {
                all_found = false;
                std::cout << "    Missing byte value: " << i << "\n";
            }
        }
        test("All 256 byte values preserved", all_found && all_bytes == d);
    }

    // Test specific problematic byte values
    std::vector<uint8_t> special = {0x00, 0x01, 0x7F, 0x80, 0xFE, 0xFF};
    for (uint8_t b : special) {
        std::vector<uint8_t> data(100, b);
        auto c = CompressHybrid(data);
        auto d = DecompressHybrid(c);
        test("Byte 0x" + std::to_string(b) + " preserved", data == d);
    }
}

void test_boundary_corruption() {
    std::cout << "\n=== Boundary Corruption Tests ===\n";

    // Test data at exact power-of-2 boundaries
    std::vector<size_t> boundaries = {
        255, 256, 257,
        511, 512, 513,
        1023, 1024, 1025,
        4095, 4096, 4097,
        8191, 8192, 8193,
        16383, 16384, 16385,
        32767, 32768, 32769,
        65535, 65536, 65537
    };

    for (size_t size : boundaries) {
        auto original = make_test_data(size);
        auto compressed = CompressHybrid(original);
        auto decompressed = DecompressHybrid(compressed);

        test("Boundary size " + std::to_string(size), original == decompressed);
    }
}

void test_sequential_corruption() {
    std::cout << "\n=== Sequential Corruption Tests ===\n";

    // Compress and decompress many times in sequence
    auto data = make_test_data(5000);

    for (int i = 0; i < 50; i++) {
        auto c = CompressHybrid(data);
        auto d = DecompressHybrid(c);

        if (data != d) {
            test("Sequential test " + std::to_string(i), false);
            return;
        }
    }
    test("50 sequential compressions", true);
}

void test_bwt_corruption() {
    std::cout << "\n=== BWT Specific Corruption Tests ===\n";

    // Test sizes that previously caused BWT issues
    std::vector<int> problematic_sizes = {
        46, 92, 138, 184, 230,  // Multiples of pattern length
        999, 1000, 1001,        // Around the size that had cycle bugs
        45, 47, 91, 93          // Off-by-one from pattern length
    };

    for (int size : problematic_sizes) {
        auto data = make_test_data(size);

        uint32_t idx;
        auto bwt = BWTEncode(data, idx);
        auto decoded = BWTDecode(bwt, idx);

        test("BWT size " + std::to_string(size), data == decoded);
    }
}

void test_mtf_corruption() {
    std::cout << "\n=== MTF Specific Corruption Tests ===\n";

    // Test MTF with various patterns
    {
        std::vector<uint8_t> data = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        auto e = MTFEncode(data);
        auto d = MTFDecode(e);
        test("MTF sequential", data == d);
    }

    {
        std::vector<uint8_t> data = {9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
        auto e = MTFEncode(data);
        auto d = MTFDecode(e);
        test("MTF reverse", data == d);
    }

    {
        std::vector<uint8_t> data;
        for (int i = 0; i < 1000; i++) data.push_back(i % 256);
        auto e = MTFEncode(data);
        auto d = MTFDecode(e);
        test("MTF all bytes", data == d);
    }
}

void test_compression_ratio_sanity() {
    std::cout << "\n=== Compression Ratio Sanity Tests ===\n";

    // Highly compressible data should compress well
    {
        std::vector<uint8_t> runs(10000, 'A');
        auto c = CompressHybrid(runs);
        double ratio = 100.0 * c.size() / runs.size();
        test("Runs compress well (<10%)", ratio < 10.0);
        std::cout << "    Ratio: " << ratio << "%\n";
    }

    // Random data should not compress much
    {
        std::vector<uint8_t> random;
        uint32_t seed = 12345;
        for (int i = 0; i < 10000; i++) {
            seed = seed * 1103515245 + 12345;
            random.push_back((seed >> 16) & 0xFF);
        }
        auto c = CompressHybrid(random);
        double ratio = 100.0 * c.size() / random.size();
        test("Random doesn't expand much (<105%)", ratio < 105.0);
        std::cout << "    Ratio: " << ratio << "%\n";

        // But it should still decompress correctly
        auto d = DecompressHybrid(c);
        test("Random decompresses correctly", random == d);
    }

    // Text should compress reasonably
    {
        auto text = make_test_data(10000);
        auto c = CompressHybrid(text);
        double ratio = 100.0 * c.size() / text.size();
        test("Text compresses (<50%)", ratio < 50.0);
        std::cout << "    Ratio: " << ratio << "%\n";
    }
}

void test_empty_and_minimal() {
    std::cout << "\n=== Empty and Minimal Data Tests ===\n";

    // Empty
    {
        std::vector<uint8_t> empty;
        auto c = CompressHybrid(empty);
        auto d = DecompressHybrid(c);
        test("Empty data", d.empty());
    }

    // Single byte
    for (int b = 0; b < 256; b += 51) {
        std::vector<uint8_t> single = {(uint8_t)b};
        auto c = CompressHybrid(single);
        auto d = DecompressHybrid(c);
        test("Single byte 0x" + std::to_string(b), single == d);
    }

    // Two bytes - all combinations of 0, 127, 255
    std::vector<uint8_t> vals = {0, 127, 255};
    for (uint8_t a : vals) {
        for (uint8_t b : vals) {
            std::vector<uint8_t> data = {a, b};
            auto c = CompressHybrid(data);
            auto d = DecompressHybrid(c);
            test("Two bytes " + std::to_string(a) + "," + std::to_string(b), data == d);
        }
    }
}

int main() {
    std::cout << "=== Corruption Detection Tests ===\n";

    test_empty_and_minimal();
    test_data_integrity();
    test_byte_values_preserved();
    test_boundary_corruption();
    test_bwt_corruption();
    test_mtf_corruption();
    test_sequential_corruption();
    test_compression_ratio_sanity();

    std::cout << "\n=== Results: " << passed << " passed, " << failed << " failed ===\n";
    return failed > 0 ? 1 : 0;
}
