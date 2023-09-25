// cmdline:-std=c++20

const auto *u8s = u8"text";   // u8s previously deduced as const char *; now deduced as const char8_t *.

auto u8c = u8'c';             // u8c previously deduced as char; now deduced as char8_t.

