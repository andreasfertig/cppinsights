// cmdlineinsights:--show-all-implicit-casts
struct A
{
    bool operator==(const A&) const { return true; }
    bool operator!=(const A&) { return true; }
};

int main()
{
    A a{};

    if(a == a) {} // a gets constified as object and parameter
    if(a != a) {} // only the parameter gets constified, as != is not const
}
