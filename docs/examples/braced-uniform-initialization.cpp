struct A
{
    // user provided constructor _missing_ initialization of `j`
    A()
    : i{3}
    {
    }

    int i;
    int j;
};

struct B
{
    // uses the default constructor
    int i;
    int j;
};

int main()
{
    A a;
    A a2{};  // only i gets initialized.

    B b;
    B b2{};  // both i and j get initialized.
}
