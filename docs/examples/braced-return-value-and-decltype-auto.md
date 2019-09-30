# Braced return value and decltype(auto) {#example_braced_return_value_and_decltype_auto}

<!-- source:braced-return-value-and-decltype-auto.cpp -->
```{.cpp}
decltype(auto) Bar()
{
    int x = 22;
    // do some fancy calculation with x
    return (x);
}
```
<!-- source-end:braced-return-value-and-decltype-auto.cpp -->


<!-- transformed:braced-return-value-and-decltype-auto.cpp -->
Here is the transformed code:
```{.cpp}
int & Bar()
{
  int x = 22;
  return (x);
}


```
[Live view](https://cppinsights.io/lnk?code=ZGVjbHR5cGUoYXV0bykgQmFyKCkKewogICAgaW50IHggPSAyMjsKICAgIC8vIGRvIHNvbWUgZmFuY3kgY2FsY3VsYXRpb24gd2l0aCB4CiAgICByZXR1cm4gKHgpOwp9&insightsOptions=cpp2a&rev=1.0)
<!-- transformed-end:braced-return-value-and-decltype-auto.cpp -->


