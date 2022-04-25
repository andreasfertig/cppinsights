# Braced (uniform) Initialization {#example_braced_uniform_initialization}

<!-- source:braced-uniform-initialization.cpp -->
```{.cpp}
struct A
{
    // user provided constructor _missing_ initialization of `j`
    A()
    : i{3}
    {
    }

    int i;
    int j;
};

struct B
{
    // uses the default constructor
    int i;
    int j;
};

int main()
{
    A a;
    A a2{};  // only i gets initialized.

    B b;
    B b2{};  // both i and j get initialized.
}
```
<!-- source-end:braced-uniform-initialization.cpp -->


<!-- transformed:braced-uniform-initialization.cpp -->
Here is the transformed code:
```{.cpp}
struct A
{
  inline A()
  : i{3}
  {
  }
  
  int i;
  int j;
};



struct B
{
  int i;
  int j;
  // inline B() noexcept = default;
};



int main()
{
  A a = A();
  A a2 = A{};
  B b;
  B b2 = {0, 0};
  return 0;
}


```
[Live view](https://cppinsights.io/lnk?code=c3RydWN0IEEKewogICAgLy8gdXNlciBwcm92aWRlZCBjb25zdHJ1Y3RvciBfbWlzc2luZ18gaW5pdGlhbGl6YXRpb24gb2YgYGpgCiAgICBBKCkKICAgIDogaXszfQogICAgewogICAgfQoKICAgIGludCBpOwogICAgaW50IGo7Cn07CgpzdHJ1Y3QgQgp7CiAgICAvLyB1c2VzIHRoZSBkZWZhdWx0IGNvbnN0cnVjdG9yCiAgICBpbnQgaTsKICAgIGludCBqOwp9OwoKaW50IG1haW4oKQp7CiAgICBBIGE7CiAgICBBIGEye307ICAvLyBvbmx5IGkgZ2V0cyBpbml0aWFsaXplZC4KCiAgICBCIGI7CiAgICBCIGIye307ICAvLyBib3RoIGkgYW5kIGogZ2V0IGluaXRpYWxpemVkLgp9&insightsOptions=cpp2a&rev=1.0)
<!-- transformed-end:braced-uniform-initialization.cpp -->

