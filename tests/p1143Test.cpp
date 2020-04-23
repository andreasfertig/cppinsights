// cmdline:-std=c++2a
// example taken from the standard
const char *g() { return "dynamic initialization"; }
constexpr const char *f(bool p) { return p ? "constant initializer" : g(); }


constinit const char *c = f(true); // OK.

namespace constInitTest {
    constinit const char *c = f(true); // OK.
}

constinit int x = 3;

int main() {
    x = 4;
}
