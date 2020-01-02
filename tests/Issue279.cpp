struct S { void f(); };

int main()
{
    auto p = &S::f;
}
