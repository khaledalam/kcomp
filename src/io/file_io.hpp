#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

using ProgressCallback = std::function<void(size_t current, size_t total)>;

std::vector<uint8_t> ReadAll(const std::string &path);
std::vector<uint8_t> ReadAllWithProgress(const std::string &path, ProgressCallback cb);
void WriteAll(const std::string &path, const std::vector<uint8_t> &data);
void WriteAllWithProgress(const std::string &path, const std::vector<uint8_t> &data, ProgressCallback cb);
size_t GetFileSize(const std::string &path);
