// cmdline:-std=c++20

#include <source_location>

auto s = std::source_location::current();

auto X = __builtin_LINE();
