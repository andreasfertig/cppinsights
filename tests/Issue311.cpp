struct alignas(double) foo {
    int i;
};

static_assert(sizeof(foo) == 8);

