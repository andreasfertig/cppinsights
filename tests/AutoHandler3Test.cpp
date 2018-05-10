struct S { void f(); };
struct T {};

template <typename T>
auto f(T, int) -> decltype(T{}.f()) {
    //printf( "Has member function named f().\n");
}

template <typename T>
void f(T, ...) {
    //printf( "No member function named f().\n");
}

int main() {
    f(S{}, 0);
    f(T{}, 0);
}
