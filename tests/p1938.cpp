// cmdline:-std=c++2b

consteval int f(int i) { return i; }

constexpr int g(int i) {
    if consteval {
        return f(i) + 1;
    } else {
        return 42;
    }
}


constexpr int Fun(int i) {
    if not consteval {
        return 42;
    } else {
        return f(i) + 1;
    }
}

consteval int h(int i) {
    return f(i) + Fun(i);
}
