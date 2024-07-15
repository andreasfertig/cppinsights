// cmdlineinsights:-edu-show-cfront

#include <cstdio>

class A {
public:
  int          a;
  virtual void v() { puts("A->v"); }
};

class B {
public:
  int          b;
  virtual void w() { puts("B->w"); }
};

class C : public A, public B {
public:
  int  c;
  void w() { puts("C->w"); }
};

class D : public A, public B {
public:
  int d;
};

int main()
{
  C c;

  c.w();
  c.v();

  B* b = &c;
  b->w();

  C* cb = (C*)(b);
  cb->v();

  //
  D  d;
  B* bd = &d;
  D* db = (D*)bd;
  db->w();
}

