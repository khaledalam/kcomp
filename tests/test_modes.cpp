#include <iostream>
#include <vector>
#include <cstdint>
#include <string>
#include "../src/models/ppm.hpp"
#include "../src/models/bwt.hpp"
#include "../src/models/lz77.hpp"
#include "../src/models/lzopt.hpp"
#include "../src/models/lzx.hpp"
#include "../src/models/lzma.hpp"
#include "../src/models/cm.hpp"
#include "../src/models/rle.hpp"

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

std::vector<uint8_t> make_test_data(int size, int pattern_type) {
    std::vector<uint8_t> data;
    data.reserve(size);

    switch (pattern_type) {
        case 0: // Repeated text
            for (int i = 0; i < size; i++) {
                data.push_back("The quick brown fox jumps over the lazy dog. "[i % 46]);
            }
            break;
        case 1: // Sequential bytes
            for (int i = 0; i < size; i++) {
                data.push_back(i % 256);
            }
            break;
        case 2: // Random-like (LCG)
            {
                uint32_t seed = 42;
                for (int i = 0; i < size; i++) {
                    seed = seed * 1103515245 + 12345;
                    data.push_back((seed >> 16) & 0xFF);
                }
            }
            break;
        case 3: // Runs of same byte
            for (int i = 0; i < size; i++) {
                data.push_back((i / 10) % 256);
            }
            break;
        case 4: // Binary with zeros
            for (int i = 0; i < size; i++) {
                data.push_back(i % 8 == 0 ? 0 : (i * 7) % 256);
            }
            break;
    }
    return data;
}

void test_ppm_orders() {
    std::cout << "\n=== PPM Order Tests ===\n";

    auto data = make_test_data(5000, 0);

    // PPM1
    {
        auto c = CompressPPM1(data);
        auto d = DecompressPPM1(c);
        test("PPM1 roundtrip", data == d);
    }

    // PPM2
    {
        auto c = CompressPPM2(data);
        auto d = DecompressPPM2(c);
        test("PPM2 roundtrip", data == d);
    }

    // PPM3
    {
        auto c = CompressPPM3(data);
        auto d = DecompressPPM3(c);
        test("PPM3 roundtrip", data == d);
    }

    // PPM4
    {
        auto c = CompressPPM4(data);
        auto d = DecompressPPM4(c);
        test("PPM4 roundtrip", data == d);
    }

    // PPM5
    {
        auto c = CompressPPM5(data);
        auto d = DecompressPPM5(c);
        test("PPM5 roundtrip", data == d);
    }

    // PPM6
    {
        auto c = CompressPPM6(data);
        auto d = DecompressPPM6(c);
        test("PPM6 roundtrip", data == d);
    }
}

void test_lz_variants() {
    std::cout << "\n=== LZ Variant Tests ===\n";

    auto data = make_test_data(10000, 0);

    // LZ77
    {
        auto c = LZ77Compress(data);
        auto d = LZ77Decompress(c);
        test("LZ77 roundtrip", data == d);
    }

    // LZOpt
    {
        auto c = LZOptCompress(data);
        auto d = LZOptDecompress(c);
        test("LZOpt roundtrip", data == d);
    }

    // LZX
    {
        auto c = LZXCompress(data);
        auto d = LZXDecompress(c);
        test("LZX roundtrip", data == d);
    }

    // LZMA
    {
        auto c = LZMACompress(data);
        auto d = LZMADecompress(c);
        test("LZMA roundtrip", data == d);
    }
}

void test_bwt_mtf() {
    std::cout << "\n=== BWT+MTF Tests ===\n";

    std::vector<int> sizes = {100, 500, 1000, 2000, 5000};

    for (int size : sizes) {
        auto data = make_test_data(size, 0);

        uint32_t idx;
        auto bwt = BWTEncode(data, idx);
        auto mtf = MTFEncode(bwt);
        auto mtf_dec = MTFDecode(mtf);
        auto orig = BWTDecode(mtf_dec, idx);

        test("BWT+MTF size=" + std::to_string(size), data == orig);
    }
}

void test_hybrid_modes() {
    std::cout << "\n=== Hybrid Mode Selection Tests ===\n";

    // Test that hybrid picks appropriate modes for different data types

    // Highly repetitive - should pick RLE or similar
    {
        std::vector<uint8_t> rep(1000, 'A');
        auto c = CompressHybrid(rep);
        auto d = DecompressHybrid(c);
        test("Hybrid repetitive data", rep == d);
    }

    // Text data
    {
        auto text = make_test_data(5000, 0);
        auto c = CompressHybrid(text);
        auto d = DecompressHybrid(c);
        test("Hybrid text data", text == d);
    }

    // Sequential bytes (good for delta)
    {
        auto seq = make_test_data(1000, 1);
        auto c = CompressHybrid(seq);
        auto d = DecompressHybrid(c);
        test("Hybrid sequential data", seq == d);
    }

    // Random data
    {
        auto rnd = make_test_data(1000, 2);
        auto c = CompressHybrid(rnd);
        auto d = DecompressHybrid(c);
        test("Hybrid random data", rnd == d);
    }

    // Data with runs
    {
        auto runs = make_test_data(1000, 3);
        auto c = CompressHybrid(runs);
        auto d = DecompressHybrid(c);
        test("Hybrid runs data", runs == d);
    }

    // Binary with zeros (sparse)
    {
        auto sparse = make_test_data(1000, 4);
        auto c = CompressHybrid(sparse);
        auto d = DecompressHybrid(c);
        test("Hybrid sparse data", sparse == d);
    }
}

void test_cm() {
    std::cout << "\n=== Context Mixing Tests ===\n";

    // Small data for CM (it's slow)
    std::vector<int> sizes = {100, 500, 1000};

    for (int size : sizes) {
        auto data = make_test_data(size, 0);
        auto c = CompressCM(data);
        auto d = DecompressCM(c);
        test("CM size=" + std::to_string(size), data == d);
    }
}

int main() {
    std::cout << "=== Compression Mode Tests ===\n";

    test_ppm_orders();
    test_lz_variants();
    test_bwt_mtf();
    test_hybrid_modes();
    test_cm();

    std::cout << "\n=== Results: " << passed << " passed, " << failed << " failed ===\n";
    return failed > 0 ? 1 : 0;
}
