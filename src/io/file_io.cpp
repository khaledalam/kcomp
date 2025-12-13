#include "file_io.hpp"
#include <cstdio>
#include <stdexcept>

std::vector<uint8_t> ReadAll(const std::string &path) {
  std::FILE *f = std::fopen(path.c_str(), "rb");
  if (!f)
    throw std::runtime_error("open failed: " + path);
  std::fseek(f, 0, SEEK_END);
  long n = std::ftell(f);
  if (n < 0) {
    std::fclose(f);
    throw std::runtime_error("ftell failed");
  }
  std::fseek(f, 0, SEEK_SET);
  std::vector<uint8_t> buf((size_t)n);
  if (!buf.empty() && std::fread(buf.data(), 1, buf.size(), f) != buf.size()) {
    std::fclose(f);
    throw std::runtime_error("read failed");
  }
  std::fclose(f);
  return buf;
}

void WriteAll(const std::string &path, const std::vector<uint8_t> &data) {
  std::FILE *f = std::fopen(path.c_str(), "wb");
  if (!f)
    throw std::runtime_error("open failed: " + path);
  if (!data.empty() &&
      std::fwrite(data.data(), 1, data.size(), f) != data.size()) {
    std::fclose(f);
    throw std::runtime_error("write failed");
  }
  std::fclose(f);
}
