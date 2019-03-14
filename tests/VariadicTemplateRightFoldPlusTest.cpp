#define INSIGHTS_USE_TEMPLATE

template<int... Ints>
constexpr int fold_minus_impl_rf() {
    return ( 5 - ... - Ints);
}

static_assert(fold_minus_impl_rf<0>() == 5);
static_assert(fold_minus_impl_rf<0,1>() == 4);

template<int... Ints>
constexpr int fold_negative_minus_impl_rf() {
    return ( -5 - ... - Ints);
}

static_assert(fold_negative_minus_impl_rf<0>() == -5);
static_assert(fold_negative_minus_impl_rf<0,1>() == -6);
