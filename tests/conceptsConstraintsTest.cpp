// cmdline:-std=c++2a
namespace Constraints {
    // N4861: [temp.constr.op]
    template<typename T>
    requires (sizeof(T) > 1) && (sizeof(T) < 255)
    void f(T) {}

    // N4861: [temp.constr.atomic]
    template <unsigned N> constexpr bool Atomic = true;
    template <unsigned N> concept C = Atomic<N>;
    template <unsigned N> concept Add1 = C<N + 1>;

    template <unsigned N> struct WrapN;
    template <unsigned N> using AddOneTy = WrapN<N + 1>;
    template <unsigned M> void g(AddOneTy<2 * M> *);

    void h() {
        g<0>(nullptr);
    }
    
}

int main() {
    Constraints::f(2);
}
