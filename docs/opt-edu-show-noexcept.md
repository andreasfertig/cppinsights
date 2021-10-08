# edu-show-noexcept {#edu_show_noexcept}
Transform a function marked with `noexcept` showing the `try-catch` the compiler most likely adds.

Please note that this transformation is for educational purposes. The actual transformation happens in the back-end of
the compiler and is not visible in the AST.

__Default:__ Off

__Examples:__

```.cpp
void Fun() noexcept(true)
{
    int i = 3;
}

void Fun2() noexcept(false)
{
    int i = 3;
}
```

transforms into this:

```.cpp
void Fun() noexcept(true)
{
  try {
    int i = 3;
  } catch(...) {
    std::terminate();
  }
}


void Fun2() noexcept(false)
{
  int i = 3;
}
```
