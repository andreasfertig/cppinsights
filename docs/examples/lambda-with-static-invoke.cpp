int main()
{
    int (*fp)(int, char) = [](int a, char b) { return a + b; };
}
