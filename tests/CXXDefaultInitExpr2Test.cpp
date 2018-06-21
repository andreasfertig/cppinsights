struct Foo
{
    int mem= 0;
    int x{2};
    char c[2]{1, 2};

 public:
    Foo() {}
};
 
int main()
{
    Foo f;
}
