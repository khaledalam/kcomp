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

void test_empty_data() {
    std::cout << "\n=== Empty Data Tests ===\n";

    std::vector<uint8_t> empty;

    test("Hybrid empty", DecompressHybrid(CompressHybrid(empty)).empty());
    test("PPM5 empty", DecompressPPM5(CompressPPM5(empty)).empty());

    uint32_t idx;
    auto bwt = BWTEncode(empty, idx);
    test("BWT empty", BWTDecode(bwt, idx).empty());

    test("MTF empty", MTFDecode(MTFEncode(empty)).empty());
}

void test_single_byte() {
    std::cout << "\n=== Single Byte Tests ===\n";

    for (int b = 0; b < 256; b += 51) {  // Test several byte values
        std::vector<uint8_t> single = {(uint8_t)b};

        auto c = CompressHybrid(single);
        auto d = DecompressHybrid(c);
        test("Hybrid byte=" + std::to_string(b), single == d);
    }
}

void test_two_bytes() {
    std::cout << "\n=== Two Byte Tests ===\n";

    std::vector<std::pair<uint8_t, uint8_t>> pairs = {
        {0, 0}, {0, 255}, {255, 0}, {255, 255},
        {'A', 'B'}, {0x00, 0x01}, {0xFE, 0xFF}
    };

    for (auto& p : pairs) {
        std::vector<uint8_t> data = {p.first, p.second};
        auto c = CompressHybrid(data);
        auto d = DecompressHybrid(c);
        test("Hybrid pair=" + std::to_string(p.first) + "," + std::to_string(p.second),
             data == d);
    }
}

void test_all_same_byte() {
    std::cout << "\n=== All Same Byte Tests ===\n";

    std::vector<int> sizes = {1, 2, 10, 100, 1000, 10000};

    for (int size : sizes) {
        std::vector<uint8_t> data(size, 'X');
        auto c = CompressHybrid(data);
        auto d = DecompressHybrid(c);
        test("All same size=" + std::to_string(size), data == d);
    }

    // All zeros
    {
        std::vector<uint8_t> zeros(5000, 0);
        auto c = CompressHybrid(zeros);
        auto d = DecompressHybrid(c);
        test("All zeros 5000", zeros == d);
    }

    // All 0xFF
    {
        std::vector<uint8_t> ones(5000, 0xFF);
        auto c = CompressHybrid(ones);
        auto d = DecompressHybrid(c);
        test("All 0xFF 5000", ones == d);
    }
}

void test_all_unique_bytes() {
    std::cout << "\n=== All Unique Bytes Tests ===\n";

    // Every byte value exactly once
    {
        std::vector<uint8_t> data;
        for (int i = 0; i < 256; i++) data.push_back(i);
        auto c = CompressHybrid(data);
        auto d = DecompressHybrid(c);
        test("All 256 unique bytes", data == d);
    }

    // Reverse order
    {
        std::vector<uint8_t> data;
        for (int i = 255; i >= 0; i--) data.push_back(i);
        auto c = CompressHybrid(data);
        auto d = DecompressHybrid(c);
        test("256 bytes descending", data == d);
    }
}

void test_boundary_sizes() {
    std::cout << "\n=== Boundary Size Tests ===\n";

    std::vector<int> sizes = {
        255, 256, 257,
        511, 512, 513,
        1023, 1024, 1025,
        4095, 4096, 4097,
        65535, 65536, 65537
    };

    for (int size : sizes) {
        std::vector<uint8_t> data;
        for (int i = 0; i < size; i++) {
            data.push_back("abcdefghij"[i % 10]);
        }
        auto c = CompressHybrid(data);
        auto d = DecompressHybrid(c);
        test("Size=" + std::to_string(size), data == d);
    }
}

void test_special_patterns() {
    std::cout << "\n=== Special Pattern Tests ===\n";

    // Alternating 0/1
    {
        std::vector<uint8_t> alt;
        for (int i = 0; i < 1000; i++) alt.push_back(i % 2);
        auto c = CompressHybrid(alt);
        auto d = DecompressHybrid(c);
        test("Alternating 0/1", alt == d);
    }

    // Alternating 0/255
    {
        std::vector<uint8_t> alt;
        for (int i = 0; i < 1000; i++) alt.push_back(i % 2 ? 0 : 255);
        auto c = CompressHybrid(alt);
        auto d = DecompressHybrid(c);
        test("Alternating 0/255", alt == d);
    }

    // Fibonacci-like pattern
    {
        std::vector<uint8_t> fib = {1, 1};
        for (int i = 2; i < 1000; i++) {
            fib.push_back((fib[i-1] + fib[i-2]) % 256);
        }
        auto c = CompressHybrid(fib);
        auto d = DecompressHybrid(c);
        test("Fibonacci pattern", fib == d);
    }

    // Sawtooth
    {
        std::vector<uint8_t> saw;
        for (int i = 0; i < 1000; i++) saw.push_back(i % 100);
        auto c = CompressHybrid(saw);
        auto d = DecompressHybrid(c);
        test("Sawtooth pattern", saw == d);
    }

    // Square wave
    {
        std::vector<uint8_t> sq;
        for (int i = 0; i < 1000; i++) sq.push_back((i / 50) % 2 ? 200 : 50);
        auto c = CompressHybrid(sq);
        auto d = DecompressHybrid(c);
        test("Square wave", sq == d);
    }
}

void test_text_variations() {
    std::cout << "\n=== Text Variation Tests ===\n";

    // Only lowercase
    {
        std::string s;
        for (int i = 0; i < 1000; i++) s += 'a' + (i % 26);
        std::vector<uint8_t> data(s.begin(), s.end());
        auto c = CompressHybrid(data);
        auto d = DecompressHybrid(c);
        test("Lowercase alphabet", data == d);
    }

    // Only uppercase
    {
        std::string s;
        for (int i = 0; i < 1000; i++) s += 'A' + (i % 26);
        std::vector<uint8_t> data(s.begin(), s.end());
        auto c = CompressHybrid(data);
        auto d = DecompressHybrid(c);
        test("Uppercase alphabet", data == d);
    }

    // Only digits
    {
        std::string s;
        for (int i = 0; i < 1000; i++) s += '0' + (i % 10);
        std::vector<uint8_t> data(s.begin(), s.end());
        auto c = CompressHybrid(data);
        auto d = DecompressHybrid(c);
        test("Digits only", data == d);
    }

    // Whitespace heavy
    {
        std::string s;
        for (int i = 0; i < 500; i++) s += "a   b\t\tc\n\n";
        std::vector<uint8_t> data(s.begin(), s.end());
        auto c = CompressHybrid(data);
        auto d = DecompressHybrid(c);
        test("Whitespace heavy", data == d);
    }

    // JSON-like
    {
        std::string s = R"({"key":"value","num":123,"arr":[1,2,3]})";
        for (int i = 0; i < 50; i++) s += s.substr(0, 40);
        std::vector<uint8_t> data(s.begin(), s.end());
        auto c = CompressHybrid(data);
        auto d = DecompressHybrid(c);
        test("JSON-like structure", data == d);
    }

    // XML-like
    {
        std::string s = "<root><item id=\"1\">value</item></root>";
        for (int i = 0; i < 50; i++) s += s.substr(0, 38);
        std::vector<uint8_t> data(s.begin(), s.end());
        auto c = CompressHybrid(data);
        auto d = DecompressHybrid(c);
        test("XML-like structure", data == d);
    }
}

void test_binary_patterns() {
    std::cout << "\n=== Binary Pattern Tests ===\n";

    // Mostly zeros with occasional data
    {
        std::vector<uint8_t> sparse(10000, 0);
        for (int i = 0; i < 100; i++) sparse[i * 100] = i;
        auto c = CompressHybrid(sparse);
        auto d = DecompressHybrid(c);
        test("Sparse data", sparse == d);
    }

    // Header-like (magic + data)
    {
        std::vector<uint8_t> header = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
        for (int i = 0; i < 1000; i++) header.push_back(i % 256);
        auto c = CompressHybrid(header);
        auto d = DecompressHybrid(c);
        test("Header + data", header == d);
    }

    // ELF-like header
    {
        std::vector<uint8_t> elf = {0x7F, 'E', 'L', 'F', 2, 1, 1, 0};
        for (int i = 0; i < 56; i++) elf.push_back(0);
        for (int i = 0; i < 1000; i++) elf.push_back((i * 7) % 256);
        auto c = CompressHybrid(elf);
        auto d = DecompressHybrid(c);
        test("ELF-like binary", elf == d);
    }
}

int main() {
    std::cout << "=== Edge Case Tests ===\n";

    test_empty_data();
    test_single_byte();
    test_two_bytes();
    test_all_same_byte();
    test_all_unique_bytes();
    test_boundary_sizes();
    test_special_patterns();
    test_text_variations();
    test_binary_patterns();

    std::cout << "\n=== Results: " << passed << " passed, " << failed << " failed ===\n";
    return failed > 0 ? 1 : 0;
}
