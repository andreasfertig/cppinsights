#include <cstdio>
#include <utility>

struct Foo {
    void bar() &  { printf("lvalue\n"); }
    void bar() && { printf("rvalue\n"); }
};

int main(){
    Foo f;
    f.bar();            // "lvalue"
    std::move(f).bar(); // "rvalue"
    Foo().bar();        // "rvalue"
}
