// cmdline:-std=c++2a

auto l = []<class T>(T t, int i) { return i; };

int main()
{
    return l(3,4) == 4;
}
