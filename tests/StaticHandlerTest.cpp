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


void B()
{
    static Bingleton bb;
}

Bingleton* B(bool c)
{
    static Bingleton bb;

    if( c ) { return nullptr; }

    return &bb;
}


Bingleton& BB(bool c)
{
    static Bingleton bb;

    if( c ) { return bb; }

    return bb;
}


int main()
{
    Singleton& s = Singleton::Instance();

    s.Set(22);

    B();

    Bingleton* bb = B(false);

    Bingleton& b2 = BB(true);
    
    return s.Get();
}
