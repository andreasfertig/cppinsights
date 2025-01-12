// cmdline:-std=c++26

auto g() {
  int ar[]{2,3,4};
  auto [a, b [[maybe_unused]], c] = ar;
  return a + c;
}
