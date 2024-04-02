#include <cstdio>
#include <utility>

class C {
public:
    C() { printf("default constructor\n"); }
    ~C() { printf("destructor\n"); }
    //C(const C&) { printf("copy constructor\n"); }
    C(C&&) { printf("move constructor\n"); }
    //C& operator=(const C&) { printf("copy assignment\n"); return *this; }
    C& operator=(C&&) { printf("move assignment\n"); return *this; }
private:
    int x;
};

C f() {
  C c;
  return c;
}

