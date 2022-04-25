// cmdlineinsights:-edu-show-cfront

const int& Fun(const int& val)
{
    return val;
}

int main()
{
    const int& cref = Fun(42);

    int x = cref;
    return cref;
}
