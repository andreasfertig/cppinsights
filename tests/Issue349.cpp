struct S{
  int mem;
};

int main()
{
  S s;
  auto m = &S::mem;
}
