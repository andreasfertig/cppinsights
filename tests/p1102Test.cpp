// cmdline:-std=c++23

int x{3};
auto noSean = [s2 = x] mutable { // Currently a syntax error.
    ++s2;
};
