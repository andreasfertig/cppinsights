#include <cstdio>
#include <cstring>
#include <new> // need this for after the transformation when a placement new is used

class Singleton
{
public:
    static Singleton& Instance();

    int Get() const { return mX;}
    void Set(int x) { mX = x;}

private:
    Singleton() = default;
    ~Singleton(){}

    int mX;

};


static size_t counter = 0;

Singleton& Singleton::Instance()
{
  static Singleton singleton;

  static bool passed = true;

  return singleton;
}

#if 0
class Bingleton
{
public:

    int Get() const { return mX;}
    void Set(int x) { mX = x;}

    Bingleton() = default;
    ~Bingleton(){}

private:
    int mX;

};


int main()
{
    auto x = [](bool c) {
    static Bingleton bb;

    if( c ) { return bb; }

    return bb;

    };


    auto y = [](bool c)  {
    static Bingleton bb;

    if( c ) { return &bb; }

    return &bb;

    };
}
#endif
