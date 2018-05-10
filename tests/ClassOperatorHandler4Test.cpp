
class Foo
{
public:
    Foo(int x) : mX{x} {}

    int mX;
};


bool operator==(const Foo& left, const Foo& right) 
{
    return left.mX == right.mX;
};



int main()
{
    Foo f1{1};
    Foo f2{2};

    const bool b = f1 == f2;

    if( b ) {
        return 0;
    } else {
        return 1;
    }
}
