#include <iostream>
#include <vector>
#include <cstdint>
#include <cassert>
#include <string>
#include "../src/models/bwt.hpp"

bool TestBWTRoundtrip(const std::vector<uint8_t>& input, const std::string& name) {
    uint32_t primary_index = 0;
    auto encoded = BWTEncode(input, primary_index);
    auto decoded = BWTDecode(encoded, primary_index);

    bool ok = (input == decoded);
    std::cout << name << " (size=" << input.size() << "): " << (ok ? "PASS" : "FAIL") << "\n";

    if (!ok && input.size() <= 50) {
        std::cout << "  Original: ";
        for (auto c : input) std::cout << (char)c;
        std::cout << "\n  Decoded:  ";
        for (auto c : decoded) std::cout << (char)c;
        std::cout << "\n";
    }

    return ok;
}

int main() {
    int passed = 0, failed = 0;

    std::cout << "=== BWT Bug Regression Tests ===\n\n";

    // Test 1: Simple case - "banana"
    {
        std::string s = "banana";
        std::vector<uint8_t> input(s.begin(), s.end());
        if (TestBWTRoundtrip(input, "banana")) passed++; else failed++;
    }

    // Test 2: Size 100 with repeating pattern (worked before)
    {
        std::vector<uint8_t> input;
        const char* pattern = "The quick brown fox jumps over the lazy dog. ";
        size_t plen = 46;
        for (int i = 0; i < 100; i++) {
            input.push_back(pattern[i % plen]);
        }
        if (TestBWTRoundtrip(input, "pattern_100")) passed++; else failed++;
    }

    // Test 3: Size 1000 with repeating pattern (FAILED before - cycle in T array)
    // This is the main regression test for the BWT circular comparison bug
    {
        std::vector<uint8_t> input;
        const char* pattern = "The quick brown fox jumps over the lazy dog. ";
        size_t plen = 46;
        for (int i = 0; i < 1000; i++) {
            input.push_back(pattern[i % plen]);
        }
        if (TestBWTRoundtrip(input, "pattern_1000")) passed++; else failed++;
    }

    // Test 4: Size 10000 with repeating pattern (worked before)
    {
        std::vector<uint8_t> input;
        const char* pattern = "The quick brown fox jumps over the lazy dog. ";
        size_t plen = 46;
        for (int i = 0; i < 10000; i++) {
            input.push_back(pattern[i % plen]);
        }
        if (TestBWTRoundtrip(input, "pattern_10000")) passed++; else failed++;
    }

    // Test 5: All same character (edge case)
    {
        std::vector<uint8_t> input(100, 'a');
        if (TestBWTRoundtrip(input, "all_same_100")) passed++; else failed++;
    }

    // Test 6: Short periodic pattern "abcabc"
    {
        std::string s = "abcabcabcabc";
        std::vector<uint8_t> input(s.begin(), s.end());
        if (TestBWTRoundtrip(input, "periodic_abc")) passed++; else failed++;
    }

    // Test 7: Two characters alternating
    {
        std::vector<uint8_t> input;
        for (int i = 0; i < 100; i++) {
            input.push_back(i % 2 ? 'a' : 'b');
        }
        if (TestBWTRoundtrip(input, "alternating_100")) passed++; else failed++;
    }

    // Test 8: Pattern that causes partial repetition at boundary
    {
        std::vector<uint8_t> input;
        const char* pattern = "hello";
        for (int i = 0; i < 503; i++) {  // 503 = 100*5 + 3, partial pattern
            input.push_back(pattern[i % 5]);
        }
        if (TestBWTRoundtrip(input, "partial_boundary")) passed++; else failed++;
    }

    std::cout << "\n=== Results: " << passed << " passed, " << failed << " failed ===\n";

    return failed > 0 ? 1 : 0;
}
