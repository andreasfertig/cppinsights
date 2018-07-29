#define INSIGHTS_USE_TEMPLATE
#include <utility>

struct Test {};

template <class T>
void foo(T && t)
{ }

int main()
{
    Test test;
    foo(test);

  	int i=0;
  	foo(i);
  
    long l;
    foo(std::move(l));
}
