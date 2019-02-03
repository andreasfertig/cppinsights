class C
{
public:
    C() = default;

    int Get() const { return ++mX; }

private:
    mutable int mX;
};

int main()
{
    C c;

    return c.Get();
}
