# alt-syntax-for {#alt_syntax_for}
Transform all for-loops into their equivalent while-loop.

__Default:__ Off

__Examples:__

```.cpp
#include <cstdio>

int main()
{
    for(int i = 0; i < 10; ++i) {
        printf("i: %d\n", i);
    }
}
```

transforms into this:

```.cpp
#include <cstdio>

int main()
{
  {
    int i = 0;
    while(i < 10) {
      ++i;
    }
    
  }
  
}


```
