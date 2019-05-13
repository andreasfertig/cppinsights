struct B {
    virtual void f(int) {  }
    void g(char)        {  }
    void h(int)         {  }
};
 
struct D : B {
    using B::f;
    void f(int) {  }
    using B::g;
    void g(int) {  }
    using B::h;
    void h(int) {  }
};

struct ProtectedMember
{
protected:
    int protectedMember;
    typedef int value_type;
};

struct UsingProtectedToPublicMember : ProtectedMember
{
    using ProtectedMember::protectedMember;
    using ProtectedMember::value_type; 
};

int main()
{
    UsingProtectedToPublicMember usingProtectedToPublicMember;
    usingProtectedToPublicMember.protectedMember = 1;
    

    D d;
    B& b = d;
 
    b.f(1);
    d.f(1);
    d.g(1);
    d.g('a');
    b.h(1);
    d.h(1);
}

