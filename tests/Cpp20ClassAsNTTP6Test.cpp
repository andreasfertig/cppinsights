// cmdline:-std=c++20

#define INSIGHTS_USE_TEMPLATE 1

struct X {
  int Fun() const { return 2; }

  int alfa;
  double beta;
};

template<X I = X{2}>
struct Test
{
    int x;
};

void test()
{
  Test<X{45, 3.14}> t1{};
  Test<> t2{};
}
