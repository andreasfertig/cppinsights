//: C08:ConstReturnValues.cpp
// Constant return by value
// Result cannot be used as an lvalue
class X
{
    int i;

public:
    X(int ii = 0);
    void modify();
};

X::X(int ii)
{
    i = ii;
}

void X::modify()
{
    i++;
}

X f5()
{
    return X();
}

const X f6()
{
    return X();
}

void f7(X& x)
{  // Pass by non-const reference
    x.modify();
}


X f8(bool x, int y)
{
    if( x || y == 2) {
	    return X(y);
    }

  	return X();
}

int main()
{
    f5() = X(1);    // OK -- non-const return value
    f5().modify();  // OK
    f8(false, 5) = X(2);    // OK -- non-const return value
    // Causes compile-time errors:
    //! f7(f5());
    //! f6() = X(1);
    //! f6().modify();
    //! f7(f6());
}  ///:~


#include <iostream>

class Base
{
public:
    ~Base() { std::cout << "Base dtor" << std::endl; }
};

class Foo : public Base
{
public:
    // Note: No virtual dtors 
    ~Foo() { std::cout << "Foo dtor" << std::endl; }
};

Base return_base() { return {}; }
Foo  return_foo()  { return {}; }

int main2()
{
        const Base &b {return_foo()};

        return 2;
}

struct S {
  S() { }  // User defined constructor makes S non-POD.
  ~S() { } // User defined destructor makes it non-trivial.
};
void test() {
  const S &s_ref = S(); // Requires a CXXBindTemporaryExpr.
}

