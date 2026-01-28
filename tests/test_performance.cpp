#include <iostream>
#include <vector>
#include <cstdint>
#include <string>
#include <chrono>
#include "../src/models/ppm.hpp"
#include "../src/models/bwt.hpp"
#include "../src/models/lz77.hpp"
#include "../src/models/lzopt.hpp"
#include "../src/models/lzx.hpp"

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

std::vector<uint8_t> make_text_data(size_t size) {
    std::vector<uint8_t> data;
    data.reserve(size);
    const char* text = "The quick brown fox jumps over the lazy dog. ";
    size_t len = 46;
    for (size_t i = 0; i < size; i++) {
        data.push_back(text[i % len]);
    }
    return data;
}

std::vector<uint8_t> make_random_data(size_t size) {
    std::vector<uint8_t> data;
    data.reserve(size);
    uint32_t seed = 42;
    for (size_t i = 0; i < size; i++) {
        seed = seed * 1103515245 + 12345;
        data.push_back((seed >> 16) & 0xFF);
    }
    return data;
}

void test_compression_ratio() {
    std::cout << "\n=== Compression Ratio Tests ===\n";

    // Text data should compress well
    {
        auto data = make_text_data(10000);
        auto c = CompressHybrid(data);
        double ratio = 100.0 * c.size() / data.size();
        test("Text 10KB compresses below 40%", ratio < 40.0);
        std::cout << "    Actual ratio: " << ratio << "%\n";
    }

    // Repeated single byte should compress extremely well
    {
        std::vector<uint8_t> data(10000, 'A');
        auto c = CompressHybrid(data);
        double ratio = 100.0 * c.size() / data.size();
        test("Repeated byte compresses below 1%", ratio < 1.0);
        std::cout << "    Actual ratio: " << ratio << "%\n";
    }

    // Random data should not expand significantly
    {
        auto data = make_random_data(10000);
        auto c = CompressHybrid(data);
        double ratio = 100.0 * c.size() / data.size();
        test("Random data doesn't expand beyond 102%", ratio < 102.0);
        std::cout << "    Actual ratio: " << ratio << "%\n";
    }

    // Small file overhead check
    {
        std::vector<uint8_t> data = {'H', 'e', 'l', 'l', 'o'};
        auto c = CompressHybrid(data);
        // Small files have overhead but shouldn't be huge
        test("Small file overhead reasonable", c.size() < 50);
        std::cout << "    5 bytes -> " << c.size() << " bytes\n";
    }
}

void test_ppm_order_effectiveness() {
    std::cout << "\n=== PPM Order Effectiveness Tests ===\n";

    auto data = make_text_data(50000);

    std::vector<std::pair<std::string, size_t>> results;

    auto c1 = CompressPPM1(data);
    results.push_back({"PPM1", c1.size()});

    auto c2 = CompressPPM2(data);
    results.push_back({"PPM2", c2.size()});

    auto c3 = CompressPPM3(data);
    results.push_back({"PPM3", c3.size()});

    auto c5 = CompressPPM5(data);
    results.push_back({"PPM5", c5.size()});

    auto c6 = CompressPPM6(data);
    results.push_back({"PPM6", c6.size()});

    // Higher orders should generally compress better for text
    std::cout << "    Original: " << data.size() << " bytes\n";
    for (auto& [name, size] : results) {
        double ratio = 100.0 * size / data.size();
        std::cout << "    " << name << ": " << size << " bytes (" << ratio << "%)\n";
    }

    // PPM3 should beat PPM1 for text
    test("PPM3 beats PPM1", c3.size() < c1.size());

    // All should decompress correctly
    test("PPM1 decompresses", DecompressPPM1(c1) == data);
    test("PPM2 decompresses", DecompressPPM2(c2) == data);
    test("PPM3 decompresses", DecompressPPM3(c3) == data);
    test("PPM5 decompresses", DecompressPPM5(c5) == data);
    test("PPM6 decompresses", DecompressPPM6(c6) == data);
}

void test_lz_effectiveness() {
    std::cout << "\n=== LZ Effectiveness Tests ===\n";

    // Data with repeated patterns - good for LZ
    std::string pattern = "ABCDEFGHIJ";
    std::string s;
    for (int i = 0; i < 1000; i++) s += pattern;
    std::vector<uint8_t> data(s.begin(), s.end());

    auto lz77 = LZ77Compress(data);
    auto lzopt = LZOptCompress(data);
    auto lzx = LZXCompress(data);

    std::cout << "    Original: " << data.size() << " bytes\n";
    std::cout << "    LZ77: " << lz77.size() << " bytes (" << (100.0 * lz77.size() / data.size()) << "%)\n";
    std::cout << "    LZOpt: " << lzopt.size() << " bytes (" << (100.0 * lzopt.size() / data.size()) << "%)\n";
    std::cout << "    LZX: " << lzx.size() << " bytes (" << (100.0 * lzx.size() / data.size()) << "%)\n";

    // All LZ variants should compress repeated patterns well
    test("LZ77 compresses repeated pattern", lz77.size() < data.size() / 2);
    test("LZOpt compresses repeated pattern", lzopt.size() < data.size() / 2);

    // Verify decompression
    test("LZ77 decompresses", LZ77Decompress(lz77) == data);
    test("LZOpt decompresses", LZOptDecompress(lzopt) == data);
    test("LZX decompresses", LZXDecompress(lzx) == data);
}

void test_bwt_effectiveness() {
    std::cout << "\n=== BWT Effectiveness Tests ===\n";

    auto data = make_text_data(10000);

    uint32_t idx;
    auto bwt = BWTEncode(data, idx);
    auto mtf = MTFEncode(bwt);

    // MTF after BWT should have many zeros and small values
    int zeros = 0, small_vals = 0;
    for (uint8_t b : mtf) {
        if (b == 0) zeros++;
        if (b < 16) small_vals++;
    }

    double zero_ratio = 100.0 * zeros / mtf.size();
    double small_ratio = 100.0 * small_vals / mtf.size();

    std::cout << "    MTF zeros: " << zeros << " (" << zero_ratio << "%)\n";
    std::cout << "    MTF small (<16): " << small_vals << " (" << small_ratio << "%)\n";

    test("BWT+MTF produces many zeros (>30%)", zero_ratio > 30.0);
    test("BWT+MTF produces many small values (>70%)", small_ratio > 70.0);

    // Verify roundtrip
    auto decoded_mtf = MTFDecode(mtf);
    auto decoded = BWTDecode(decoded_mtf, idx);
    test("BWT+MTF roundtrip", data == decoded);
}

void test_hybrid_selection() {
    std::cout << "\n=== Hybrid Mode Selection Tests ===\n";

    // Different data types should potentially trigger different modes

    // Highly repetitive
    {
        std::vector<uint8_t> data(5000, 'X');
        auto c = CompressHybrid(data);
        double ratio = 100.0 * c.size() / data.size();
        std::cout << "    Repetitive: mode " << (int)c[0] << ", ratio " << ratio << "%\n";
        test("Repetitive data very compressible", ratio < 1.0);
    }

    // Text
    {
        auto data = make_text_data(5000);
        auto c = CompressHybrid(data);
        double ratio = 100.0 * c.size() / data.size();
        std::cout << "    Text: mode " << (int)c[0] << ", ratio " << ratio << "%\n";
        test("Text data compressible", ratio < 50.0);
    }

    // Binary sequential
    {
        std::vector<uint8_t> data;
        for (int i = 0; i < 5000; i++) data.push_back(i % 256);
        auto c = CompressHybrid(data);
        double ratio = 100.0 * c.size() / data.size();
        std::cout << "    Sequential binary: mode " << (int)c[0] << ", ratio " << ratio << "%\n";
        auto d = DecompressHybrid(c);
        test("Sequential binary roundtrip", data == d);
    }

    // Random
    {
        auto data = make_random_data(5000);
        auto c = CompressHybrid(data);
        double ratio = 100.0 * c.size() / data.size();
        std::cout << "    Random: mode " << (int)c[0] << ", ratio " << ratio << "%\n";
        auto d = DecompressHybrid(c);
        test("Random data roundtrip", data == d);
    }
}

void test_scaling() {
    std::cout << "\n=== Scaling Tests ===\n";

    std::vector<size_t> sizes = {1000, 5000, 10000, 50000, 100000};

    for (size_t size : sizes) {
        auto data = make_text_data(size);

        auto start = std::chrono::high_resolution_clock::now();
        auto c = CompressHybrid(data);
        auto compress_time = std::chrono::high_resolution_clock::now();
        auto d = DecompressHybrid(c);
        auto decompress_time = std::chrono::high_resolution_clock::now();

        auto ct = std::chrono::duration_cast<std::chrono::milliseconds>(compress_time - start).count();
        auto dt = std::chrono::duration_cast<std::chrono::milliseconds>(decompress_time - compress_time).count();

        double ratio = 100.0 * c.size() / data.size();

        bool correct = (data == d);
        test("Size " + std::to_string(size / 1000) + "KB", correct);
        std::cout << "    Ratio: " << ratio << "%, compress: " << ct << "ms, decompress: " << dt << "ms\n";
    }
}

int main() {
    std::cout << "=== Performance Characteristics Tests ===\n";

    test_compression_ratio();
    test_ppm_order_effectiveness();
    test_lz_effectiveness();
    test_bwt_effectiveness();
    test_hybrid_selection();
    test_scaling();

    std::cout << "\n=== Results: " << passed << " passed, " << failed << " failed ===\n";
    return failed > 0 ? 1 : 0;
}
