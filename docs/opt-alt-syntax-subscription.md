# alt-syntax-subscription {#alt_syntax_subscription}
Transform array subscriptions E1[E2] into (*(E1 + E2)).

__Default:__ Off

__Examples:__

```.cpp
int main()
{
    int array[4]{};

    array[0] = 1;
}
```

transforms into this:

```.cpp
int main()
{
  int array[4] = {0, 0, 0, 0};
  (*(array + 0)) = 1;
  return 0;
}

```
