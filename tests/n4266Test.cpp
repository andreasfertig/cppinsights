namespace [[deprecated]] Test {
    void Fun() {}
}

enum class Color  {
    Red [[deprecated]],
    Green [[deprecated]] = 2 
};

int main()
{
    Test::Fun();

    auto cl = Color::Red;
}

