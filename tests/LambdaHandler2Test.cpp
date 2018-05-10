int main()
{
#if 0
  [](){}();

  [](){ return 1;}();
#endif
  int (*fp)(int, char) = [](int a, char b){ return a+b;};

//  int (*fd)(int) = [](auto a){ return a;};

}

