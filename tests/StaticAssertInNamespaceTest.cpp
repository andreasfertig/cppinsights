namespace StaticAssertInNamespaceTest {
  struct S {
    int x;
  };

  static_assert(sizeof(S) == sizeof(int));
}
