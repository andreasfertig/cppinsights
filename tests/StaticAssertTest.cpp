struct S {
  int x;
};

static_assert(sizeof(S) == sizeof(int));

//static_assert(sizeof(S) != sizeof(int));

