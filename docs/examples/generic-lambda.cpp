int main()
{
    // Generic lambdas have a method template call operator.
    auto x = [](auto x) { return x * x; };

    x(2);    // int
    x(3.0);  // double
}
