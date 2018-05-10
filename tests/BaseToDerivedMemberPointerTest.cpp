class Base
{

};

class Derived : public Base
{
};


int main()
{
    Derived* d{new Derived};

    Base* b = d;

    Base* bb{new Derived};
}
