# show-all-callexpr-template-parameters {#show_all_callexpr_template_parameters}
Show all template parameters of a CallExpr.

__Default:__ Off

__Examples:__

```.cpp
#include <utility>

auto Fun()
{
    return std::make_pair(5, 7.5);
}
```

transforms into this:

```.cpp
#include <utility>

std::pair<int, double> Fun()
{
  return std::make_pair<int, double>(5, 7.5);
}


```
