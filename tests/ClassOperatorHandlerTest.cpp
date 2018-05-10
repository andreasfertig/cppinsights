class Test
{
public:
    Test(int x)
    : mX{x}
    {
    }

    //    void operator++() { mX++; }
    Test operator++()
    {
        mX++;
        return *this;
    }

    Test operator++(int)
    {
        Test tmp{*this};
        mX++;

        return tmp;
    }

    Test operator--()
    {
        mX--;
        return *this;
    }
    
    Test operator--(int)
    {
        Test tmp{*this};
        mX--;

        return tmp;
    }
    
    Test& operator+=(const Test& rhs)
    {
        mX += rhs.mX;

        return *this;
    }

    Test& operator<<(const Test& rhs)
    {
        mX += rhs.mX;

        return *this;
    }

    bool operator>(const Test& rhs) { return mX > rhs.mX; }

    bool operator>=(const Test& rhs) { return mX >= rhs.mX; }

    Test& operator[](const int index) { return *this; };

private:
    int mX;
};

class Fest
{
public:
    Fest(int x)
    : mX{x}
    {
    }

    int  Get() const { return mX; }
    int& Get() { return mX; }

private:
    int mX;
};


namespace Hello {
    namespace Bello {
class Fest
{
public:
    Fest(int x)
    : mX{x}
    {
    }

    int  Get() const { return mX; }
    int& Get() { return mX; }

    Fest& operator<<(const Fest& rhs)
    {
        mX += rhs.mX;

        return *this;
    }


private:
    int mX;
};

inline bool operator<(const Fest& LHS, const Fest& RHS)
{
    return LHS.Get() < RHS.Get();
}

static Fest sF{3};
}
}

inline bool operator<(const Fest& LHS, const Fest& RHS)
{
    return LHS.Get() < RHS.Get();
}

int main()
{
    Test t1{2};

    t1++;
    t1--;

    ++t1;
    --t1;

    ++++t1;

    Test t2{5};

    t1 += t2;

    if(t1 > t2) {
        return 1;
    }

    if(t1 >= t2) {
        return 3;
    }

    Test& t3 = t1[0];

    Test t4 = t1;
    t4 << t1 << t2 << t3;

    Test* t5 = new Test(2);

    (*t5)++;
    auto tt5 = *t5++;  // increment pointer and after that dereference it

    ++*t5;
    ++++*t5;

    Fest f1{2};
    Fest f2{3};

    if(f1 < f2) {
        return 22;
    }

    Hello::Bello::Fest f3{3};
    Hello::Bello::Fest f4{3};

    if( f3 < f4 ) { return 44;}

    Hello::Bello::sF << Hello::Bello::sF << Hello::Bello::sF;
}
