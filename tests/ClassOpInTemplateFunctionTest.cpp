class Foo
{
public:
    Foo(int x) : mX{x} {    }

     Foo(const char c) : mX{0} {
        *this = c;
     }

    bool operator==(const int& right) 
    {
        return false;
    }

    bool operator==(const long& right) 
    {
        return false;
    }


   template<int N>
   constexpr decltype(auto) get() const  noexcept {
     if      constexpr(N == 1) { return mX; } 
     else if constexpr(N == 0) { return 2;   }
   }


    int mX;
};


int main()
{
    Foo f1{1};
    Foo f2{2};

    const bool b2 = f1 == f2.get<1>();
}
