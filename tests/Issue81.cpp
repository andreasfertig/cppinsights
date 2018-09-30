auto f(int i)
{ // also `decltype(auto) f(int i)` and `int(*f(int i))[3]`
  static int arr[5][3];
  return &arr[i];
}

auto *ff(int i)
{
  static int arr[5][3];
  return &arr[i];
}

