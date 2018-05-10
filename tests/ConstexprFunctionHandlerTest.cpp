constexpr int factorial(int n)
{
    return n <= 1? 1 : (n * factorial(n - 1));
}

constexpr float factorialFloat(int n)
{
    return n <= 1? 1.0f : (n * factorialFloat(n - 1));
}


int dummy(int n)
{
    return n <= 1? 1 : (n * dummy(n - 1));
}


constexpr float Get()
{
    return 23.97f;
}

int main()
{
    const int fact = factorial(4);
    const float factFloat = factorialFloat(4);

    const int d = dummy(4);
    int fact2 = factorial(4);
    int fact3 = factorial(fact2);
    int fact4 = factorial(fact);

    float f = Get();
    const float f2 = Get();
    
    return fact + d + factFloat + fact2 + fact3 + fact4;
}
