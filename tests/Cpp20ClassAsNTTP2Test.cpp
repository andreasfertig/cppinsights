// cmdline:-std=c++20

#include <cstdio>

#define INSIGHTS_USE_TEMPLATE 1

struct A{
  int x;
  constexpr A(int _x) : x{_x} {}
};

template<A a>
void Fun()
{
    printf("prim\n");
}

template<>
void Fun<A{3}>()
{
    printf("for 3\n");
}


template<A a>
bool varTmplTest = a.x;


template<bool, A a>
struct ClsTmplTest
{
};

template<A a>
struct ClsTmplTest<true, a>
{
};


int main()
{
  Fun<A{2}>();
  Fun<A{3}>();


  auto a = varTmplTest<A{4}>; 
  auto b = varTmplTest<A{3}>; // existing A!
                              
                              
ClsTmplTest<true, A{5}> clstmpl{};

}
