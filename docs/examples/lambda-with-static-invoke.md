# Lambda with static invoke function {#example_lambda_with_static_invoke}

<!-- source:lambda-with-static-invoke.cpp -->
```{.cpp}
int main()
{
    int (*fp)(int, char) = [](int a, char b) { return a + b; };
}
```
<!-- source-end:lambda-with-static-invoke.cpp -->


<!-- transformed:lambda-with-static-invoke.cpp -->
Here is the transformed code:
```{.cpp}
int main()
{
      
  class __lambda_3_28
  {
    public: 
    inline /*constexpr */ int operator()(int a, char b) const
    {
      return a + static_cast<int>(b);
    }
    
    using retType_3_28 = int (*)(int, char);
    inline constexpr operator retType_3_28 () const noexcept
    {
      return __invoke;
    }
    
    private: 
    static inline /*constexpr */ int __invoke(int a, char b)
    {
      return __lambda_3_28{}.operator()(a, b);
    }
    
    
  };
  
  using FuncPtr_3 = int (*)(int, char);
  FuncPtr_3 fp = __lambda_3_28{}.operator __lambda_3_28::retType_3_28();
  return 0;
}

```
[Live view](https://cppinsights.io/lnk?code=aW50IG1haW4oKQp7CiAgICBpbnQgKCpmcCkoaW50LCBjaGFyKSA9IFtdKGludCBhLCBjaGFyIGIpIHsgcmV0dXJuIGEgKyBiOyB9Owp9&insightsOptions=cpp2a&rev=1.0)
<!-- transformed-end:lambda-with-static-invoke.cpp -->

