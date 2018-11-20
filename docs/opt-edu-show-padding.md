# edu-show-padding {#edu_show_padding}
Show the padding bytes in a struct/class

__Default:__ Off

__Examples:__

```.cpp
struct Data
{
    char a;
    int  b;
    char c;
};
```

transforms into this:

```.cpp
struct Data  /* size: 12, align: 4 */
{
  char a;                         /* offset: 0, size: 1
  char __padding[3];                            size: 3 */
  int b;                          /* offset: 4, size: 4 */
  char c;                         /* offset: 8, size: 1
  char __padding[3];                            size: 3 */
};



```
