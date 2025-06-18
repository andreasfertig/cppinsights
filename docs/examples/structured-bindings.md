# Structured Bindings {#example_structured_bindings}

<!-- source:structured-bindings.cpp -->
```{.cpp}
struct Point
{
    int x;
    int y;
};

Point pt{1, 2};
// Here we get an additional object injected to which ax and ay refer.
auto [ax, ay] = pt;

// In case of an reference the injected object is just a reference to
// the original one.
auto& [a2x, a2y] = pt;
```
<!-- source-end:structured-bindings.cpp -->


<!-- transformed:structured-bindings.cpp -->
Here is the transformed code:
```{.cpp}
struct Point
{
  int x;
  int y;
  // inline constexpr Point(const Point &) noexcept = default;
};


Point pt = {1, 2};

Point __pt9 = Point(pt);
int & ax = __pt9.x;
int & ay = __pt9.y;

Point & __pt13 = pt;
int & a2x = __pt13.x;
int & a2y = __pt13.y;

```
[Live view](https://cppinsights.io/lnk?code=c3RydWN0IFBvaW50CnsKICAgIGludCB4OwogICAgaW50IHk7Cn07CgpQb2ludCBwdHsxLCAyfTsKLy8gSGVyZSB3ZSBnZXQgYW4gYWRkaXRpb25hbCBvYmplY3QgaW5qZWN0ZWQgdG8gd2hpY2ggYXggYW5kIGF5IHJlZmVyLgphdXRvIFtheCwgYXldID0gcHQ7CgovLyBJbiBjYXNlIG9mIGFuIHJlZmVyZW5jZSB0aGUgaW5qZWN0ZWQgb2JqZWN0IGlzIGp1c3QgYSByZWZlcmVuY2UgdG8KLy8gdGhlIG9yaWdpbmFsIG9uZS4KYXV0byYgW2EyeCwgYTJ5XSA9IHB0Ow==&insightsOptions=cpp2a&rev=1.0)
<!-- transformed-end:structured-bindings.cpp -->


