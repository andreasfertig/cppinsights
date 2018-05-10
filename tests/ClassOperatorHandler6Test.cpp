#include <typeinfo>
#ifndef NULL
#define NULL 0
#endif

#include <new>

class Bar {};

class Woo
{
public:
    Woo(int x, char c) : mX{x} {    }

    int mX;
};



class Foo : public Bar
{
public:
    Foo(int x) : mX{x} {    }

     Foo(const char c) : mX{0} {
        *this = c;
     }

    bool operator==(const Foo& right) 
    {
        return mX == right.mX;
    }

    bool operator==(const char* right) 
    {
        return false;
    }

    bool operator==(const Bar* right) 
    {
        return false;
    }

    bool operator==(const int& right) 
    {
        return false;
    }

    bool operator==(const long& right) 
    {
        return false;
    }

    bool operator==(const std::type_info& right) 
    {
        return false;
    }

    Foo& operator=(const char& ref)
    {
        mX = ref;

        return *this;
    }

    bool operator==(const Woo* right) 
    {
        return mX == right->mX;
    }

    bool operator==(const Woo& right) 
    {
        return mX == right.mX;
    }
    
    int mX;
};



int main()
{
    Foo f1{1};
    Foo f2{2};

    const bool b = f1 == f2;
    void *t;

    if( b ) {
        return 0;
    } else {
        return 1;
    }

    // PredefinedExpr
    const bool b2 = f1 == __func__;

    // CXXTypeidExpr
    const bool b3 = f1 == typeid(f2);

    // GNUNullExpr
    const bool b4 = f1 == __null;

    // NullptrExpr
    const bool b5 = f1 == (t == nullptr);

    // CXXNamedCastExpr
    unsigned long l = 2;
    const bool b6 = f1 == static_cast<int>(l);

    // CXXNamedCastExpr
    const bool b7 = f1 == dynamic_cast<Bar*>(&f2);
    

    // ExplicitCastExpr
    const bool b8 = f1 == (char*)t;

    // ExplicitCastExpr
    const bool b9 = f1 == new char[2];
    const bool b10 = f1 == new char[2] { 1, 2};
    
    char buffer[1024];
    const bool b11 = f1 == new (buffer) char[2] { 1, 2};

    const bool b12 = f1 == new Woo{ 1, 'a'};
    
    const bool b13 = f1 == Woo{ 1, 'a'};

    // thould trigger thisExpr
    Foo f3{'c'};
}

