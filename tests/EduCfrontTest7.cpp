// cmdlineinsights:-edu-show-cfront

class Type
{
    int tint{7};
    int oint;

public:
    Type(int v)
    : oint{v}
    {
    }
    ~Type() {}
};

class DefaultCtorType
{
public:
};

class Base
{
    Type mX{5};

public:
    virtual ~Base() {}
};

class BaseSecond : private Base
{
    Type mY{5};

public:
    virtual ~BaseSecond() {}
};

class BaseThird
{
    Type mZ{5};

public:
    virtual ~BaseThird() {}
};

class Derived : public BaseSecond, public BaseThird
{
    double          mD;
    DefaultCtorType dt;
    int             g{4};

public:
    ~Derived() { mD = 7; }
};

Derived d{};
