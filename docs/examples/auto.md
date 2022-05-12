# auto {#example_auto}

<!-- source:auto.cpp -->
```{.cpp}
class CTest
{
    auto Test() { return 22; }
};

auto Test()
{
    return 1;
}

auto Best() -> int
{
    return 1;
}

constexpr auto CEBest() -> int
{
    return 1;
}

decltype(auto) West()
{
    return 'c';
}

constexpr decltype(auto) CEWest()
{
    return 'c';
}

[[maybe_unused]] inline constexpr decltype(auto) MUCEWest()
{
    return 'c';
}

int main()
{
    int            x = 2;
    const char*    p;
    constexpr auto cei       = 0;
    auto constexpr cei2      = 0;
    auto                 i   = 0;
    decltype(auto)       xX  = (i);
    auto                 ii  = &i;
    auto&                ir  = i;
    auto*                ip  = &i;
    const auto*          cip = &i;
    auto*                pp  = p;
    const auto*          cp  = p;
    volatile const auto* vcp = p;
    auto                 f   = 1.0f;
    auto                 c   = 'c';
    auto                 u   = 0u;
    decltype(u)          uu  = u;

    [[maybe_unused]] auto        mu  = 0u;
    [[maybe_unused]] decltype(u) muu = u;
}
```
<!-- source-end:auto.cpp -->


<!-- transformed:auto.cpp -->
Here is the transformed code:
```{.cpp}
class CTest
{
  inline int Test()
  {
    return 22;
  }
  
};



int Test()
{
  return 1;
}


int Best()
{
  return 1;
}


inline constexpr int CEBest()
{
  return 1;
}


char West()
{
  return 'c';
}


inline constexpr char CEWest()
{
  return 'c';
}

[[maybe_unused]] inline constexpr char MUCEWest()
                 {
                   return 'c';
                 }
                 

int main()
{
  int x = 2;
  const char * p;
  constexpr const int cei = 0;
  constexpr const int cei2 = 0;
  int i = 0;
  int & xX = (i);
  int * ii = &i;
  int & ir = i;
  int * ip = &i;
  const int * cip = &i;
  const char * pp = p;
  const char * cp = p;
  const volatile char * vcp = p;
  float f = 1.0F;
  char c = 'c';
  unsigned int u = 0U;
  unsigned int uu = u;
  [[maybe_unused]] unsigned int mu = 0U;
  [[maybe_unused]] unsigned int muu = u;
  return 0;
}


```
[Live view](https://cppinsights.io/lnk?code=Y2xhc3MgQ1Rlc3QKewogICAgYXV0byBUZXN0KCkgeyByZXR1cm4gMjI7IH0KfTsKCmF1dG8gVGVzdCgpCnsKICAgIHJldHVybiAxOwp9CgphdXRvIEJlc3QoKSAtPiBpbnQKewogICAgcmV0dXJuIDE7Cn0KCmNvbnN0ZXhwciBhdXRvIENFQmVzdCgpIC0+IGludAp7CiAgICByZXR1cm4gMTsKfQoKZGVjbHR5cGUoYXV0bykgV2VzdCgpCnsKICAgIHJldHVybiAnYyc7Cn0KCmNvbnN0ZXhwciBkZWNsdHlwZShhdXRvKSBDRVdlc3QoKQp7CiAgICByZXR1cm4gJ2MnOwp9CgpbW21heWJlX3VudXNlZF1dIGlubGluZSBjb25zdGV4cHIgZGVjbHR5cGUoYXV0bykgTVVDRVdlc3QoKQp7CiAgICByZXR1cm4gJ2MnOwp9CgppbnQgbWFpbigpCnsKICAgIGludCAgICAgICAgICAgIHggPSAyOwogICAgY29uc3QgY2hhciogICAgcDsKICAgIGNvbnN0ZXhwciBhdXRvIGNlaSAgICAgICA9IDA7CiAgICBhdXRvIGNvbnN0ZXhwciBjZWkyICAgICAgPSAwOwogICAgYXV0byAgICAgICAgICAgICAgICAgaSAgID0gMDsKICAgIGRlY2x0eXBlKGF1dG8pICAgICAgIHhYICA9IChpKTsKICAgIGF1dG8gICAgICAgICAgICAgICAgIGlpICA9ICZpOwogICAgYXV0byYgICAgICAgICAgICAgICAgaXIgID0gaTsKICAgIGF1dG8qICAgICAgICAgICAgICAgIGlwICA9ICZpOwogICAgY29uc3QgYXV0byogICAgICAgICAgY2lwID0gJmk7CiAgICBhdXRvKiAgICAgICAgICAgICAgICBwcCAgPSBwOwogICAgY29uc3QgYXV0byogICAgICAgICAgY3AgID0gcDsKICAgIHZvbGF0aWxlIGNvbnN0IGF1dG8qIHZjcCA9IHA7CiAgICBhdXRvICAgICAgICAgICAgICAgICBmICAgPSAxLjBmOwogICAgYXV0byAgICAgICAgICAgICAgICAgYyAgID0gJ2MnOwogICAgYXV0byAgICAgICAgICAgICAgICAgdSAgID0gMHU7CiAgICBkZWNsdHlwZSh1KSAgICAgICAgICB1dSAgPSB1OwoKICAgIFtbbWF5YmVfdW51c2VkXV0gYXV0byAgICAgICAgbXUgID0gMHU7CiAgICBbW21heWJlX3VudXNlZF1dIGRlY2x0eXBlKHUpIG11dSA9IHU7Cn0=&insightsOptions=cpp2a&rev=1.0)
<!-- transformed-end:auto.cpp -->



