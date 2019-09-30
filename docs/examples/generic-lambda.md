# Generic Lambda {#example_generic_lambda}

<!-- source:generic-lambda.cpp -->
```{.cpp}
int main()
{
    // Generic lambdas have a method template call operator.
    auto x = [](auto x) { return x * x; };

    x(2);    // int
    x(3.0);  // double
}
```
<!-- source-end:generic-lambda.cpp -->


<!-- transformed:generic-lambda.cpp -->
Here is the transformed code:
```{.cpp}
int main()
{
    
  class __lambda_4_14
  {
    public: 
    template<class type_parameter_0_0>
    inline /*constexpr */ auto operator()(type_parameter_0_0 x) const
    {
      return x * x;
    }
    
    /* First instantiated from: generic-lambda.cpp:6 */
    #ifdef INSIGHTS_USE_TEMPLATE
    template<>
    inline /*constexpr */ int operator()(int x) const
    {
      return x * x;
    }
    #endif
    
    
    /* First instantiated from: generic-lambda.cpp:7 */
    #ifdef INSIGHTS_USE_TEMPLATE
    template<>
    inline /*constexpr */ double operator()(double x) const
    {
      return x * x;
    }
    #endif
    
    private: 
    template<class type_parameter_0_0>
    static inline auto __invoke(type_parameter_0_0 x)
    {
      return x * x;
    }
    
  };
  
  __lambda_4_14 x = __lambda_4_14{};
  x.operator()(2);
  x.operator()(3.0);
}


```
[Live view](https://cppinsights.io/lnk?code=aW50IG1haW4oKQp7CiAgICAvLyBHZW5lcmljIGxhbWJkYXMgaGF2ZSBhIG1ldGhvZCB0ZW1wbGF0ZSBjYWxsIG9wZXJhdG9yLgogICAgYXV0byB4ID0gW10oYXV0byB4KSB7IHJldHVybiB4ICogeDsgfTsKCiAgICB4KDIpOyAgICAvLyBpbnQKICAgIHgoMy4wKTsgIC8vIGRvdWJsZQp9&insightsOptions=cpp2a&rev=1.0)
<!-- transformed-end:generic-lambda.cpp -->

