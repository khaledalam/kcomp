#pragma once

#include <cstdint>
#include <string>
#include <vector>

std::vector<uint8_t> ReadAll(const std::string &path);
void WriteAll(const std::string &path, const std::vector<uint8_t> &data);
