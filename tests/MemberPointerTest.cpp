// cmdline:-std=c++11

#define INSIGHTS_USE_TEMPLATE

struct Other {
    double Callback(int, double) { return 3.14; }
};


template<typename T>
struct Test
{
    template <typename R, typename... Args>
    void fun(R (T::*ptr)(Args...))
    {
    }    
};

int main()
{
    Test<Other> t{};

    t.fun(&Other::Callback);
}
