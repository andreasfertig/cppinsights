template<class... Ts>
struct overload : Ts... {
  using Ts::operator()...;
};

// #B CTAD
template<class... Ts>
overload(Ts...) -> overload<Ts...>;

int main() {
    overload ol{
      [](int&) {  },
      [](char&) {  },
    };
}

