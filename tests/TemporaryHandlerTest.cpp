#include <iostream>
using namespace std;

class A
    {
    };

class B
    {
    public:
        operator A() const { return A(); }
    };

class C
    : public B
    {
    };

void foo( const A& )
    {
    cout << "A" << endl;
    }

void foo( B& )
    {
    cout << "B" << endl;
    }

int main()
    {
    foo( C());
    } 

