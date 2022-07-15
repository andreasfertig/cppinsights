# edu-show-initlist {#edu_show_initlist}
Transform a std::initializer list

__Default:__ Off

__Examples:__

```.cpp
#include <vector>

std::vector<int> vec{40, 2};
```

transforms into this:

```.cpp
#include <vector>

const int __list0[2]{40, 2};
std::vector<int> vec = std::vector<int, std::allocator<int> >{std::initializer_list<int>{__list0, 2}};


```
