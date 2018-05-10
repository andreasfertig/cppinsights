template<typename BT>
struct Foo
{
    BT raw;

    Foo()             = default;
    Foo(const Foo& v) = default;
    constexpr Foo(BT v)
    : BT(v)
    {
    }
    constexpr operator BT() const { return this->raw; }
    Foo&      operator=(const Foo& rhs) = default;

    Foo& operator=(BT v)
    {
        this->raw = v;
        return *this;
    }
    void operator=(BT v) volatile { this->raw = v; }
};

using Int  = Foo<int>;
using Char = Foo<char>;

class Test
{
public:
    void Apply()
    {
        Char* data            = mData;
        data[GetLength() + 1] = '\0';
        data[GetLength() + 1] = '\n';
        data[GetLength() + 1] = '\\';
        data[GetLength() + 1] = '\'';
        data[GetLength() + 1] = '\a';
        data[GetLength() + 1] = '\b';
        data[GetLength() + 1] = '\f';
        data[GetLength() + 1] = '\r';
        data[GetLength() + 1] = '\t';
        data[GetLength() + 1] = '\v';

        data[GetLength() + 1] = L'\n';
        data[GetLength() + 1] = u8'\n';
        data[GetLength() + 1] = u'\n';
        data[GetLength() + 1] = U'\n';

        data[GetLength() + 1] = 3u;

        data[GetLength() + 1] = 3ul;
        data[GetLength() + 1] = 3ull;

        data[GetLength() + 1] = 3ll;
        data[GetLength() + 1] = 3L;
    }

    Int GetLength() const { return mLength; }

private:
    Char mData[1024];
    Int  mLength;
};

int main()
{
    Test t;

    t.Apply();
}
