#include "file_io.hpp"
#include <cstdio>
#include <stdexcept>

constexpr size_t CHUNK_SIZE = 64 * 1024; // 64KB chunks for progress

size_t GetFileSize(const std::string &path) {
  std::FILE *f = std::fopen(path.c_str(), "rb");
  if (!f) return 0;
  std::fseek(f, 0, SEEK_END);
  long n = std::ftell(f);
  std::fclose(f);
  return n > 0 ? static_cast<size_t>(n) : 0;
}

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

std::vector<uint8_t> ReadAllWithProgress(const std::string &path, ProgressCallback cb) {
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

  size_t total = static_cast<size_t>(n);
  std::vector<uint8_t> buf(total);

  if (!buf.empty()) {
    size_t read_total = 0;
    while (read_total < total) {
      size_t to_read = std::min(CHUNK_SIZE, total - read_total);
      size_t read_now = std::fread(buf.data() + read_total, 1, to_read, f);
      if (read_now == 0) {
        std::fclose(f);
        throw std::runtime_error("read failed");
      }
      read_total += read_now;
      if (cb) cb(read_total, total);
    }
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

void WriteAllWithProgress(const std::string &path, const std::vector<uint8_t> &data, ProgressCallback cb) {
  std::FILE *f = std::fopen(path.c_str(), "wb");
  if (!f)
    throw std::runtime_error("open failed: " + path);

  if (!data.empty()) {
    size_t total = data.size();
    size_t written = 0;
    while (written < total) {
      size_t to_write = std::min(CHUNK_SIZE, total - written);
      size_t wrote = std::fwrite(data.data() + written, 1, to_write, f);
      if (wrote == 0) {
        std::fclose(f);
        throw std::runtime_error("write failed");
      }
      written += wrote;
      if (cb) cb(written, total);
    }
  }
  std::fclose(f);
}
