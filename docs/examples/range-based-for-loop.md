# Range-based for-loop {#example_range_based_for_loop}

<!-- source:range-based-for-loop.cpp -->
```{.cpp}
#include <cstdio>

int main()
{
    const char arr[]{2, 4, 6, 8, 10};

    for(const char& c : arr) {
        printf("c=%c\n", c);
    }
}
```
<!-- source-end:range-based-for-loop.cpp -->


<!-- transformed:range-based-for-loop.cpp -->
Here is the transformed code:
```{.cpp}
#include <cstdio>

int main()
{
  const char arr[5] = {2, 4, 6, 8, 10};
  {
    char const (&__range1)[5] = arr;
    const char * __begin1 = __range1;
    const char * __end1 = __range1 + 5L;
    for(; __begin1 != __end1; ++__begin1) 
    {
      const char & c = *__begin1;
    }
    
  }
}


```
[Live view](https://cppinsights.io/lnk?code=I2luY2x1ZGUgPGNzdGRpbz4KCmludCBtYWluKCkKewogICAgY29uc3QgY2hhciBhcnJbXXsyLCA0LCA2LCA4LCAxMH07CgogICAgZm9yKGNvbnN0IGNoYXImIGMgOiBhcnIpIHsKICAgICAgICBwcmludGYoImM9JWNcbiIsIGMpOwogICAgfQp9&insightsOptions=cpp2a&rev=1.0)
<!-- transformed-end:range-based-for-loop.cpp -->




