#include <string>
#include <iostream>

struct B {
    virtual void f(int) { std::cout << "B::f\n"; }
    void g(char)        { std::cout << "B::g\n"; }
    void h(int)         { std::cout << "B::h\n"; }

 protected:
    int m; // B::m is protected
    typedef int value_type;
};
 
template<typename T>
struct D : B {
    using B::m; // D::m is public
    using B::value_type; // D::value_type is public
 
    using B::f;
    void f(int) { std::cout << "D::f\n"; } // D::f(int) overrides B::f(int)
    using B::g;
    void g(int) { std::cout << "D::g\n"; } // both g(int) and g(char) are visible
                                           // as members of D
    using B::h;
    void h(int) { std::cout << "D::h\n"; } // D::h(int) hides B::h(int)
};



namespace Test
{
    struct Alloc { };

    enum class West { north };
    enum struct Best { north };
    enum  Fest { cheer };

    namespace Inner
    {
        inline namespace v1 /*inline namespace*/ {
            static const int zz=0;
            using type = int;

            namespace /*anonymous namespace*/ {
                class Void{};
            }
        }
    }
}

int main()
{
   using std::string;
   string str = "Example";

   D<int> d;

   using namespace std::string_literals; // makes visible operator""s 
                                        // from std::literals::string_literals
   auto str2 = "abc"s;   

   using Vec = Test::Alloc;
   Vec v;
   
    using W = Test::West;
    using B = Test::Best;
    using F = Test::Fest;

    namespace myStd = std;

    namespace myInner = Test::Inner;

    using vzz = Test::Inner::type;

    using Test::Inner::Void;
}
