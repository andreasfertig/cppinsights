// cmdline:-std=c++98
void X() { throw; }

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
