// cmdline:-std=c++20

template<typename Type, typename Archive>
constexpr static auto simple()
{
  return requires(Type && item, Archive && archive)
  {
    serialize(archive, item);
  };
}

template<typename Type, typename Archive>
constexpr static auto nested()
{
  return requires(Type && item, Archive && archive)
  {
    requires serialize(archive, item);
  };
}

template<typename Type>
constexpr static auto type()
{
  return requires(Type && item)
  {
    typename Type::value;
  };
}


int main()
{
  return simple<int, int>() || nested<int, int>() || type<int>();
}

