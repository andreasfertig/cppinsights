# edu-show-lifetime {#edu_show_lifetime}
Show lifetime of objects

__Default:__ Off

__Examples:__

```.cpp
struct Data
{
    int mData[5]{};

    const int* begin() const { return mData; }
    const int* end() const { return mData + 1; }
};

struct Keeper
{
    Data data{2, 3, 4};

    auto& items() const { return data; }
};

Keeper get()
{
    return {};
}

int main()
{
    for(auto& item : get().items()) {
    }
}
```

transforms into this:

```.cpp
/*************************************************************************************
 * NOTE: This an educational hand-rolled transformation. Things can be incorrect or  *
 * buggy.                                                                            *
 *************************************************************************************/
struct Data
{
  int mData[5]{0, 0, 0, 0, 0};
  inline const int * begin() const
  {
    return this->mData;
  }
  
  inline const int * end() const
  {
    return this->mData + 1;
  }
  
};


struct Keeper
{
  Data data{{2, 3, 4, 0, 0}};
  inline const Data & items() const
  {
    return this->data;
  }
  
};


Keeper get()
{
  return {{{2, 3, 4, 0, 0}}};
}

int main()
{
  {
    Keeper __temporary23_26 = get();
    const Data & __range1 = static_cast<const Keeper &&>(__temporary23_26).items();
    /* __temporary23_26 // lifetime ends here */
    const int * __begin1 = __range1.begin();
    const int * __end1 = __range1.end();
    for(; __begin1 != __end1; ++__begin1) {
      const int & item = *__begin1;
      /* item // lifetime ends here */
    }
    
    /* __range1 // lifetime ends here */
  }
  return 0;
}

```
