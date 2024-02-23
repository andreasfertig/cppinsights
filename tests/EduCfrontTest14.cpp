// cmdlineinsights:-edu-show-cfront

class A
{
public:
    ~A() {}
};

class B
{
public:
    B() = default;

    B(B&&) {}
};

int main()
{
    A* a = new A[3]{};

    delete[] a;

    int* i = new int[3]{2, 3};

    int* x = new int{2};

    B b1{};

    B b2{static_cast<B&&>(b1)};
}
