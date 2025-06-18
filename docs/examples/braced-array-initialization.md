# Braced array initialization {#example_braced_array_initialization}

<!-- source:braced-array-initialization.cpp -->
```{.cpp}
int main()
{
    // The compiler automatically fills the remaining
    // fields with 0 thanks to braced initialization.
    int arr[5]{2, 3, 4};
}
```
<!-- source-end:braced-array-initialization.cpp -->


<!-- transformed:braced-array-initialization.cpp -->
Here is the transformed code:
```{.cpp}
int main()
{
  int arr[5] = {2, 3, 4, 0, 0};
  return 0;
}

```
[Live view](https://cppinsights.io/lnk?code=aW50IG1haW4oKQp7CiAgICAvLyBUaGUgY29tcGlsZXIgYXV0b21hdGljYWxseSBmaWxscyB0aGUgcmVtYWluaW5nCiAgICAvLyBmaWVsZHMgd2l0aCAwIHRoYW5rcyB0byBicmFjZWQgaW5pdGlhbGl6YXRpb24uCiAgICBpbnQgYXJyWzVdezIsIDMsIDR9Owp9&insightsOptions=cpp2a&rev=1.0)
<!-- transformed-end:braced-array-initialization.cpp -->

