template <typename T>
class Foo{};

typedef Foo<int> IntFoo; // this type is never instantiated which results in a 
                         // CXXRecordDecl without a definition
typedef Foo<double> DoubleFoo;


int main()
{
  DoubleFoo df;
}
