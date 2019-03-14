void X() noexcept(true);

class Sing
{
public:
    Sing() noexcept { X(); }
};

Sing & Test()
{
    static Sing s;

    return s;
}
