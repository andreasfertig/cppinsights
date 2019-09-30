#include <cstdio>

int main()
{
    const char arr[]{2, 4, 6, 8, 10};

    for(const char& c : arr) {
        printf("c=%c\n", c);
    }
}
