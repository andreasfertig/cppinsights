template<typename... Args> struct count;

template<>
struct count<> {
  static const int value = 0;
};

template<typename T, typename... Args>
struct count<T, Args...>
{
  static const int value = 1 + count<Args...>::value;
};

static_assert(count<double>::value == 1, 
              "2 elements");

