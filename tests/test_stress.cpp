#include <iostream>
#include <vector>
#include <cstdint>
#include <string>
#include <chrono>
#include "../src/models/ppm.hpp"
#include "../src/models/bwt.hpp"
#include "../src/models/lz77.hpp"

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

std::vector<uint8_t> generate_data(size_t size, int type) {
    std::vector<uint8_t> data;
    data.reserve(size);

    uint32_t seed = 42;
    auto next_rand = [&]() {
        seed = seed * 1103515245 + 12345;
        return (seed >> 16) & 0xFF;
    };

    switch (type) {
        case 0: // Random
            for (size_t i = 0; i < size; i++) data.push_back(next_rand());
            break;
        case 1: // Text-like
            for (size_t i = 0; i < size; i++) {
                data.push_back("The quick brown fox jumps over the lazy dog. "[i % 46]);
            }
            break;
        case 2: // Binary with structure
            for (size_t i = 0; i < size; i++) {
                if (i % 512 < 8) data.push_back(0x7F);  // Header
                else data.push_back((i * 7) % 256);
            }
            break;
        case 3: // High entropy
            for (size_t i = 0; i < size; i++) data.push_back(next_rand());
            break;
        case 4: // Low entropy (lots of zeros)
            for (size_t i = 0; i < size; i++) {
                data.push_back(next_rand() < 200 ? 0 : next_rand());
            }
            break;
    }
    return data;
}

void test_large_sizes() {
    std::cout << "\n=== Large Size Tests ===\n";

    std::vector<size_t> sizes = {
        10 * 1024,      // 10 KB
        50 * 1024,      // 50 KB
        100 * 1024,     // 100 KB
        500 * 1024,     // 500 KB
    };

    for (size_t size : sizes) {
        auto data = generate_data(size, 1);  // Text-like

        auto start = std::chrono::high_resolution_clock::now();
        auto compressed = CompressHybrid(data);
        auto compress_time = std::chrono::high_resolution_clock::now();

        auto decompressed = DecompressHybrid(compressed);
        auto decompress_time = std::chrono::high_resolution_clock::now();

        auto ct = std::chrono::duration_cast<std::chrono::milliseconds>(compress_time - start).count();
        auto dt = std::chrono::duration_cast<std::chrono::milliseconds>(decompress_time - compress_time).count();

        double ratio = 100.0 * compressed.size() / data.size();

        std::string name = std::to_string(size / 1024) + "KB";
        test(name + " roundtrip (" + std::to_string((int)ratio) + "%, " +
             std::to_string(ct) + "ms/" + std::to_string(dt) + "ms)",
             data == decompressed);
    }
}

void test_many_small() {
    std::cout << "\n=== Many Small Files Test ===\n";

    int count = 100;
    int success = 0;

    for (int i = 0; i < count; i++) {
        size_t size = 100 + (i * 50);  // 100 to 5100 bytes
        auto data = generate_data(size, i % 5);

        auto compressed = CompressHybrid(data);
        auto decompressed = DecompressHybrid(compressed);

        if (data == decompressed) success++;
    }

    test("100 files (100-5100 bytes)", success == count);
    std::cout << "    " << success << "/" << count << " succeeded\n";
}

void test_repeated_compression() {
    std::cout << "\n=== Repeated Compression Test ===\n";

    auto data = generate_data(10000, 1);

    // Compress and decompress multiple times
    int iterations = 10;
    bool all_ok = true;

    for (int i = 0; i < iterations; i++) {
        auto compressed = CompressHybrid(data);
        auto decompressed = DecompressHybrid(compressed);

        if (data != decompressed) {
            all_ok = false;
            std::cout << "    Failed at iteration " << i << "\n";
            break;
        }
    }

    test("10 iterations", all_ok);
}

void test_incremental_sizes() {
    std::cout << "\n=== Incremental Size Tests ===\n";

    int success = 0;
    int total = 0;

    // Test every size from 1 to 500
    for (int size = 1; size <= 500; size++) {
        std::vector<uint8_t> data;
        for (int i = 0; i < size; i++) {
            data.push_back("abcdefghij"[i % 10]);
        }

        auto c = CompressHybrid(data);
        auto d = DecompressHybrid(c);

        if (data == d) success++;
        total++;
    }

    test("Sizes 1-500", success == total);
    std::cout << "    " << success << "/" << total << " succeeded\n";
}

void test_all_byte_values() {
    std::cout << "\n=== All Byte Values Tests ===\n";

    // Test that all 256 byte values survive compression
    for (int dominant = 0; dominant < 256; dominant += 51) {
        std::vector<uint8_t> data(1000, dominant);
        // Sprinkle in some other values
        for (int i = 0; i < 100; i++) {
            data[i * 10] = (dominant + i) % 256;
        }

        auto c = CompressHybrid(data);
        auto d = DecompressHybrid(c);

        test("Dominant byte " + std::to_string(dominant), data == d);
    }
}

void test_worst_case_patterns() {
    std::cout << "\n=== Worst Case Pattern Tests ===\n";

    // Incompressible random data
    {
        auto data = generate_data(10000, 3);  // High entropy
        auto c = CompressHybrid(data);
        auto d = DecompressHybrid(c);
        test("Random 10KB", data == d);
    }

    // Maximum expansion case (compression makes it bigger)
    {
        std::vector<uint8_t> data;
        uint32_t seed = 999;
        for (int i = 0; i < 1000; i++) {
            seed = seed * 1103515245 + 12345;
            data.push_back((seed >> 16) & 0xFF);
        }
        auto c = CompressHybrid(data);
        auto d = DecompressHybrid(c);
        test("Incompressible 1KB", data == d);
    }

    // Pathological BWT case (size 1000 with repeating pattern - was buggy)
    {
        std::vector<uint8_t> data;
        const char* pattern = "The quick brown fox jumps over the lazy dog. ";
        for (int i = 0; i < 1000; i++) {
            data.push_back(pattern[i % 46]);
        }
        auto c = CompressHybrid(data);
        auto d = DecompressHybrid(c);
        test("BWT pathological (size 1000)", data == d);
    }

    // Many unique short strings
    {
        std::vector<uint8_t> data;
        for (int i = 0; i < 1000; i++) {
            data.push_back('A' + (i % 26));
            data.push_back('0' + (i % 10));
            data.push_back(' ');
        }
        auto c = CompressHybrid(data);
        auto d = DecompressHybrid(c);
        test("Many unique short strings", data == d);
    }
}

void test_ppm_orders_stress() {
    std::cout << "\n=== PPM Order Stress Tests ===\n";

    auto data = generate_data(20000, 1);

    // Test all PPM orders with larger data
    {
        auto c = CompressPPM1(data);
        auto d = DecompressPPM1(c);
        test("PPM1 20KB", data == d);
    }
    {
        auto c = CompressPPM2(data);
        auto d = DecompressPPM2(c);
        test("PPM2 20KB", data == d);
    }
    {
        auto c = CompressPPM3(data);
        auto d = DecompressPPM3(c);
        test("PPM3 20KB", data == d);
    }
    {
        auto c = CompressPPM4(data);
        auto d = DecompressPPM4(c);
        test("PPM4 20KB", data == d);
    }
    {
        auto c = CompressPPM5(data);
        auto d = DecompressPPM5(c);
        test("PPM5 20KB", data == d);
    }
    {
        auto c = CompressPPM6(data);
        auto d = DecompressPPM6(c);
        test("PPM6 20KB", data == d);
    }
}

void test_bwt_sizes() {
    std::cout << "\n=== BWT Size Stress Tests ===\n";

    // Test BWT with various sizes including problematic ones
    std::vector<int> sizes = {
        100, 200, 500,
        999, 1000, 1001,  // Around the problematic size
        2000, 5000, 10000,
        15000, 20000
    };

    for (int size : sizes) {
        std::vector<uint8_t> data;
        const char* pattern = "The quick brown fox jumps over the lazy dog. ";
        for (int i = 0; i < size; i++) {
            data.push_back(pattern[i % 46]);
        }

        uint32_t idx;
        auto bwt = BWTEncode(data, idx);
        auto decoded = BWTDecode(bwt, idx);

        test("BWT size=" + std::to_string(size), data == decoded);
    }
}

int main() {
    std::cout << "=== Stress Tests ===\n";

    test_incremental_sizes();
    test_all_byte_values();
    test_worst_case_patterns();
    test_bwt_sizes();
    test_ppm_orders_stress();
    test_many_small();
    test_repeated_compression();
    test_large_sizes();

    std::cout << "\n=== Results: " << passed << " passed, " << failed << " failed ===\n";
    return failed > 0 ? 1 : 0;
}
