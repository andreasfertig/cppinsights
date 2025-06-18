# edu-show-cfront {#edu_show_cfront}
Show transformation to C

__Default:__ Off

__Examples:__

```.cpp
#include <cstdio>

class A
{
public:
    int          a;
    virtual void v() { puts("A->v"); }
};

class B
{
public:
    int          b;
    virtual void w() { puts("B->w"); }
};

class C : public A, public B
{
public:
    int  c;
    void w() { puts("C->w"); }
};

class D : public A, public B
{
public:
    int d;
};

class Apple
{
public:
    Apple() {}

    Apple(int x)
    : mX{x}
    {
    }

    ~Apple() { mX = 5; }

    Apple(const Apple&) {}

    void Set(int x) { mX = x; }
    int  Get() const { return mX; }

private:
    int mX{};
};

void MemberFunctions()
{
    Apple a{};

    a.Set(4);

    Apple* paaa{};
    paaa->Set(5);

    Apple b{6};
    Apple c{b};

    b = a;
}

void Inheritance()
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

int main()
{
    MemberFunctions();

    Inheritance();
}
```

transforms into this:

```.cpp
/*************************************************************************************
 * NOTE: This an educational hand-rolled transformation. Things can be incorrect or  *
 * buggy.                                                                            *
 *************************************************************************************/
void __cxa_start(void);
void __cxa_atexit(void);
typedef int (*__vptp)();

struct __mptr
{
    short  d;
    short  i;
    __vptp f;
};

extern struct __mptr* __vtbl_array[];


#include <cstdio>

typedef struct A
{
  __mptr * __vptrA;
  int a;
} A;

inline void vA(A * __this)
{
  puts("A->v");
}

inline A * Constructor_A(A * __this)
{
  __this->__vptrA = __vtbl_array[0];
  return __this;
}


typedef struct B
{
  __mptr * __vptrB;
  int b;
} B;

inline void wB(B * __this)
{
  puts("B->w");
}

inline B * Constructor_B(B * __this)
{
  __this->__vptrB = __vtbl_array[1];
  return __this;
}


typedef struct C
{
  __mptr * __vptrA;
  int a;
  __mptr * __vptrB;
  int b;
  int c;
} C;

inline void wC(C * __this)
{
  puts("C->w");
}

inline C * Constructor_C(C * __this)
{
  Constructor_A((A *)__this);
  Constructor_B((B *)__this);
  __this->__vptrA = __vtbl_array[2];
  __this->__vptrB = __vtbl_array[3];
  return __this;
}


typedef struct D
{
  __mptr * __vptrA;
  int a;
  __mptr * __vptrB;
  int b;
  int d;
} D;

inline D * Constructor_D(D * __this)
{
  Constructor_A((A *)__this);
  Constructor_B((B *)__this);
  __this->__vptrA = __vtbl_array[4];
  __this->__vptrB = __vtbl_array[5];
  return __this;
}


typedef struct Apple
{
  int mX;
} Apple;

inline Apple * Constructor_Apple(Apple * __this)
{
  __this->mX = 0;
  return __this;
}

inline Apple * Constructor_Apple(Apple * __this, int x)
{
  __this->mX = x;
  return __this;
}

inline void Destructor_Apple(Apple * __this)
{
  __this->mX = 5;
}

inline Apple * CopyConstructor_Apple(Apple * __this, const Apple * __rhs)
{
  __this->mX = 0;
  return __this;
}

inline void Set(Apple * __this, int x)
{
  __this->mX = x;
}

inline int Get(const Apple * __this)
{
  return __this->mX;
}

inline Apple * operatorEqual(Apple * __this, const Apple * __rhs)
{
  __this->mX = __rhs->mX;
  return __this;
}


void MemberFunctions(void)
{
  Apple a;
  Constructor_Apple((Apple *)&a);
  Set(&a, 4);
  Apple * paaa = nullptr;
  Set(paaa, 5);
  Apple b;
  Constructor_Apple((Apple *)&b, 6);
  Apple c;
  CopyConstructor_Apple((Apple *)&c, &b);
  operatorEqual(&b, &a);
  Destructor_Apple(&c);
  Destructor_Apple(&b);
  Destructor_Apple(&a);
}

void Inheritance(void)
{
  C c;
  Constructor_C((C *)&c);
  (*((void (*)(C *))((&c)->__vptrA[0]).f))((((C *)(char *)(&c)) + ((&c)->__vptrA[0]).d));
  (*((void (*)(A *))((&c)->__vptrA[0]).f))((((A *)(char *)(&c)) + ((&c)->__vptrA[0]).d));
  B * b = (B *)((char*)&c+16);
  (*((void (*)(B *))((b)->__vptrB[0]).f))((((B *)(char *)(b)) + ((b)->__vptrB[0]).d));
  C * cb = (C *)((char*)(b)-16);
  (*((void (*)(A *))((cb)->__vptrA[0]).f))((((A *)(char *)(cb)) + ((cb)->__vptrA[0]).d));
  D d;
  Constructor_D((D *)&d);
  B * bd = (B *)((char*)&d+16);
  D * db = (D *)((char*)bd-16);
  (*((void (*)(B *))((db)->__vptrB[0]).f))((((B *)(char *)(db)) + ((db)->__vptrB[0]).d));
  /* d // lifetime ends here */
  /* c // lifetime ends here */
}

int __main(void)
{
  MemberFunctions()Inheritance();
  return 0;
}

int main(void)
{
  __cxa_start();
  int ret = __main();
  __cxa_atexit();
  return ret;
  /* ret // lifetime ends here */
}

__mptr __vtbl_A[1] = {0, 0, (__vptp)vA};
__mptr __vtbl_B[1] = {0, 0, (__vptp)wB};
__mptr __vtbl_CA[2] = {{0, 0, (__vptp)vA}, {0, 0, (__vptp)wC}};
__mptr __vtbl_CB[1] = {-16, 0, (__vptp)wC};
__mptr __vtbl_DA[1] = {0, 0, (__vptp)vA};
__mptr __vtbl_DB[1] = {0, 0, (__vptp)wB};

__mptr * __vtbl_array[6] = {__vtbl_A, __vtbl_B, __vtbl_CA, __vtbl_CB, __vtbl_DA, __vtbl_DB};

void __cxa_start(void)
{
}

void __cxa_atexit(void)
{
}


```
