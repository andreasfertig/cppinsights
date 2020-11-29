struct Foo {
  int a[5];
  float b[];
};

int main()
{
  Foo foo{{10}, {}};
  auto [a1, b1] = foo;
  const auto& [a2, b2] = foo;
}
