void Test(int n) 
{
    char buffer[n];


    [&](int i, int x)  {
        return buffer[i] > buffer[x];
    }(1, 2);
}

int main()
{
    Test(2);
}
