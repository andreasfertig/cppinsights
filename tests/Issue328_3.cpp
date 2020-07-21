template<typename T>
constexpr bool is_v = false;

class Foo{};

bool operator==(const Foo& left, const bool& right) 
{
    return false;
};

bool operator==(const bool& right, const Foo& left) 
{
    return false;
};


struct out
{
  void operator<<(bool) {}
};

static out cout;

int main()
{
    Foo f{};

    cout << (f == is_v<int>);
    cout << (is_v<int> == f);
}

