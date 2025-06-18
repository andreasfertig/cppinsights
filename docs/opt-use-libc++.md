# use-libc++ {#use_libc++}
Use libc++ (LLVM) instead of libstdc++ (GNU).

__Default:__ Off

__Examples:__

```.cpp
#include <string>

int main()
{
    const std::string s{"Hello"};

    for(const auto& e : s) {
    }
}
```

transforms into this:

```.cpp
#include <string>

int main()
{
  const std::basic_string<char, std::char_traits<char>, std::allocator<char> > s = std::basic_string<char, std::char_traits<char>, std::allocator<char> >{"Hello"};
  {
    const std::basic_string<char, std::char_traits<char>, std::allocator<char> > & __range1 = s;
    std::__wrap_iter<const char *> __begin1 = __range1.begin();
    std::__wrap_iter<const char *> __end1 = __range1.end();
    for(; std::operator!=(__begin1, __end1); __begin1.operator++()) {
      const char & e = __begin1.operator*();
    }
    
  }
  return 0;
}

```
