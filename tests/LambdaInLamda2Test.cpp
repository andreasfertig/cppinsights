int main()
{
    auto l = 
    [s =
      []  () {
        struct S{
            int v{676};
        };
        return S{};
      } ()
    ]  () mutable 
    {
        ++s.v;
        return s;
    };

    auto l2 = l;
}


