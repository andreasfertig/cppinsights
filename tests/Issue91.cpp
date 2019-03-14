template<int... Ints>
constexpr int fold_minus_impl() {
    return (Ints - ... - 5);
}

template<int... Ints>
constexpr int fold_minus() {
    return fold_minus_impl<0, Ints...>();
}

static_assert(fold_minus() == -5);

template <int b>
class print_int;

int i = fold_minus<0>();
static_assert(fold_minus<0>() == 5);
static_assert(0 - 0 - 5 == -5);

//print_int<fold_minus<0>()> p;
