template<int N, typename T>
constexpr T min(const T& b)
{
    return N < b;
} 

int main()
{
  auto m = min<2>(3);
}

