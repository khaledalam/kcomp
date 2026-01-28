#include <iostream>
#include <vector>
#include <cstdint>
#include <string>
#include <fstream>
#include "../src/models/ppm.hpp"

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

std::vector<uint8_t> read_file(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return {};
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(f)),
                                 std::istreambuf_iterator<char>());
}

void test_json_formats() {
    std::cout << "\n=== JSON Format Tests ===\n";

    // Empty JSON
    {
        std::string s = "{}";
        std::vector<uint8_t> data(s.begin(), s.end());
        auto c = CompressHybrid(data);
        auto d = DecompressHybrid(c);
        test("Empty JSON object", data == d);
    }

    // Simple JSON
    {
        std::string s = R"({"key":"value"})";
        std::vector<uint8_t> data(s.begin(), s.end());
        auto c = CompressHybrid(data);
        auto d = DecompressHybrid(c);
        test("Simple JSON", data == d);
    }

    // Nested JSON
    {
        std::string s = R"({"outer":{"inner":{"deep":"value"}}})";
        std::vector<uint8_t> data(s.begin(), s.end());
        auto c = CompressHybrid(data);
        auto d = DecompressHybrid(c);
        test("Nested JSON", data == d);
    }

    // JSON array
    {
        std::string s = R"([1,2,3,4,5,6,7,8,9,10])";
        std::vector<uint8_t> data(s.begin(), s.end());
        auto c = CompressHybrid(data);
        auto d = DecompressHybrid(c);
        test("JSON array", data == d);
    }

    // JSON with special characters
    {
        std::string s = R"({"msg":"Hello\nWorld\t\"quoted\""})";
        std::vector<uint8_t> data(s.begin(), s.end());
        auto c = CompressHybrid(data);
        auto d = DecompressHybrid(c);
        test("JSON special chars", data == d);
    }

    // Large JSON
    {
        std::string s = "{\"items\":[";
        for (int i = 0; i < 100; i++) {
            if (i > 0) s += ",";
            s += "{\"id\":" + std::to_string(i) + ",\"name\":\"item" + std::to_string(i) + "\"}";
        }
        s += "]}";
        std::vector<uint8_t> data(s.begin(), s.end());
        auto c = CompressHybrid(data);
        auto d = DecompressHybrid(c);
        test("Large JSON array", data == d);
    }
}

void test_xml_formats() {
    std::cout << "\n=== XML Format Tests ===\n";

    // Empty XML
    {
        std::string s = "<root/>";
        std::vector<uint8_t> data(s.begin(), s.end());
        auto c = CompressHybrid(data);
        auto d = DecompressHybrid(c);
        test("Empty XML", data == d);
    }

    // Simple XML
    {
        std::string s = "<root><item>value</item></root>";
        std::vector<uint8_t> data(s.begin(), s.end());
        auto c = CompressHybrid(data);
        auto d = DecompressHybrid(c);
        test("Simple XML", data == d);
    }

    // XML with attributes
    {
        std::string s = R"(<root attr="value"><item id="1" type="test">content</item></root>)";
        std::vector<uint8_t> data(s.begin(), s.end());
        auto c = CompressHybrid(data);
        auto d = DecompressHybrid(c);
        test("XML with attributes", data == d);
    }

    // XML with CDATA
    {
        std::string s = "<root><![CDATA[Some <special> content & stuff]]></root>";
        std::vector<uint8_t> data(s.begin(), s.end());
        auto c = CompressHybrid(data);
        auto d = DecompressHybrid(c);
        test("XML with CDATA", data == d);
    }

    // Large nested XML
    {
        std::string s = "<?xml version=\"1.0\"?><root>";
        for (int i = 0; i < 50; i++) {
            s += "<item id=\"" + std::to_string(i) + "\"><name>Item " + std::to_string(i) + "</name><value>" + std::to_string(i * 10) + "</value></item>";
        }
        s += "</root>";
        std::vector<uint8_t> data(s.begin(), s.end());
        auto c = CompressHybrid(data);
        auto d = DecompressHybrid(c);
        test("Large nested XML", data == d);
    }
}

void test_csv_formats() {
    std::cout << "\n=== CSV Format Tests ===\n";

    // Simple CSV
    {
        std::string s = "a,b,c\n1,2,3\n4,5,6\n";
        std::vector<uint8_t> data(s.begin(), s.end());
        auto c = CompressHybrid(data);
        auto d = DecompressHybrid(c);
        test("Simple CSV", data == d);
    }

    // CSV with quotes
    {
        std::string s = "name,value\n\"John, Jr.\",100\n\"Jane\",200\n";
        std::vector<uint8_t> data(s.begin(), s.end());
        auto c = CompressHybrid(data);
        auto d = DecompressHybrid(c);
        test("CSV with quotes", data == d);
    }

    // Large CSV
    {
        std::string s = "id,name,value,description\n";
        for (int i = 0; i < 100; i++) {
            s += std::to_string(i) + ",item" + std::to_string(i) + "," + std::to_string(i * 10) + ",\"Description for item " + std::to_string(i) + "\"\n";
        }
        std::vector<uint8_t> data(s.begin(), s.end());
        auto c = CompressHybrid(data);
        auto d = DecompressHybrid(c);
        test("Large CSV", data == d);
    }
}

void test_html_formats() {
    std::cout << "\n=== HTML Format Tests ===\n";

    // Simple HTML
    {
        std::string s = "<!DOCTYPE html><html><head><title>Test</title></head><body><p>Hello</p></body></html>";
        std::vector<uint8_t> data(s.begin(), s.end());
        auto c = CompressHybrid(data);
        auto d = DecompressHybrid(c);
        test("Simple HTML", data == d);
    }

    // HTML with attributes and classes
    {
        std::string s = R"(<!DOCTYPE html><html lang="en"><head><meta charset="UTF-8"><title>Test</title></head><body class="main"><div id="content" class="container"><p style="color:red">Hello World</p></div></body></html>)";
        std::vector<uint8_t> data(s.begin(), s.end());
        auto c = CompressHybrid(data);
        auto d = DecompressHybrid(c);
        test("HTML with attributes", data == d);
    }

    // HTML with repeated structures
    {
        std::string s = "<!DOCTYPE html><html><body><ul>";
        for (int i = 0; i < 50; i++) {
            s += "<li class=\"item\">Item " + std::to_string(i) + "</li>";
        }
        s += "</ul></body></html>";
        std::vector<uint8_t> data(s.begin(), s.end());
        auto c = CompressHybrid(data);
        auto d = DecompressHybrid(c);
        test("HTML with repeated structures", data == d);
    }
}

void test_log_formats() {
    std::cout << "\n=== Log Format Tests ===\n";

    // Apache-style log
    {
        std::string s;
        for (int i = 0; i < 50; i++) {
            s += "192.168.1." + std::to_string(i % 256) + " - - [01/Jan/2024:12:" + std::to_string(i % 60) + ":00 +0000] \"GET /page" + std::to_string(i) + " HTTP/1.1\" 200 1234\n";
        }
        std::vector<uint8_t> data(s.begin(), s.end());
        auto c = CompressHybrid(data);
        auto d = DecompressHybrid(c);
        test("Apache log format", data == d);
    }

    // JSON log format
    {
        std::string s;
        for (int i = 0; i < 50; i++) {
            s += "{\"timestamp\":\"2024-01-01T12:" + std::to_string(i % 60) + ":00Z\",\"level\":\"INFO\",\"message\":\"Request " + std::to_string(i) + " processed\"}\n";
        }
        std::vector<uint8_t> data(s.begin(), s.end());
        auto c = CompressHybrid(data);
        auto d = DecompressHybrid(c);
        test("JSON log format", data == d);
    }

    // Syslog format
    {
        std::string s;
        for (int i = 0; i < 50; i++) {
            s += "Jan  1 12:" + std::to_string(i % 60) + ":00 hostname app[" + std::to_string(1000 + i) + "]: Message " + std::to_string(i) + "\n";
        }
        std::vector<uint8_t> data(s.begin(), s.end());
        auto c = CompressHybrid(data);
        auto d = DecompressHybrid(c);
        test("Syslog format", data == d);
    }
}

void test_code_formats() {
    std::cout << "\n=== Code Format Tests ===\n";

    // C code
    {
        std::string s = R"(
#include <stdio.h>

int main() {
    printf("Hello, World!\n");
    for (int i = 0; i < 10; i++) {
        printf("i = %d\n", i);
    }
    return 0;
}
)";
        for (int i = 0; i < 20; i++) s += s.substr(0, 100);
        std::vector<uint8_t> data(s.begin(), s.end());
        auto c = CompressHybrid(data);
        auto d = DecompressHybrid(c);
        test("C code", data == d);
    }

    // Python code
    {
        std::string s = R"(
def hello():
    print("Hello, World!")

for i in range(10):
    hello()
    print(f"Iteration {i}")
)";
        for (int i = 0; i < 30; i++) s += s.substr(0, 80);
        std::vector<uint8_t> data(s.begin(), s.end());
        auto c = CompressHybrid(data);
        auto d = DecompressHybrid(c);
        test("Python code", data == d);
    }

    // JavaScript code
    {
        std::string s = R"(
function greet(name) {
    console.log(`Hello, ${name}!`);
}

const items = [1, 2, 3, 4, 5];
items.forEach(item => {
    greet(`User ${item}`);
});
)";
        for (int i = 0; i < 25; i++) s += s.substr(0, 90);
        std::vector<uint8_t> data(s.begin(), s.end());
        auto c = CompressHybrid(data);
        auto d = DecompressHybrid(c);
        test("JavaScript code", data == d);
    }
}

void test_binary_formats() {
    std::cout << "\n=== Binary Format Tests ===\n";

    // PNG-like header
    {
        std::vector<uint8_t> data = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
        for (int i = 0; i < 1000; i++) data.push_back(i % 256);
        auto c = CompressHybrid(data);
        auto d = DecompressHybrid(c);
        test("PNG-like header", data == d);
    }

    // JPEG-like header
    {
        std::vector<uint8_t> data = {0xFF, 0xD8, 0xFF, 0xE0, 0x00, 0x10, 'J', 'F', 'I', 'F', 0x00};
        for (int i = 0; i < 1000; i++) data.push_back(i % 256);
        auto c = CompressHybrid(data);
        auto d = DecompressHybrid(c);
        test("JPEG-like header", data == d);
    }

    // ZIP-like header
    {
        std::vector<uint8_t> data = {'P', 'K', 0x03, 0x04, 0x14, 0x00, 0x00, 0x00};
        for (int i = 0; i < 1000; i++) data.push_back(i % 256);
        auto c = CompressHybrid(data);
        auto d = DecompressHybrid(c);
        test("ZIP-like header", data == d);
    }

    // ELF-like header
    {
        std::vector<uint8_t> data = {0x7F, 'E', 'L', 'F', 2, 1, 1, 0};
        for (int i = 0; i < 56; i++) data.push_back(0);  // ELF header padding
        for (int i = 0; i < 1000; i++) data.push_back((i * 7) % 256);
        auto c = CompressHybrid(data);
        auto d = DecompressHybrid(c);
        test("ELF-like header", data == d);
    }
}

void test_real_files() {
    std::cout << "\n=== Real File Tests ===\n";

    std::vector<std::pair<std::string, std::string>> files = {
        {"testdata/json_data.json", "JSON file"},
        {"testdata/xml_data.xml", "XML file"},
        {"testdata/csv_data.csv", "CSV file"},
        {"testdata/log_file.log", "Log file"},
        {"testdata/source_code.txt", "Source code"},
        {"testdata/webpage.html", "HTML file"},
        {"testdata/image.bmp", "BMP image"},
        {"testdata/document.pdf", "PDF document"},
        {"testdata/test_audio.wav", "WAV audio"},
        {"testdata/test_image.png", "PNG image"},
        {"testdata/archive.tar", "TAR archive"},
        {"testdata/binary.elf", "ELF binary"}
    };

    for (const auto& [path, name] : files) {
        auto data = read_file(path);
        if (data.empty()) {
            std::cout << "  [SKIP] " << name << " (not found)\n";
            continue;
        }

        auto c = CompressHybrid(data);
        auto d = DecompressHybrid(c);
        test(name + " (" + std::to_string(data.size()) + " bytes)", data == d);
    }
}

int main() {
    std::cout << "=== File Format Tests ===\n";

    test_json_formats();
    test_xml_formats();
    test_csv_formats();
    test_html_formats();
    test_log_formats();
    test_code_formats();
    test_binary_formats();
    test_real_files();

    std::cout << "\n=== Results: " << passed << " passed, " << failed << " failed ===\n";
    return failed > 0 ? 1 : 0;
}
