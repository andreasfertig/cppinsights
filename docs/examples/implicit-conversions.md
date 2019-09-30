# Implicit conversions {#example_implicit_conversions}

<!-- source:implicit-conversions.cpp -->
```{.cpp}
#include <cstdio>
template<typename U>
class X
{
public:
    X()           = default;
    X(const X& x) = default;

    template<typename T>
    X(T&& x)
    : mX{}
    {
    }

private:
    U mX;
};

int main()
{
    X<int> arr[2]{};

    // We use X<const int> instead of X<int> here. This results
    // in a constructor call to create a X<const int> object as
    // you can see in the transformation.
    for(const X<const int>& x : arr) {
    }
}
```
<!-- source-end:implicit-conversions.cpp -->


<!-- transformed:implicit-conversions.cpp -->
Here is the transformed code:
```{.cpp}
#include <cstdio>
template<typename U>
class X
{
public:
    X()           = default;
    X(const X& x) = default;

    template<typename T>
    X(T&& x)
    : mX{}
    {
    }

private:
    U mX;
};

int main()
{
  X<int> arr[2];
}


```
[Live view](https://cppinsights.io/lnk?code=I2luY2x1ZGUgPGNzdGRpbz4KdGVtcGxhdGU8dHlwZW5hbWUgVT4KY2xhc3MgWAp7CnB1YmxpYzoKICAgIFgoKSAgICAgICAgICAgPSBkZWZhdWx0OwogICAgWChjb25zdCBYJiB4KSA9IGRlZmF1bHQ7CgogICAgdGVtcGxhdGU8dHlwZW5hbWUgVD4KICAgIFgoVCYmIHgpCiAgICA6IG1Ye30KICAgIHsKICAgIH0KCnByaXZhdGU6CiAgICBVIG1YOwp9OwoKaW50IG1haW4oKQp7CiAgICBYPGludD4gYXJyWzJde307CgogICAgLy8gV2UgdXNlIFg8Y29uc3QgaW50PiBpbnN0ZWFkIG9mIFg8aW50PiBoZXJlLiBUaGlzIHJlc3VsdHMKICAgIC8vIGluIGEgY29uc3RydWN0b3IgY2FsbCB0byBjcmVhdGUgYSBYPGNvbnN0IGludD4gb2JqZWN0IGFzCiAgICAvLyB5b3UgY2FuIHNlZSBpbiB0aGUgdHJhbnNmb3JtYXRpb24uCiAgICBmb3IoY29uc3QgWDxjb25zdCBpbnQ+JiB4IDogYXJyKSB7CiAgICB9Cn0=&insightsOptions=cpp2a&rev=1.0)
<!-- transformed-end:implicit-conversions.cpp -->


