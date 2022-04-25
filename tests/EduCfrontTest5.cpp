// cmdlineinsights:-edu-show-cfront

class WithDefaultCtor
{
public:
    WithDefaultCtor() {}
};

class Type
{
public:
    Type(int) {}
    ~Type() {}
};

class Base
{
    Type mY{5};

public:
    virtual ~Base() {}
};

class BaseSecond
{
    Type mX{5};

public:
    virtual ~BaseSecond() {}
};

class Derived : public Base, public BaseSecond
{
    double          mD;
    WithDefaultCtor mWd;

public:
    ~Derived() { mD = 7; }
};

Derived d{};

int main(int argc, const char* argv[])
{
    int x{7};

    ++x;
}
