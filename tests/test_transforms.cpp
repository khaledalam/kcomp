#include <iostream>
#include <vector>
#include <cstdint>
#include <string>
#include "../src/models/ppm.hpp"
#include "../src/models/bwt.hpp"
#include "../src/models/lz77.hpp"
#include "../src/models/dict.hpp"

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

void test_rle() {
    std::cout << "\n=== RLE Transform Tests ===\n";

    // Empty
    {
        std::vector<uint8_t> empty;
        test("RLE empty", RLEDecompress(RLECompress(empty)).empty());
    }

    // Single byte
    {
        std::vector<uint8_t> single = {'X'};
        auto c = RLECompress(single);
        auto d = RLEDecompress(c);
        test("RLE single byte", single == d);
    }

    // Long run of same byte
    {
        std::vector<uint8_t> run(1000, 'A');
        auto c = RLECompress(run);
        auto d = RLEDecompress(c);
        test("RLE long run", run == d);
        test("RLE long run compressed", c.size() < run.size());
    }

    // Multiple runs
    {
        std::vector<uint8_t> runs;
        for (int i = 0; i < 10; i++) {
            for (int j = 0; j < 50; j++) runs.push_back('A' + i);
        }
        auto c = RLECompress(runs);
        auto d = RLEDecompress(c);
        test("RLE multiple runs", runs == d);
    }

    // No runs (alternating)
    {
        std::vector<uint8_t> alt;
        for (int i = 0; i < 100; i++) alt.push_back(i % 2 ? 'A' : 'B');
        auto c = RLECompress(alt);
        auto d = RLEDecompress(c);
        test("RLE alternating", alt == d);
    }

    // Mixed
    {
        std::vector<uint8_t> mixed;
        mixed.insert(mixed.end(), 100, 'X');
        mixed.push_back('Y');
        mixed.insert(mixed.end(), 50, 'Z');
        mixed.push_back('A');
        mixed.push_back('B');
        mixed.insert(mixed.end(), 200, 0);
        auto c = RLECompress(mixed);
        auto d = RLEDecompress(c);
        test("RLE mixed", mixed == d);
    }
}

void test_delta() {
    std::cout << "\n=== Delta Transform Tests ===\n";

    // Empty
    {
        std::vector<uint8_t> empty;
        test("Delta empty", DeltaDecode(DeltaEncode(empty)).empty());
    }

    // Single byte
    {
        std::vector<uint8_t> single = {100};
        auto e = DeltaEncode(single);
        auto d = DeltaDecode(e);
        test("Delta single byte", single == d);
    }

    // Sequential increasing
    {
        std::vector<uint8_t> seq;
        for (int i = 0; i < 256; i++) seq.push_back(i);
        auto e = DeltaEncode(seq);
        auto d = DeltaDecode(e);
        test("Delta sequential", seq == d);
    }

    // Sequential decreasing
    {
        std::vector<uint8_t> seq;
        for (int i = 255; i >= 0; i--) seq.push_back(i);
        auto e = DeltaEncode(seq);
        auto d = DeltaDecode(e);
        test("Delta decreasing", seq == d);
    }

    // Constant (delta = 0)
    {
        std::vector<uint8_t> const_data(1000, 42);
        auto e = DeltaEncode(const_data);
        auto d = DeltaDecode(e);
        test("Delta constant", const_data == d);
    }

    // Gradual increase (good for delta)
    {
        std::vector<uint8_t> grad;
        for (int i = 0; i < 1000; i++) grad.push_back((i / 4) % 256);
        auto e = DeltaEncode(grad);
        auto d = DeltaDecode(e);
        test("Delta gradual", grad == d);
    }

    // Random (bad for delta but should still work)
    {
        std::vector<uint8_t> rnd;
        uint32_t seed = 12345;
        for (int i = 0; i < 1000; i++) {
            seed = seed * 1103515245 + 12345;
            rnd.push_back((seed >> 16) & 0xFF);
        }
        auto e = DeltaEncode(rnd);
        auto d = DeltaDecode(e);
        test("Delta random", rnd == d);
    }
}

void test_word_encoding() {
    std::cout << "\n=== Word Encoding Tests ===\n";

    // Empty
    {
        std::vector<uint8_t> empty;
        test("Word empty", WordDecode(WordEncode(empty)).empty());
    }

    // Single word
    {
        std::string s = "hello";
        std::vector<uint8_t> data(s.begin(), s.end());
        auto e = WordEncode(data);
        auto d = WordDecode(e);
        test("Word single", data == d);
    }

    // Multiple words
    {
        std::string s = "the quick brown fox";
        std::vector<uint8_t> data(s.begin(), s.end());
        auto e = WordEncode(data);
        auto d = WordDecode(e);
        test("Word multiple", data == d);
    }

    // Repeated words (should benefit from tokenization)
    {
        std::string s;
        for (int i = 0; i < 100; i++) s += "the quick brown fox jumps ";
        std::vector<uint8_t> data(s.begin(), s.end());
        auto e = WordEncode(data);
        auto d = WordDecode(e);
        test("Word repeated", data == d);
    }

    // HTML-like content
    {
        std::string s = "<html><head><title>Test</title></head><body>"
                       "<p>Hello World</p><p>Hello World</p></body></html>";
        std::vector<uint8_t> data(s.begin(), s.end());
        auto e = WordEncode(data);
        auto d = WordDecode(e);
        test("Word HTML", data == d);
    }

    // Mixed text and numbers
    {
        std::string s = "value1=100 value2=200 value3=300";
        for (int i = 0; i < 50; i++) s += " value1=100 value2=200";
        std::vector<uint8_t> data(s.begin(), s.end());
        auto e = WordEncode(data);
        auto d = WordDecode(e);
        test("Word mixed", data == d);
    }
}

void test_sparse_encoding() {
    std::cout << "\n=== Sparse Encoding Tests ===\n";

    // Empty
    {
        std::vector<uint8_t> empty;
        test("Sparse empty", SparseDecode(SparseEncode(empty)).empty());
    }

    // All zeros
    {
        std::vector<uint8_t> zeros(1000, 0);
        auto e = SparseEncode(zeros);
        auto d = SparseDecode(e);
        test("Sparse all zeros", zeros == d);
        test("Sparse all zeros compressed", e.size() < zeros.size());
    }

    // Mostly zeros
    {
        std::vector<uint8_t> sparse(10000, 0);
        sparse[0] = 1;
        sparse[1000] = 2;
        sparse[5000] = 3;
        sparse[9999] = 4;
        auto e = SparseEncode(sparse);
        auto d = SparseDecode(e);
        test("Sparse mostly zeros", sparse == d);
    }

    // No zeros
    {
        std::vector<uint8_t> dense;
        for (int i = 0; i < 1000; i++) dense.push_back((i % 255) + 1);
        auto e = SparseEncode(dense);
        auto d = SparseDecode(e);
        test("Sparse no zeros", dense == d);
    }

    // Alternating zeros
    {
        std::vector<uint8_t> alt;
        for (int i = 0; i < 1000; i++) alt.push_back(i % 2 ? 0 : i % 256);
        auto e = SparseEncode(alt);
        auto d = SparseDecode(e);
        test("Sparse alternating", alt == d);
    }
}

void test_record_interleave() {
    std::cout << "\n=== Record Interleave Tests ===\n";

    // Small data (smaller than record size)
    {
        std::vector<uint8_t> small = {'A', 'B', 'C', 'D'};
        auto e = RecordInterleave(small, 512);
        auto d = RecordDeinterleave(e);
        test("Record small", small == d);
    }

    // Exact multiple of record size
    {
        std::vector<uint8_t> exact(1024, 0);
        for (size_t i = 0; i < exact.size(); i++) exact[i] = i % 256;
        auto e = RecordInterleave(exact, 512);
        auto d = RecordDeinterleave(e);
        test("Record exact", exact == d);
    }

    // Non-multiple of record size
    {
        std::vector<uint8_t> partial(1500, 0);
        for (size_t i = 0; i < partial.size(); i++) partial[i] = i % 256;
        auto e = RecordInterleave(partial, 512);
        auto d = RecordDeinterleave(e);
        test("Record partial", partial == d);
    }

    // Different record sizes
    {
        std::vector<uint8_t> data(2048, 0);
        for (size_t i = 0; i < data.size(); i++) data[i] = i % 256;

        std::vector<int> rec_sizes = {64, 128, 256, 512, 1024};
        for (int rs : rec_sizes) {
            auto e = RecordInterleave(data, rs);
            auto d = RecordDeinterleave(e);
            test("Record size=" + std::to_string(rs), data == d);
        }
    }
}

void test_dict_encoding() {
    std::cout << "\n=== Dictionary Encoding Tests ===\n";

    // Empty
    {
        std::vector<uint8_t> empty;
        auto e = DictEncode(empty);
        auto d = DictDecode(e);
        test("Dict empty", empty == d);
    }

    // Small text
    {
        std::string s = "Hello, World!";
        std::vector<uint8_t> data(s.begin(), s.end());
        auto e = DictEncode(data);
        auto d = DictDecode(e);
        test("Dict small text", data == d);
    }

    // Common English words (should match dictionary)
    {
        std::string s = "the and for are but not you all can had";
        for (int i = 0; i < 50; i++) s += " the and for are";
        std::vector<uint8_t> data(s.begin(), s.end());
        auto e = DictEncode(data);
        auto d = DictDecode(e);
        test("Dict common words", data == d);
    }

    // Binary data (shouldn't break)
    {
        std::vector<uint8_t> bin;
        for (int i = 0; i < 1000; i++) bin.push_back(i % 256);
        auto e = DictEncode(bin);
        auto d = DictDecode(e);
        test("Dict binary", bin == d);
    }
}

void test_combined_transforms() {
    std::cout << "\n=== Combined Transform Tests ===\n";

    std::vector<uint8_t> data;
    for (int i = 0; i < 5000; i++) {
        data.push_back("The quick brown fox jumps over the lazy dog. "[i % 46]);
    }

    // RLE + Delta
    {
        auto rle = RLECompress(data);
        auto delta = DeltaEncode(rle);
        auto d_delta = DeltaDecode(delta);
        auto d_rle = RLEDecompress(d_delta);
        test("RLE + Delta", data == d_rle);
    }

    // Delta + RLE
    {
        auto delta = DeltaEncode(data);
        auto rle = RLECompress(delta);
        auto d_rle = RLEDecompress(rle);
        auto d_delta = DeltaDecode(d_rle);
        test("Delta + RLE", data == d_delta);
    }

    // BWT + MTF
    {
        uint32_t idx;
        auto bwt = BWTEncode(data, idx);
        auto mtf = MTFEncode(bwt);
        auto d_mtf = MTFDecode(mtf);
        auto d_bwt = BWTDecode(d_mtf, idx);
        test("BWT + MTF", data == d_bwt);
    }

    // BWT + MTF + RLE
    {
        uint32_t idx;
        auto bwt = BWTEncode(data, idx);
        auto mtf = MTFEncode(bwt);
        auto rle = RLECompress(mtf);
        auto d_rle = RLEDecompress(rle);
        auto d_mtf = MTFDecode(d_rle);
        auto d_bwt = BWTDecode(d_mtf, idx);
        test("BWT + MTF + RLE", data == d_bwt);
    }

    // Word + RLE
    {
        auto word = WordEncode(data);
        auto rle = RLECompress(word);
        auto d_rle = RLEDecompress(rle);
        auto d_word = WordDecode(d_rle);
        test("Word + RLE", data == d_word);
    }
}

int main() {
    std::cout << "=== Transform Tests ===\n";

    test_rle();
    test_delta();
    test_word_encoding();
    test_sparse_encoding();
    test_record_interleave();
    test_dict_encoding();
    test_combined_transforms();

    std::cout << "\n=== Results: " << passed << " passed, " << failed << " failed ===\n";
    return failed > 0 ? 1 : 0;
}
