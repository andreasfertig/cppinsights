//CXXDependentScopeMemberExpr
// http://lists.llvm.org/pipermail/cfe-dev/2017-January/052433.html

template <typename T>
struct A
{
   void foo(int);
};

template <typename T>
struct A<T*>
{
  struct B { void operator()(int); };
  B foo;
};

template <typename T>
void foo()
{
  A<T> a;
  a.foo(3);
}

template void foo<int>(); // first case
template void foo<int*>(); // second case


#include <vector>

template <typename T>
bool foo2() {
    std::vector<T> vec;
    //vec._M_impl;          // 'vec' is identified as part of 'CXXDependentScopeMemberExpr' but access to member field '_M_impl' is not and it's completely missing from the AST
    return vec.empty(); // 'vec' is identified as part of 'CXXDependentScopeMemberExpr' but call expression 'empty' is not and it's completely missing from the AST
}

bool bar() {
    std::vector<int> vec;
//    vec._M_impl; // both 'vec' & access to member field '_M_impl' are identified as expected
    return vec.empty(); // both 'vec' & 'empty' are identified as expected
}


int main()
{
    
    foo<int>();
    foo<int*>();

    foo2<int>();
}
