// cmdline:-std=c++20
struct Point
{
  int x:2;
  int y:6;
};

Point pt{1,2};

// The following will fail after the transformation. We users cannot bind a bit-field to a non-const reference. But the
// variables below are still modifiable.
auto& [a2x, a2y] = pt;

// once we use a reference here it must be a const reference
void Func(auto x) { }

int main()
{
    Func(pt.x);

    ++a2x;

    //assert(2 == pt.x);
}
