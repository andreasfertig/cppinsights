class Foo{ 
public:
    Foo(double amount){} 
    Foo(int x, int y=2) {}
};

class Bar: public Foo{ 
public:
      using Foo::Foo;

int mX;
};

int main()
{
    Bar bar0{100.0};
    Bar bar1(100.0);

    Bar bar2(1);
}
