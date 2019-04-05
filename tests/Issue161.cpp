#include <iostream>

int main(int argc, char** argv)
{
    char arr[10]{2,4,6,8};
    for (auto i : arr)
        std::cout << i << " ";
}
