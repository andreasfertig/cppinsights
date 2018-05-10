class Base {
};

class Derived : public Base {
};

int main(){
  Derived d;
  // static_cast<Base&>(d)
  Base& b = d;
}
