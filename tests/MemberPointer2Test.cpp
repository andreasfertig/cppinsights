template<typename T>
struct myStringView {
};

using string_view = myStringView<char>;

template<typename T>
struct printer final {
  template<typename R>
  void operator()(R T::* ptr)
  {}
};

struct my_type final {
  string_view b;
  int i;
};

int main()
{
  constexpr my_type t{};
  printer<my_type>  p{};
  p(&my_type::b);
  p(&my_type::i);
}

