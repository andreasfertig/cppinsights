template<typename U>
class Test
{
public:   
    template <class T> 
    void func() 
    { 
        T t; 
        t.g(); // CXXDependentScopeMemberExpr
    }
};

struct S
{
    void g();
};

int main()
{
    Test<int> t;

    t.func<S>();
}
