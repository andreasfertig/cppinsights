# auto, uniform initialization and the equal sign {#example_auto_uniform_initialization_and_the_equal_sign}

<!-- source:auto-uniform-initialization-and-the-equal-sign.cpp -->
```{.cpp}
#include <initializer_list>

int main()
{
    auto i = 3;       // int
    auto a = {42};    // initializer list
    auto b{42};       // int
    auto c = {1, 2};  // initializer list
}
```
<!-- source-end:auto-uniform-initialization-and-the-equal-sign.cpp -->


<!-- transformed:auto-uniform-initialization-and-the-equal-sign.cpp -->
Here is the transformed code:
```{.cpp}
#include <initializer_list>

int main()
{
  int i = 3;
  std::initializer_list<int> a = std::initializer_list<int>{42};
  int b = {42};
  std::initializer_list<int> c = std::initializer_list<int>{1, 2};
  return 0;
}

```
[Live view](https://cppinsights.io/lnk?code=I2luY2x1ZGUgPGluaXRpYWxpemVyX2xpc3Q+CgppbnQgbWFpbigpCnsKICAgIGF1dG8gaSA9IDM7ICAgICAgIC8vIGludAogICAgYXV0byBhID0gezQyfTsgICAgLy8gaW5pdGlhbGl6ZXIgbGlzdAogICAgYXV0byBiezQyfTsgICAgICAgLy8gaW50CiAgICBhdXRvIGMgPSB7MSwgMn07ICAvLyBpbml0aWFsaXplciBsaXN0Cn0=&insightsOptions=cpp2a&rev=1.0)
<!-- transformed-end:auto-uniform-initialization-and-the-equal-sign.cpp -->


