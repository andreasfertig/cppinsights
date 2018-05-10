class Foo
{
    Foo()
    :mX{0}
    {
        auto f = [this] () { 
            Get();
        };

        auto g = [&] () { 
            Get();
        };

        auto h = [*this] () { 
            Get();
        };
        
    }

    int Get() const { return 22; }

 private:
    int mX;

};


// http://en.cppreference.com/w/cpp/language/lambda
struct X {
    int x, y;
    int operator()(int);
    void f()
    {
        // the context of the following lambda is the member function X::f
        [=]()->int
        {
            return operator()(this->x + y); // X::operator()(this->x + (*this).y)
                                            // this has type X*
        };
    }
};
