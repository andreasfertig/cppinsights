// cmdline:-std=c++20

#define INSIGHTS_USE_TEMPLATE 1

struct X {
  int Fun() const { return 2; }

  int alfa;
  char beta;
};

template<X I>
int Apple() {
  return I.Fun();
}

void test()
{
  Apple<X{45}>();
  Apple<X{45, 't'}>();
}
