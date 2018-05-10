int main()
{
  // empty lambda directly invoked with empty body
    []{}();


    []{ 1 * 2; }();


    int a;
    int b;

    [](int& x, int& y) { x = x + y; }(a, b);
    
}

