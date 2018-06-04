int main()
{
#if 0
  [](){}();

  [](){ return 1;}();
#endif
  int (*fp)(int, char) = [](int a, char b){ return a+b;};

//   int (*fp)(int, char) = static_cast<int (*)(int, char)>(__lambda_8_26{}.operator __lambda_8_26::retType());

//  int (*fd)(int) = [](auto a){ return a;};

}

