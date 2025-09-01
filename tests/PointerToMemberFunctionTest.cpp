#define INSIGHTS_USE_TEMPLATE

struct Person {
    int age;

    int Fun(double) { return 2;}
};

template<typename Key, typename POD>
Key extractKey(Key POD::*pMember);

struct S { void f(); };

int main()
{
    auto p = &Person::Fun;
}
