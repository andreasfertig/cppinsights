# show-all-implicit-casts {#show_all_implicit_casts}
Show all implicit casts which can be noisy.

__Default:__ Off

__Examples:__

```.cpp
int main()
{
    short s = 2;
}
```

transforms into this:

```.cpp
int main()
{
  short s = static_cast<short>(2);
  return 0;
}

```
