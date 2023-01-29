#pragma once

#include <string>
#include <string_view>
#include <vector>

[[nodiscard]] std::vector<std::string_view> StringViewSplitter(std::string_view input, char c);