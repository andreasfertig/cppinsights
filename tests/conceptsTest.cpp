// cmdline:-std=c++2a
template<typename T>
concept C = (f(T()), true);

template<typename T>
constexpr bool foo() { return false; }


template<typename T>
requires (b(T()), true)
constexpr bool FunctionWithRequiresForTemplateParametersAndReturn() requires (f(T()), true) { return true; }


template<typename T>
requires (f(T()), true)
constexpr bool FunctionWithRequiresForTemplateParameters() { return true; }

template<typename T>
constexpr bool FunctionWithRequiresForReturn() requires (f(T()), true) { return true; }

struct test 
{
    template<typename T>
    constexpr bool foo() { return false; }

    template<typename T>
      requires (f(T()), true)
    constexpr bool FunctionWithRequiresForTemplateParameters() { return true; }


    template<typename T>
    constexpr bool FunctionWithRequiresForReturn() requires (f(T()), true) { return true; }
    
    template<typename T>
    requires (b(T()), true)
    constexpr bool FunctionWithRequiresForTemplateParametersAndReturn() requires (f(T()), true) { return true; }
};


template<typename T>
concept default_constructible = requires { T{}; T(); };

template<typename T>
requires default_constructible<T>
class ClsWithRequires
{};

namespace a {
  struct A {};
  void f(A a);
  void b(A a);

  template<typename T>
  requires default_constructible<T>
  class ClsWithRequires
  {};
}

static_assert(C<a::A>);

namespace RequiresWithParens
{
    // Source: https://en.cppreference.com/w/cpp/language/constraints
    template<typename T>
    static constexpr bool get_value() { return T::value; }
 
    template<typename T>
    requires (sizeof(T) > 1 && get_value<T>())
    auto f(T t) {
        return t.x; 
    }
}


struct Value
{
    static constexpr bool value = true;
    int x;
};


int main()
{
    FunctionWithRequiresForTemplateParametersAndReturn<a::A>();
    FunctionWithRequiresForTemplateParameters<a::A>();
    FunctionWithRequiresForReturn<a::A>();

    ClsWithRequires<int> c{};

    a::ClsWithRequires<int> ac{};

    RequiresWithParens::f(Value{});
}
