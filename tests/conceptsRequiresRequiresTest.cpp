// cmdline:-std=c++2a
namespace RequiresRequires {
    // Source: N4861
    template<typename T>
    requires requires (T x) { x + x; }
    T add(T a, T b) { return a + b; }
};

int main() {
    return RequiresRequires::add(2, 5);
}
