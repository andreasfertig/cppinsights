template<typename T>
bool Is(const T& x)
{
    T b{x};

    return b == x;
}

int main()
{
    return Is(22);
}
