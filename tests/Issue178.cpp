struct X{
  int f();
  int f(int);
  int f(double);
};

struct Y:X {
  int f(float);
  using X::f;
};
