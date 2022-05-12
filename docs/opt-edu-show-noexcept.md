# edu-show-noexcept {#edu_show_noexcept}
Transform a noexcept function

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
#include <exception> // for noexcept transformation
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
