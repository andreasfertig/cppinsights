class Base {
public:
  Base(double) {}
};

class Derived : public Base{
public:
  using Base::Base;
  
  int someValue;
};
  
  
int main()
{
  Derived d(3.14);
}
