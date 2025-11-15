#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>

std::vector<std::uint8_t> sha1(const std::string &data);
std::vector<std::uint8_t> sha1_bytes(const std::vector<std::uint8_t> &data);