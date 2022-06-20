// cmdline:-std=c++20

template<auto SIZE>
class LambdaInNTTP {
};

static LambdaInNTTP<(+decltype([] { }){})> lambdaInNTTP();


