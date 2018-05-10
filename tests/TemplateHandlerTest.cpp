#include <iostream>


template<typename T>
constexpr T min(const T& a, const T& b)
{
    return (a < b) ? a : b;
} 

template<typename T>
static T max(const T& a, const T& b)
{
    return (a > b) ? a : b;
} 


template<typename T>
class Template
{
public:
    Template(const T& x)
        : mX{x}
    {}

    T Get() const { return mX;}

private:
    const T mX;
};


 
void tprintf(const char* format) // base function
{
    std::cout << format;
}
 
template<typename T, typename... Targs>
void tprintf(const char* format, T value, Targs... Fargs) // recursive variadic function
{
    for ( ; *format != '\0'; format++ ) {
        if ( *format == '%' ) {
           std::cout << value;
           tprintf(format+1, Fargs...); // recursive call
           return;
        }
        std::cout << *format;
    }
}

int main()
{
    int a=1, b=2;

    int mi = min(a, b);

    double ad=2.4, bd=3.4;

    auto md = min( ad, bd);

    Template<int> w{2};

    Template<double> ww{3.0};

    tprintf("% world% %\n","Hello",'!',123);

    int ma1 = max(1, 2);
    double ma2 = max(2.0, 4.0);
}
