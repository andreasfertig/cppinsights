template <class T>
void foo(T && t)
{ }

struct Test {};

int main()
{
    Test test;
    foo(test);
    return 0;
}
