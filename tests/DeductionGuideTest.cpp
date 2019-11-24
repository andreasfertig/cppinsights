// Source: https://en.cppreference.com/w/cpp/language/class_template_argument_deduction

#include <iterator>
#include <vector>

namespace Test {
// declaration of the template
template<class T> struct container {
    container(T t) {}
    template<class Iter> container(Iter beg, Iter end);
};
// additional deduction guide
template<class Iter>
container(Iter b, Iter e) -> container<typename std::iterator_traits<Iter>::value_type>;
// uses
container c(7); // OK: deduces T=int using an implicitly-generated guide
std::vector<double> v = { /* ... */};
auto d = container(v.begin(), v.end()); // OK: deduces T=double
}
