#define INSIGHTS_USE_TEMPLATE
#include <cstdio>

static const char c[] = "Hello World";

template<auto C>
struct SC
{
    static void Print() { printf("%s\n", C); }
};

int main()
{
    SC<c> sc;

    sc.Print();
}

