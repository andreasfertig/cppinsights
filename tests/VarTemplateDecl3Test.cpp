#define INSIGHTS_USE_TEMPLATE
// source: https://en.cppreference.com/w/cpp/language/variable_template

template<class T>
constexpr T pi = T(3.1415926535897932385L);
 
template<class T>
T circular_area(T r)
{
    return pi<T> * r * r;
}

template<typename T>
struct X
{
   T x;
   T y;
};

template<typename T>
X<T> DoIt(T x)
{
    return { circular_area(x), circular_area(x+2)};
}

int main()
{
    int x=1;

    auto[a,b] = DoIt(x);


    return circular_area(x);
}

