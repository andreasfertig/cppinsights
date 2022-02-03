// cmdline:-std=c++20

template<typename T>
concept C = true;

auto lambda = [](C auto container) { };

int main() {
  lambda(4);
}
