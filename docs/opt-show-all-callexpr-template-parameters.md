# show-all-callexpr-template-parameters {#show_all_callexpr_template_parameters}

C++ Insights in default mode hides the template parameters used when invoking a function template. This option shows the
usually hidden parameters.

__Default:__ Off

__Examples:__

```.cpp
#include <utility>

auto Fun() {
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
