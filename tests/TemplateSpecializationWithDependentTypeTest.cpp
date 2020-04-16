#define INSIGHTS_USE_TEMPLATE

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
  struct B { };
  B foo;
};

int main()
{    

  A<int*> a;
}
