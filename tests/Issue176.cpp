// cmdline:-std=c++2a
#include <iostream>

// Moved this out of main to get a version that compiles after transformation. As it is not allowed for a user to
// declare a method template in a function.
auto foo = []<typename T>(T i) { return 1; };

int main()
{
    foo(3);
    return 0;
}
