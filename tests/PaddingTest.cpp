// cmdlineinsights:-edu-show-padding

struct __attribute__((packed)) test0 {
    int   i;
    char  c;
    char  c2;
    float f;
};

struct test1 {
    int   i;
    char  c;
    char  c2;
    float f;
};

struct test2 {
    int  i;
    char c;
};

struct test3 {
    int  i;
    char c;

    struct X {
        char c;
        int  x;
        int  y;
    };

    X x;
};


struct NVBase {
    int x;
};

struct NVDerived : NVBase {
    int x;
};

struct NVDerived2 : NVDerived {
    int x;
};

struct NVBase2 {
    int x;
};

struct NVDerived3 : NVBase, NVBase2 {
    int x;
};




struct Base {
    virtual ~Base() = default;
    virtual void fun() {}
};

struct Derived : Base {
    int x;
};


struct Empty {};

struct test5 {
    int i;
};

struct test4 {
    alignas(sizeof(int) * 2) int i;
};

struct Data {
    int  b;
    char a;
    char c;
};



struct alignas(64) Test5 {
    int i;
};

struct Test6 : Test5 {
    int u;
};

