void X() noexcept(false) { throw; }

class Sing
{
public:
    Sing() { X(); }
};

Sing & Test()
{
    static Sing s;

    return s;
}
