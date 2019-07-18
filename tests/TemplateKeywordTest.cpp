namespace detail
{
  template<typename T>
  struct to{
    template<typename... Ts>
    struct result { };
  };
}

template <typename ...>
struct compose;

template <>
struct compose<>
{
  using continuation = int;

  template <typename C = continuation>
  struct to
  {
    using T = detail::to<C>;
    template <typename ... Ts>
    // missing template in output for the template specialization. Looks ok.
    using result = typename T::template result<Ts...>;
  };
};

int main()
{
    // XXX: ns missing here for = ...::a;
    compose<>::to<>::result<int, char> a;
}
