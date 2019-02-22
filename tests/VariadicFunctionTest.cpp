template<typename T>
void foo(T, ...) {}

void boo(int, ...) {}


int main()
{
   foo(1, 2.);
   boo(1, 2.);
}

