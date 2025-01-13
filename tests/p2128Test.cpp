// cmdline:-std=c++2b

class mdspan {
  int field{};
public:    
  template<class... IndexType>
  constexpr auto& operator[](IndexType...) { return field; }
};

int main() {
  mdspan s{};
  s[1, 1, 1] = 42;
}
