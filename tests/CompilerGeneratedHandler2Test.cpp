#include <utility>


class C
{
public:
    int i;

    virtual ~C() = default;
};

class C2
{
public:
    const int i{2};

    C2() noexcept = default;
};


class D : public C
{
    public:
        int x;
};


int main()
{
    C c{};

    C c2 = c;
    C c3 = std::move(c);


}
