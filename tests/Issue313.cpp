// cmdlineinsights:--show-all-implicit-casts

struct A {};
struct B {
   explicit operator A() { return {}; }
};

void DangerousFunc(const A&) {}

int main()
{
  DangerousFunc(static_cast<A>(B{}));
}
