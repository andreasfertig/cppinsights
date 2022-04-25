// cmdlineinsights:-edu-show-cfront

void Fun(unsigned int& x) {}
void Fun(signed int& x) {}
void Fun(double d) {}
void Fun(const int& x) {}
void Fun(const char** x) {}
void Fun(int*) {}
void Fun(const int*) {}

void Run(int&) {}
void Run(int&&) {}

void Bun(int) {}

int main()
{
    int*       p{};
    const int* cp{};

    Fun(2);
    Fun(3.14);
    Fun(p);
    Fun(cp);

    int r{};
    Fun(r);

    unsigned ur{};
    Fun(ur);

    Run(4);

    int i{};
    Run(i);

    Bun(4);
}
