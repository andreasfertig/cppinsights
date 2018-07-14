template <typename T>
class Foo{
public:
    Foo() = default;

protected:
    bool Mo() { return false; }
};

class Bar : public Foo<int>
{
public:
    bool Do() { return Mo(); }
};


int main()
{
  Bar bar;

  bar.Do();
}

