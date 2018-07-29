#define INSIGHTS_USE_TEMPLATE

#include <vector>

template<typename T>
class MyVector
{
    std::vector<T> v;

public:
    // MyVector with initial size
    MyVector(const std::size_t n)
    : v(n)
    {
    }

    // MyVector with initial size and value
    MyVector(const std::size_t n, const double initialValue)
    : v(n, initialValue)
    {
    }

    // size of underlying container
    std::size_t size() const { return v.size(); }

    // index operators
    T operator[](const std::size_t i) const { return v[i]; }

    T& operator[](const std::size_t i) { return v[i]; }
};

using vec_t = MyVector<int>;

vec_t operator+(const vec_t& a, const vec_t& b)
{
    vec_t result(a.size());
    for(std::size_t s = 0; s <= a.size(); ++s) {
        result[s] = a[s] + b[s];
    }
    return result;
}

template<typename T>
MyVector<T> operator*(const MyVector<T>& a, const MyVector<T>& b)
{
    MyVector<T> result(a.size());
    for(std::size_t s = 0; s <= a.size(); ++s) {
        result[s] = a[s] + b[s];
    }
    return result;
}

vec_t Foo(const vec_t& a, const vec_t& b)
{
    vec_t result{a.size()};
    for(std::size_t s = 0; s <= a.size(); ++s) {
        result[s] = a[s] + b[s];
    }
    return result;
}

int main()
{
    vec_t vecA(4, 2);
    vec_t vecB(6, 7);

    vec_t res = Foo(vecA, vecB);

    vecA + vecB;
    vecA* vecB;
}
