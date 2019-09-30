# Recursive Template {#example_recursive_template}

<!-- source:recursive-template.cpp -->
```{.cpp}
#include <cstdio>
#include <vector>

template<int n>
struct A
{
    static const auto value = A<n - 1>::value + n;
};

template<>
struct A<1>
{
    static const auto value = 1;
};

int main()
{
    printf("c=%c\n", A<5>::value);
}
```
<!-- source-end:recursive-template.cpp -->


<!-- transformed:recursive-template.cpp -->
Here is the transformed code:
```{.cpp}
#include <cstdio>
#include <vector>

template<int n>
struct A
{
    static const auto value = A<n - 1>::value + n;
};

template<>
struct A<1>
{
  static const int value = 1;
};



int main()
{
}


```
[Live view](https://cppinsights.io/lnk?code=I2luY2x1ZGUgPGNzdGRpbz4KI2luY2x1ZGUgPHZlY3Rvcj4KCnRlbXBsYXRlPGludCBuPgpzdHJ1Y3QgQQp7CiAgICBzdGF0aWMgY29uc3QgYXV0byB2YWx1ZSA9IEE8biAtIDE+Ojp2YWx1ZSArIG47Cn07Cgp0ZW1wbGF0ZTw+CnN0cnVjdCBBPDE+CnsKICAgIHN0YXRpYyBjb25zdCBhdXRvIHZhbHVlID0gMTsKfTsKCmludCBtYWluKCkKewogICAgcHJpbnRmKCJjPSVjXG4iLCBBPDU+Ojp2YWx1ZSk7Cn0=&insightsOptions=cpp2a&rev=1.0)
<!-- transformed-end:recursive-template.cpp -->

