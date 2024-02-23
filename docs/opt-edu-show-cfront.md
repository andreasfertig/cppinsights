# edu-show-cfront {#edu_show_cfront}
Show transformation to C

__Default:__ Off

__Examples:__

```.cpp
class Apple
{
public:
    Apple(){};

    Apple(int x)
    : mX{x}
    {
    }

    ~Apple() { mX = 5; }

    Apple(const Apple&) {}

    void Set(int x) { mX = x; }
    int  Get() const { return mX; }

private:
    int mX{};
};

int main()
{
    Apple a{};

    a.Set(4);

    Apple* paaa{};
    paaa->Set(5);

    Apple b{6};
    Apple c{b};

    b = a;
}
```

transforms into this:

```.cpp
/*************************************************************************************
 * NOTE: This an educational hand-rolled transformation. Things can be incorrect or  *
 * buggy.                                                                            *
 *************************************************************************************/
void __cxa_start(void);
void __cxa_atexit(void);
typedef struct Apple
{
  int mX;
} Apple;

inline Apple * Constructor_Apple(Apple * __this)
{
  __this->mX = 0;
  return __this;
}

inline Apple * Constructor_Apple(Apple * __this, int x)
{
  __this->mX = x;
  return __this;
}

inline void Destructor_Apple(Apple * __this)
{
  __this->mX = 5;
}

inline Apple * CopyConstructor_Apple(Apple * __this, const Apple * __rhs)
{
  __this->mX = 0;
  return __this;
}

inline void Set(Apple * __this, int x)
{
  __this->mX = x;
}

inline int Get(const Apple * __this)
{
  return __this->mX;
}

inline Apple * operatorEqual(Apple * __this, const Apple * __rhs)
{
  __this->mX = __rhs->mX;
  return __this;
}



int __main(void)
{
  Apple a;
  ;
  Set(&a, 4);
  Apple * paaa = nullptr;
  Set(paaa, 5);
  Apple b;
  ;
  Apple c;
  ;
  operatorEqual(&b, &a);
  return 0;
  Destructor_Apple(&c);
  Destructor_Apple(&b);
  Destructor_Apple(&a);
  ;
}

int main(void)
{
  __cxa_start();
  int ret = __main();
  __cxa_atexit();
  return ret;
  /* ret // lifetime ends here */
  ;
}

void __cxa_start(void)
{
}

void __cxa_atexit(void)
{
}


```
