#include <iostream>
using namespace std;

int a = 3;
int main() {
    int a = 3;
    {
        int a = 4;
        {
            int a = 5;
            cout << a + (::a);
        }
    }
}

