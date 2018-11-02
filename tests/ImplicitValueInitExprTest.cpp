#include <array>

struct Point
{
    double x;
    double y;
};

struct Float
{
    float f;
};

struct Char
{
    char c;
    wchar_t w;
    char16_t c16;
    char32_t c32;
};

struct Bool
{
    bool b;
};

struct Pointer
{
    char* p;
};


struct Ints
{
    int i;
    unsigned int ui;
};

struct Complex
{
    float _Complex fc;
    int _Complex ic;    
};

int main()
{
    // XXX wired nullptr in clang-7 on macOS
    //std::array<int, 10> arr{};

    Point array[10] = { [2].y = 1.0, [2].x = 2.0, [0].x = 1.0 };

    Float ff{};
    Char c{};

    Bool b{};
    Pointer p{};

    Ints i{};

    Complex cc[5] = { [2].fc = 1.0, [2].ic = 3, [0].ic = 22 };
}
