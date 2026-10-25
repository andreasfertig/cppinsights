// cmdline:-std=c++26

int main()
{
  int res{};
  template for(auto x : {1, 2, 3})
  {
    res += x;
  }
}
