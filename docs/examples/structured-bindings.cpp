struct Point
{
    int x;
    int y;
};

Point pt{1, 2};
// Here we get an additional object injected to which ax and ay refer.
auto [ax, ay] = pt;

// In case of an reference the injected object is just a reference to
// the original one.
auto& [a2x, a2y] = pt;
